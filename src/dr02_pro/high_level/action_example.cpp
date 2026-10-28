/**
 * @file action_example.cpp
 * @brief High-level preset action command example.
 * @author DEEPRobotics
 * @date 2026-08-14
 *
 * @copyright Copyright (c) 2026 DEEPRobotics
 *
 */
#include <array>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <thread>

#include "drdds/msg/motion_state.hpp"
#include "rclcpp/rclcpp.hpp"

namespace {

struct ActionInfo {
    int32_t id;
    const char* name;
};

constexpr std::array<ActionInfo, 8> kActionInfos{{
    {30, "arm_wave_small"},       // Small wave.
    {31, "arm_heart"},            // Heart gesture.
    {32, "arm_clap"},             // Clap.
    {33, "arm_salute"},           // Salute.
    {34, "waist_patrol"},         // Waist patrol.
    {35, "neck_patrol"},          // Look left and right.
    {36, "neck_shake"},           // Shake the head.
    {37, "neck_nod"},             // Nod the head.
}};

/**
 * @brief Print command-line usage and all public action IDs.
 * @param program Program name.
 */
void PrintUsage(const char* program) {
    std::cout << "Available action IDs:\n"
              << "  id       name\n";
    for (const auto& action : kActionInfos) {
        std::cout << "  " << action.id << "  " << action.name << "\n";
    }
    std::cout << "\nUsage:\n"
              << "  " << program << " <action_id> --confirm\n";
}

/**
 * @brief Parse a decimal or hexadecimal action ID.
 * @param text Input command-line string.
 * @param action_id Parsed action ID.
 * @return true if the value is one of the public action IDs.
 */
bool ParseActionId(const char* text, int32_t& action_id) {
    errno = 0;
    char* end = nullptr;
    const long parsed = std::strtol(text, &end, 0);
    if (errno != 0 || end == text || *end != '\0' ||
        parsed < std::numeric_limits<int32_t>::min() ||
        parsed > std::numeric_limits<int32_t>::max()) {
        return false;
    }

    for (const auto& action : kActionInfos) {
        if (parsed == action.id) {
            action_id = action.id;
            return true;
        }
    }
    return false;
}

const ActionInfo* FindAction(int32_t action_id) {
    for (const auto& action : kActionInfos) {
        if (action.id == action_id) {
            return &action;
        }
    }
    return nullptr;
}

/**
 * @brief Wait briefly for the robot-side /ACTION_COMMAND subscriber.
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

}  // namespace

int main(int argc, char* argv[]) {
    if (argc == 1) {
        PrintUsage(argv[0]);
        return 0;
    }

    if (argc != 3 || std::string(argv[2]) != "--confirm") {
        std::cerr << "This example requires an action ID and --confirm.\n";
        PrintUsage(argv[0]);
        return 1;
    }

    int32_t action_id = 0;
    if (!ParseActionId(argv[1], action_id)) {
        std::cerr << "Unsupported action ID: " << argv[1] << "\n";
        PrintUsage(argv[0]);
        return 1;
    }

    const auto* action = FindAction(action_id);
    rclcpp::init(argc, argv);
    auto node = std::make_shared<rclcpp::Node>("action_example");
    auto publisher = node->create_publisher<drdds::msg::MotionState>("/ACTION_COMMAND", 10);

    if (!WaitForSubscriber(node, publisher)) {
        RCLCPP_ERROR(node->get_logger(), "No subscriber found on /ACTION_COMMAND within 2 seconds");
        rclcpp::shutdown();
        return 1;
    }

    drdds::msg::MotionState msg;
    msg.header.frame_id = 0;
    msg.header.stamp = node->now();
    msg.data.state = action_id;
    publisher->publish(msg);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    RCLCPP_INFO(node->get_logger(),
                "Published /ACTION_COMMAND action=%u (%s).",
                static_cast<uint32_t>(action_id), action->name);
    RCLCPP_INFO(node->get_logger(),
                "View execution results with: ros2 run dr02_pro action_status_example");
    rclcpp::shutdown();
    return 0;
}
