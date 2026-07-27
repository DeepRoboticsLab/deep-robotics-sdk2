/**
 * @file audio_play_example.cpp
 * @brief Audio playback topic example.
 * @author DEEPRobotics
 * @date 2026-06-27
 *
 * @copyright Copyright (c) 2026 DEEPRobotics
 *
 */
#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

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
 * @param program Program name.
 */
void PrintUsage(const char* program) {
    std::cout << "Usage:\n"
              << "  " << program << " test.wav\n\n"
              << "Publishes /AUDIO/PLAY_FILE with the remote WAV file name.\n";
}

/**
 * @brief Check whether an audio file name is safe to publish.
 * @param file_name Audio file name.
 * @return true if the file name is safe; false otherwise.
 */
bool IsSafeAudioFileName(const std::string& file_name) {
    if (file_name.empty()) return false;
    if (file_name.find('/') != std::string::npos) return false;
    if (file_name.find('\\') != std::string::npos) return false;
    if (file_name.find("..") != std::string::npos) return false;
    return true;
}

/**
 * @brief Check whether a file name has a WAV extension.
 * @param file_name Audio file name.
 * @return true if the file name ends with a WAV extension; false otherwise.
 */
bool HasWavExtension(const std::string& file_name) {
    if (file_name.size() < 4) return false;
    const std::string ext = file_name.substr(file_name.size() - 4);
    return ext == ".wav" || ext == ".WAV";
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc != 2) {
        PrintUsage(argv[0]);
        return 1;
    }

    const std::string file_name = argv[1];
    if (!IsSafeAudioFileName(file_name) || !HasWavExtension(file_name)) {
        std::cerr << "invalid wav file name: " << file_name << "\n";
        return 1;
    }

    rclcpp::init(argc, argv);
    auto node = std::make_shared<rclcpp::Node>("audio_play_example");
    auto publisher = node->create_publisher<std_msgs::msg::String>("/AUDIO/PLAY_FILE", 10);

    if (!WaitForSubscriber(node, publisher)) {
        RCLCPP_ERROR(node->get_logger(), "No subscriber found on /AUDIO/PLAY_FILE within 2 seconds");
        rclcpp::shutdown();
        return 1;
    }

    std_msgs::msg::String msg;
    msg.data = file_name;
    publisher->publish(msg);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    RCLCPP_INFO(node->get_logger(), "Published /AUDIO/PLAY_FILE file=%s", file_name.c_str());
    rclcpp::shutdown();
    return 0;
}
