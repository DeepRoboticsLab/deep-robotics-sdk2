/**
 * @file user_command_interface.hpp
 * @brief User command input interface for the state machine.
 * @author DEEPRobotics
 * @date 2026-06-27
 *
 * @copyright Copyright (c) 2026 DEEPRobotics
 *
 */
#pragma once

#include <fcntl.h>
#include <unistd.h>
#include <chrono>
#include <ctime>
#include <iostream>
#include <thread>

#include <termios.h>

#include "utils.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <functional>
#include <memory>
#include <unordered_map>

#include "types.h"
#include "drdds/msg/steer.hpp"
#include "motion_state_store.hpp"
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

#define AXIS_STEP 0.70f
#define KEYBOARD_REFRESH_MS 20
#define KEY_HOLD_TIMEOUT_MS 320.0

namespace deep_robotics::dr02_std {

class UserCommandInterface {
 public:
  /**
   * @brief Construct a user command interface.
   * @param node ROS2 node used to create input subscribers.
   */
  explicit UserCommandInterface(rclcpp::Node::SharedPtr node)
      : node_(node) {
    if (!node_) {
      std::cerr << "UserCommandInterface node is null" << std::endl;
    }
    usr_cmd_ = new UserCommand();
    std::memset(usr_cmd_, 0, sizeof(UserCommand));

    gamepad_key_sub_ = node_->create_subscription<std_msgs::msg::String>(
        "/GAMEPAD_KEY", 10, std::bind(&UserCommandInterface::HandleKey, this, std::placeholders::_1));
    steer_sub_ = node_->create_subscription<drdds::msg::Steer>(
        "/STEER", 10, std::bind(&UserCommandInterface::HandleSteer, this, std::placeholders::_1));
    real_steer_sub_ = node_->create_subscription<drdds::msg::Steer>(
        "/REAL_STEER", 10, std::bind(&UserCommandInterface::HandleRealSteer, this, std::placeholders::_1));
  }

  virtual ~UserCommandInterface() { delete usr_cmd_; }

  /**
   * @brief Start keyboard and gamepad command handling.
   */
  virtual void Start() {
    std::memset(usr_cmd_, 0, sizeof(UserCommand));
    std::cout << "\n╔════════════════════════════════════════════════╗\n"
              << "║                GAMEPAD TELEOP                  ║\n"
              << "╚════════════════════════════════════════════════╝\n"
              << "  Movement:  Left joystick\n"
              << "  Rotation:  Right joystick\n"
              << "  Mode:      L1 (zeropos)  L2 (rl control)  R2 (damping) \n"
              << "╔════════════════════════════════════════════════╗\n"
              << "║                KEYBOARD TELEOP                 ║\n"
              << "╚════════════════════════════════════════════════╝\n"
              << "  Movement:  W/S (forward/back)  A/D (left/right)\n"
              << "  Rotation:  Q (CCW)  E (CW)\n"
              << "  Mode:      Z (zeropos)  C (rl control)  R (damping) \n"
              << "\n";

    start_thread_flag_ = true;
    kb_thread_ = std::thread(std::bind(&UserCommandInterface::KeyboardLoop, this));
  }

  /**
   * @brief Stop keyboard command handling.
   */
  virtual void Stop() {
    start_thread_flag_ = false;
    if (kb_thread_.joinable()) {
      kb_thread_.join();
    }
  }

  /**
   * @brief Get the current user command.
   * @return Pointer to the current user command.
   */
  virtual UserCommand* GetUserCommand() { return usr_cmd_; }

  /**
   * @brief Set the shared motion-state store.
   * @param motion_state_store Shared motion-state store.
   */
  virtual void SetMotionStateStore(const std::shared_ptr<MotionStateStore>& motion_state_store) {
    motion_state_store_ = motion_state_store;
  }

  rclcpp::Node::SharedPtr node_;
  std::shared_ptr<MotionStateStore> motion_state_store_;
  UserCommand* usr_cmd_;

 private:
  /**
   * @brief Get elapsed time since startup in milliseconds.
   * @return Elapsed time in milliseconds.
   */
  double GetCurrentTimeStamp() {
    static timespec startup_timestamp;
    timespec now_timestamp;
    if (startup_timestamp.tv_sec + startup_timestamp.tv_nsec == 0) {
      clock_gettime(CLOCK_MONOTONIC, &startup_timestamp);
    }
    clock_gettime(CLOCK_MONOTONIC, &now_timestamp);
    return (now_timestamp.tv_sec - startup_timestamp.tv_sec) * 1e3 +
           (now_timestamp.tv_nsec - startup_timestamp.tv_nsec) / 1e6;
  }

  /**
   * @brief Get the current motion state.
   * @return Current robot motion state.
   */
  RobotMotionState GetCurrentState() const {
    return motion_state_store_ ? motion_state_store_->GetState() : RobotMotionState::Idle;
  }

  /**
   * @brief Handle gamepad key messages from /GAMEPAD_KEY.
   * @param msg Received key message.
   */
  void HandleKey(const std_msgs::msg::String::SharedPtr msg) {
    static const std::unordered_map<std::string, KeyCode> key_map = {{"G20_KEY_L1", KeyCode::L1},
                                                                     {"G20_KEY_L2", KeyCode::L2},
                                                                     {"G20_KEY_R1", KeyCode::R1},
                                                                     {"G20_KEY_R2", KeyCode::R2}};

    KeyCode code = KeyCode::UNKNOWN;
    const auto it = key_map.find(msg->data);
    if (it != key_map.end()) {
      code = it->second;
    }

    const RobotMotionState current_state = GetCurrentState();
    switch (code) {
      case KeyCode::L1:
        if (current_state == RobotMotionState::Idle) {
          usr_cmd_->target_mode = uint8_t(RobotMotionState::ZeroPos);
          std::cout << "[MODE] Zero Pos\n";
        }
        break;
      case KeyCode::L2:
        if (current_state == RobotMotionState::ZeroPos) {
          usr_cmd_->target_mode = uint8_t(RobotMotionState::RLControl);
        }
        break;
      case KeyCode::R1:
        break;
      case KeyCode::R2:
        usr_cmd_->target_mode = uint8_t(RobotMotionState::JointDamping);
        std::cout << "[MODE] Joint Damping\n";
        break;
      default:
        break;
    }
  }

