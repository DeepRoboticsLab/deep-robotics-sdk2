/**
 * @file joints_example.cpp
 * @brief Low-level joint state and joint command example.
 * @author DEEPRobotics
 * @date 2026-06-27
 *
 * @copyright Copyright (c) 2026 DEEPRobotics
 *
 */
#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <string>

#include "drdds/msg/joints.hpp"
#include "drdds/msg/joints_cmd.hpp"
#include "rclcpp/rclcpp.hpp"
#include "utils.hpp"

namespace {

constexpr int kDofNum = 21;
constexpr double kMoveDurationSec = 3.0;

using JointArray = std::array<float, kDofNum>;

constexpr JointArray kDr02Kp{
    600.0f,                                  // waist
    200.0f, 200.0f, 200.0f, 200.0f,          // left arm
    200.0f, 200.0f, 200.0f, 200.0f,          // right arm
    1800.0f, 600.0f, 600.0f, 1800.0f, 600.0f, 90.0f, // left leg
    1800.0f, 600.0f, 600.0f, 1800.0f, 600.0f, 90.0f, // right leg
};

constexpr JointArray kDr02Kd{
    6.0f,                                  // waist
    5.0f, 5.0f, 5.0f, 5.0f,               // left arm
    5.0f, 5.0f, 5.0f, 5.0f,               // right arm
    24.0f, 6.0f, 6.0f, 24.0f, 6.0f, 3.0f, // left leg
    24.0f, 6.0f, 6.0f, 24.0f, 6.0f, 3.0f, // right leg
};

/**
 * @brief Print command-line usage.
 * @param program Program name.
 */
void PrintUsage(const char* program) {
    std::cerr << "Usage:\n"
              << "  " << program << "\n"
              << "  " << program << " --confirm\n"
              << "\n"
              << "Default mode subscribes /JOINTS_DATA, prints one frame, and exits.\n"
              << "--confirm publishes /JOINTS_CMD and moves all DR02 Std joints to zero position.\n";
}

}  // namespace

class JointsExample : public rclcpp::Node {
public:
    explicit JointsExample(bool publish_cmd) : Node("joints_example"), publish_cmd_(publish_cmd) {
        joint_data_sub_ = create_subscription<drdds::msg::Joints>(
            "/JOINTS_DATA", 10, std::bind(&JointsExample::HandleJointData, this, std::placeholders::_1));

        if (publish_cmd_) {
            joint_cmd_pub_ = create_publisher<drdds::msg::JointsCmd>("/JOINTS_CMD", 10);
            publish_timer_ =
                create_wall_timer(std::chrono::milliseconds(2), std::bind(&JointsExample::PublishJointCommand, this));
        } else {
            RCLCPP_INFO(get_logger(), "Waiting for one /JOINTS_DATA frame.");
        }
    }

private:
    /**
     * @brief Handle joint-state messages from /JOINTS_DATA.
     * @param msg Received joint-state message.
     */
    void HandleJointData(const drdds::msg::Joints::SharedPtr msg) {
        if (initial_state_ready_) {
            return;
        }

        if (msg->data.size() != kDofNum) {
            RCLCPP_WARN_THROTTLE(
                get_logger(), *get_clock(), 1000, "Expected %d joints, received %zu", kDofNum, msg->data.size());
            return;
        }

        for (int i = 0; i < kDofNum; ++i) {
            initial_positions_[i] = msg->data[i].position;
            initial_velocities_[i] = msg->data[i].velocity;
        }

        RCLCPP_INFO(get_logger(), "frame=%lu joints=%zu", static_cast<unsigned long>(msg->header.frame_id),
                    msg->data.size());
        RCLCPP_INFO(get_logger(), "idx position velocity torque motor_temp driver_temp");
        for (std::size_t i = 0; i < msg->data.size(); ++i) {
            const auto& joint = msg->data[i];
            RCLCPP_INFO(get_logger(), "%zu %.6f %.6f %.6f %.1f %.1f", i, joint.position, joint.velocity,
                        joint.torque, joint.motion_temp, joint.driver_temp);
        }
        start_time_ = now();
        initial_state_ready_ = true;

        if (!publish_cmd_) {
            rclcpp::shutdown();
            return;
        }

        RCLCPP_INFO(get_logger(), "Initial joint state received. Starting %.1f s zero-position interpolation.",
                    kMoveDurationSec);
    }

    /**
     * @brief Publish the current joint command to /JOINTS_CMD.
     */
    void PublishJointCommand() {
        if (!initial_state_ready_) {
            RCLCPP_INFO_THROTTLE(get_logger(), *get_clock(), 1000, "Waiting for /JOINTS_DATA...");
            return;
        }

        const rclcpp::Time stamp = now();
        const double elapsed = std::max(0.0, (stamp - start_time_).seconds());
        drdds::msg::JointsCmd msg;
        msg.header.frame_id = frame_id_++;
        msg.header.stamp = stamp;
        msg.data.resize(kDofNum);

        for (int i = 0; i < kDofNum; ++i) {
            float target_position = 0.0f;
            float target_velocity = 0.0f;
            if (elapsed < kMoveDurationSec) {
                target_position = deep_robotics::common::GetCubicSplinePos(
                    initial_positions_[i], initial_velocities_[i], 0.0f, 0.0f, static_cast<float>(elapsed),
                    static_cast<float>(kMoveDurationSec));
                target_velocity = deep_robotics::common::GetCubicSplineVel(
                    initial_positions_[i], initial_velocities_[i], 0.0f, 0.0f, static_cast<float>(elapsed),
                    static_cast<float>(kMoveDurationSec));
            }

            msg.data[i].control_word = 4;
            msg.data[i].position = target_position;
            msg.data[i].velocity = target_velocity;
            msg.data[i].torque = 0.0f;
            msg.data[i].kp = kDr02Kp[i];
            msg.data[i].kd = kDr02Kd[i];
        }

        joint_cmd_pub_->publish(msg);
        if (!completed_ && elapsed >= kMoveDurationSec) {
            completed_ = true;
            RCLCPP_INFO(get_logger(), "Reached zero position. Holding zero command until Ctrl+C.");
        }
    }

    const bool publish_cmd_;
    JointArray initial_positions_{};
    JointArray initial_velocities_{};
    bool initial_state_ready_ = false;
    bool completed_ = false;
    uint64_t frame_id_ = 0;
    rclcpp::Time start_time_;
    rclcpp::Publisher<drdds::msg::JointsCmd>::SharedPtr joint_cmd_pub_;
    rclcpp::Subscription<drdds::msg::Joints>::SharedPtr joint_data_sub_;
    rclcpp::TimerBase::SharedPtr publish_timer_;
};

int main(int argc, char* argv[]) {
    const bool publish_cmd = argc == 2 && std::string(argv[1]) == "--confirm";
    if (argc > 2 || (argc == 2 && !publish_cmd)) {
        PrintUsage(argv[0]);
        return 1;
    }

    if (publish_cmd) {
        std::cerr << "WARNING: publishing /JOINTS_CMD directly.\n"
                  << "Do not run state_machine or another /JOINTS_CMD publisher at the same time.\n";
    }

    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<JointsExample>(publish_cmd));
    rclcpp::shutdown();
    return 0;
}
