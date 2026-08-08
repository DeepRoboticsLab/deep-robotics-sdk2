/**
 * @file motion_state_example.cpp
 * @brief High-level motion-state command example.
 * @author DEEPRobotics
 * @date 2026-06-27
 *
 * @copyright Copyright (c) 2026 DEEPRobotics
 *
 */
#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <limits>
#include <memory>
#include <thread>

#include "drdds/msg/motion_state.hpp"
#include "rclcpp/rclcpp.hpp"

namespace {

/**
 * @brief Wait briefly for a matching subscriber.
 * @param node ROS node that owns the publisher.
 * @param publisher Publisher to check for subscriptions.
 * @return true if a subscriber is discovered; false on timeout or shutdown.
 */
bool WaitForSubscriber(const rclcpp::Node::SharedPtr& node,
                       const std::shared_ptr<rclcpp::PublisherBase>& publisher) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (rclcpp::ok() && publisher->get_subscription_count() == 0 &&
           std::chrono::steady_clock::now() < deadline) {
        rclcpp::spin_some(node);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    return publisher->get_subscription_count() > 0;
}

enum RobotMotionState{
    Idle            = 0x0,  // Zero torque, idle state.
    JointDamping    = 0x2,  // Soft emergency stop, damping state.
    RLControl       = 0x11, // RL control state.
    SuspendedStand  = 0x20006,    // Suspended stand completed; RL mode can be entered.
};

/**
 * @brief Print command-line usage.
 * @param program Program name.
 */
void PrintUsage(const char* program) {
    std::cerr << "Usage:\n"
              << "  " << program << " <motion_state_value>\n"
              << "\n"
              << "Supported motion state values:\n"
              << "  0x0      Idle\n"
              << "  0x2      JointDamping\n"
              << "  0x11     RLControl\n"
              << "  0x20006  SuspendedStand\n";
}

/**
 * @brief Parse a motion-state value from text.
 * @param text Input command-line string.
 * @param value Parsed motion-state value.
 * @return true if parsing succeeds; false otherwise.
 */
bool ParseMotionState(const char* text, int32_t& value) {
    errno = 0;
    char* end = nullptr;
    const long parsed = std::strtol(text, &end, 0);
    if (errno != 0 || end == text || *end != '\0' || parsed < std::numeric_limits<int32_t>::min() ||
        parsed > std::numeric_limits<int32_t>::max()) {
        return false;
    }
    value = static_cast<int32_t>(parsed);
    return true;
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc != 2) {
        PrintUsage(argv[0]);
        return 1;
    }

    int32_t state = 0;
    if (!ParseMotionState(argv[1], state)) {
        std::cerr << "Invalid motion state value: " << argv[1] << "\n";
        PrintUsage(argv[0]);
        return 1;
    }
    switch (state) {
        case Idle:
        case JointDamping:
        case RLControl:
        case SuspendedStand:
            break;
        default:
            std::cerr << "Unsupported motion state value: " << argv[1] << "\n";
            PrintUsage(argv[0]);
            return 1;
    }

    rclcpp::init(argc, argv);
    auto node = std::make_shared<rclcpp::Node>("motion_state_example");
    auto publisher = node->create_publisher<drdds::msg::MotionState>("/MOTION_STATE", 10);

    if (!WaitForSubscriber(node, publisher)) {
        RCLCPP_ERROR(node->get_logger(), "No subscriber found on /MOTION_STATE within 2 seconds");
        rclcpp::shutdown();
        return 1;
    }

    drdds::msg::MotionState msg;
    msg.header.frame_id = 0;
    msg.header.stamp = node->now();
    msg.data.state = state;
    publisher->publish(msg);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    RCLCPP_INFO(node->get_logger(), "Published /MOTION_STATE state=0x%x", state);
    rclcpp::shutdown();
    return 0;
}