  /**
   * @brief Handle normalized steer messages from /STEER.
   * @param msg Received steer message.
   */
  void HandleSteer(const drdds::msg::Steer::SharedPtr msg) {
    usr_cmd_->forward_vel_scale = msg->data.x;
    usr_cmd_->side_vel_scale = msg->data.y;
    usr_cmd_->turning_vel_scale = msg->data.yaw;
  }

  /**
   * @brief Handle real-velocity steer messages from /REAL_STEER.
   * @param msg Received steer message.
   */
  void HandleRealSteer(const drdds::msg::Steer::SharedPtr msg) {
    usr_cmd_->forward_vel_scale = RetLimitNumber(msg->data.x, -1.0f, 1.0f);
    usr_cmd_->side_vel_scale = RetLimitNumber(msg->data.y / 0.75f, -1.0f, 1.0f);
    usr_cmd_->turning_vel_scale = RetLimitNumber(msg->data.yaw / 2.25f, -1.0f, 1.0f);
  }

  /**
   * @brief Poll keyboard input and update the current user command.
   */
  void KeyboardLoop() {
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

    char input;
    double forward_last_input_ms = -1e9;
    double side_last_input_ms = -1e9;
    double turnning_last_input_ms = -1e9;
    bool keyboard_forward_active = false;
    bool keyboard_side_active = false;
    bool keyboard_turnning_active = false;

    while (start_thread_flag_) {
      usr_cmd_->time_stamp = GetCurrentTimeStamp();
      const double current_time_ms = usr_cmd_->time_stamp;
      const RobotMotionState current_state = GetCurrentState();

      if (read(STDIN_FILENO, &input, 1) != -1) {
        const char key = static_cast<char>(std::tolower(static_cast<unsigned char>(input)));
        std::cout << "input: " << input << std::endl;
        if (key == 'r') {
          usr_cmd_->target_mode = uint8_t(RobotMotionState::JointDamping);
        }
        switch (current_state) {
          case RobotMotionState::Idle:
            if (key == 'z') {
              usr_cmd_->target_mode = uint8_t(RobotMotionState::ZeroPos);
            }
            break;
          case RobotMotionState::ZeroPos:
            if (key == 'c') {
              usr_cmd_->target_mode = uint8_t(RobotMotionState::RLControl);
            }
            break;
          case RobotMotionState::RLControl:
            if (key == 'w') {
              usr_cmd_->forward_vel_scale = AXIS_STEP;
              forward_last_input_ms = current_time_ms;
              keyboard_forward_active = true;
            } else if (key == 's') {
              usr_cmd_->forward_vel_scale = -AXIS_STEP;
              forward_last_input_ms = current_time_ms;
              keyboard_forward_active = true;
            }

            if (key == 'a') {
              usr_cmd_->side_vel_scale = AXIS_STEP;
              side_last_input_ms = current_time_ms;
              keyboard_side_active = true;
            } else if (key == 'd') {
              usr_cmd_->side_vel_scale = -AXIS_STEP;
              side_last_input_ms = current_time_ms;
              keyboard_side_active = true;
            }

            if (key == 'q') {
              usr_cmd_->turning_vel_scale = AXIS_STEP;
              turnning_last_input_ms = current_time_ms;
              keyboard_turnning_active = true;
            } else if (key == 'e') {
              usr_cmd_->turning_vel_scale = -AXIS_STEP;
              turnning_last_input_ms = current_time_ms;
              keyboard_turnning_active = true;
            }
            break;
          default:
            break;
        }
      }

      if (current_state == RobotMotionState::RLControl) {
        if (keyboard_forward_active && current_time_ms - forward_last_input_ms > KEY_HOLD_TIMEOUT_MS) {
          usr_cmd_->forward_vel_scale = 0.0f;
          keyboard_forward_active = false;
        }
        if (keyboard_side_active && current_time_ms - side_last_input_ms > KEY_HOLD_TIMEOUT_MS) {
          usr_cmd_->side_vel_scale = 0.0f;
          keyboard_side_active = false;
        }
        if (keyboard_turnning_active && current_time_ms - turnning_last_input_ms > KEY_HOLD_TIMEOUT_MS) {
          usr_cmd_->turning_vel_scale = 0.0f;
          keyboard_turnning_active = false;
        }

        ClipNumber(usr_cmd_->forward_vel_scale, -1., 1.);
        ClipNumber(usr_cmd_->side_vel_scale, -1., 1.);
        ClipNumber(usr_cmd_->turning_vel_scale, -1., 1.);
      } else {
        usr_cmd_->forward_vel_scale = 0.0f;
        usr_cmd_->side_vel_scale = 0.0f;
        usr_cmd_->turning_vel_scale = 0.0f;
        keyboard_forward_active = false;
        keyboard_side_active = false;
        keyboard_turnning_active = false;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(KEYBOARD_REFRESH_MS));
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  }

  bool start_thread_flag_ = false;
  std::thread kb_thread_;
  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr gamepad_key_sub_;
  rclcpp::Subscription<drdds::msg::Steer>::SharedPtr steer_sub_;
  rclcpp::Subscription<drdds::msg::Steer>::SharedPtr real_steer_sub_;
};
}  // namespace deep_robotics::dr02_std
