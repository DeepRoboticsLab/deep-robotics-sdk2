/**
 * @file gait_example.cpp
 * @brief High-level gait command example.
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

#include "drdds/msg/gait.hpp"
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

enum RobotGaitType{
    HumanWALKAMP = 0x21001,        // Humanoid walking.
    HumanWALKTERRAIN  = 0x21006,   // Complex terrain walking.
};

/**
 * @brief Print command-line usage.
 * @param program Program name.
 */
void PrintUsage(const char* program) {
    std::cerr << "Usage:\n"
              << "  " << program << " <gait_value>\n"
              << "\n"
              << "Supported gait values:\n"
              << "  0x21001  HumanWALKAMP\n"
              << "  0x21006  HumanWALKTERRAIN\n";
}

/**
 * @brief Parse a gait value from text.
 * @param text Input command-line string.
 * @param value Parsed gait value.
 * @return true if parsing succeeds; false otherwise.
 */
bool ParseGait(const char* text, uint32_t& value) {
    if (text[0] == '-') {
        return false;
    }
    errno = 0;
    char* end = nullptr;
    const unsigned long parsed = std::strtoul(text, &end, 0);
    if (errno != 0 || end == text || *end != '\0' || parsed > std::numeric_limits<uint32_t>::max()) {
        return false;
    }
    value = static_cast<uint32_t>(parsed);
    return true;
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc != 2) {
        PrintUsage(argv[0]);
        return 1;
    }

    uint32_t gait = 0;
    if (!ParseGait(argv[1], gait)) {
        std::cerr << "Invalid gait value: " << argv[1] << "\n";
        PrintUsage(argv[0]);
        return 1;
    }
    switch (gait) {
        case HumanWALKAMP:
        case HumanWALKTERRAIN:
            break;
        default:
            std::cerr << "Unsupported gait value: " << argv[1] << "\n";
            PrintUsage(argv[0]);
            return 1;
    }

    rclcpp::init(argc, argv);
    auto node = std::make_shared<rclcpp::Node>("gait_example");
    auto publisher = node->create_publisher<drdds::msg::Gait>("/GAIT", 10);

    if (!WaitForSubscriber(node, publisher)) {
        RCLCPP_ERROR(node->get_logger(), "No subscriber found on /GAIT within 2 seconds");
        rclcpp::shutdown();
        return 1;
    }

    drdds::msg::Gait msg;
    msg.header.frame_id = 0;
    msg.header.stamp = node->now();
    msg.data.gait = gait;
    publisher->publish(msg);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    RCLCPP_INFO(node->get_logger(), "Published /GAIT gait=0x%x", gait);
    rclcpp::shutdown();
    return 0;
}
