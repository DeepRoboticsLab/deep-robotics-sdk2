/**
 * @file arm_joint_example.cpp
 * @brief Upper-body joint command example.
 * @author DEEPRobotics
 * @date 2026-06-27
 *
 * @copyright Copyright (c) 2026 DEEPRobotics
 *
 */
#include <algorithm>
#include <array>
#include <chrono>
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
constexpr int kUpperBodyDofNum = 9;
constexpr double kMoveDurationSec = 3.0;
constexpr auto kPublishPeriod = std::chrono::milliseconds(5);

using JointArray = std::array<float, kDofNum>;

constexpr JointArray kUpperBodyKp{
    600.0f,                                  // waist
    200.0f, 200.0f, 200.0f, 200.0f,          // left arm
    200.0f, 200.0f, 200.0f, 200.0f,          // right arm
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,     // left leg
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,     // right leg
};

constexpr JointArray kUpperBodyKd{
    6.0f,                                  // waist
    5.0f, 5.0f, 5.0f, 5.0f,               // left arm
    5.0f, 5.0f, 5.0f, 5.0f,               // right arm
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,   // left leg
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,   // right leg
};

}  // namespace

class ArmJointExample : public rclcpp::Node {
public:
    ArmJointExample() : Node("arm_joint_example") {
        joint_cmd_pub_ = create_publisher<drdds::msg::JointsCmd>("/JOINTS_CMD", 10);
        joint_data_sub_ = create_subscription<drdds::msg::Joints>(
            "/JOINTS_DATA", 10, std::bind(&ArmJointExample::HandleJointData, this, std::placeholders::_1));
        publish_timer_ = create_wall_timer(kPublishPeriod, std::bind(&ArmJointExample::PublishJointCommand, this));

        RCLCPP_WARN(get_logger(), "Publishing /JOINTS_CMD directly. Do not run the state machine at the same time.");
        RCLCPP_INFO(get_logger(), "Waiting for /JOINTS_DATA before moving waist and arms to zero position.");
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

        start_time_ = now();
        initial_state_ready_ = true;
        RCLCPP_INFO(get_logger(), "Initial joint state received. Starting %.1f s upper-body interpolation.",
                    kMoveDurationSec);
    }

    /**
     * @brief Publish the current upper-body joint command to /JOINTS_CMD.
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
            if (i < kUpperBodyDofNum && elapsed < kMoveDurationSec) {
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
            msg.data[i].kp = kUpperBodyKp[i];
            msg.data[i].kd = kUpperBodyKd[i];
        }

        joint_cmd_pub_->publish(msg);
        if (!completed_ && elapsed >= kMoveDurationSec) {
            completed_ = true;
            RCLCPP_INFO(get_logger(), "Reached upper-body zero position. Holding command until Ctrl+C.");
        }
    }

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
    if (argc != 2 || std::string(argv[1]) != "--confirm") {
        std::cerr << "Usage:\n"
                  << "  " << argv[0] << " --confirm\n\n"
                  << "WARNING: this example publishes /JOINTS_CMD directly.\n"
                  << "Do not run state_machine or another /JOINTS_CMD publisher at the same time.\n";
        return 1;
    }

    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<ArmJointExample>());
    rclcpp::shutdown();
    return 0;
}
