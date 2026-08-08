/**
 * @file audio_volume_example.cpp
 * @brief Audio volume command example.
 * @author DEEPRobotics
 * @date 2026-06-27
 *
 * @copyright Copyright (c) 2026 DEEPRobotics
 *
 */
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "drdds/msg/std_msg_int32.hpp"
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

/**
 * @brief Print command-line usage.
 * @param prog Program name.
 */
void PrintUsage(const char* prog) {
    std::cout << "Usage:\n"
              << "  " << prog << " <0-100>\n\n"
              << "Example:\n"
              << "  " << prog << " 80\n";
}

/**
 * @brief Parse an audio volume percentage from text.
 * @param text Input command-line string.
 * @param volume Parsed volume percentage.
 * @return true if parsing succeeds; false otherwise.
 */
bool ParseVolume(const char* text, int& volume) {
    if (!text || !*text) return false;
    char* end = nullptr;
    const long value = std::strtol(text, &end, 10);
    if (*end != '\0' || value < 0 || value > 100) return false;
    volume = static_cast<int>(value);
    return true;
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc != 2) {
        PrintUsage(argv[0]);
        return 1;
    }

    int volume = 0;
    if (!ParseVolume(argv[1], volume)) {
        std::cerr << "volume must be an integer in range 0~100\n";
        return 1;
    }

    rclcpp::init(argc, argv);
    auto node = std::make_shared<rclcpp::Node>("audio_volume_example");
    auto publisher = node->create_publisher<drdds::msg::StdMsgInt32>("/AUDIO/VOLUME_CMD", 10);

    if (!WaitForSubscriber(node, publisher)) {
        RCLCPP_ERROR(node->get_logger(), "No subscriber found on /AUDIO/VOLUME_CMD within 2 seconds");
        rclcpp::shutdown();
        return 1;
    }

    drdds::msg::StdMsgInt32 msg;
    msg.value = volume;
    publisher->publish(msg);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    RCLCPP_INFO(node->get_logger(), "Published /AUDIO/VOLUME_CMD volume=%d", volume);
    rclcpp::shutdown();
    return 0;
}
