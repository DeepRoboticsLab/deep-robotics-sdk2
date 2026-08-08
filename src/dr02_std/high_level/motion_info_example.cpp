/**
 * @file motion_info_example.cpp
 * @brief High-level motion-info subscriber example.
 * @author DEEPRobotics
 * @date 2026-06-27
 *
 * @copyright Copyright (c) 2026 DEEPRobotics
 *
 */
#include <functional>
#include <iostream>
#include <memory>
#include <string>

#include "drdds/msg/motion_info.hpp"
#include "rclcpp/rclcpp.hpp"

namespace {

/**
 * @brief Print command-line usage.
 * @param program Program name.
 */
void PrintUsage(const char* program) {
    std::cerr << "Usage:\n"
              << "  " << program << " [--once]\n";
}

}  // namespace

class MotionInfoSubscriber : public rclcpp::Node {
public:
    explicit MotionInfoSubscriber(bool once) : Node("motion_info_example"), once_(once) {
        motion_info_sub_ = create_subscription<drdds::msg::MotionInfo>(
            "/MOTION_INFO", 10, std::bind(&MotionInfoSubscriber::HandleMotionInfo, this, std::placeholders::_1));
        RCLCPP_INFO(get_logger(), "Subscribed /MOTION_INFO. Press Ctrl+C to exit.");
    }

private:
    /**
     * @brief Handle motion-info messages from /MOTION_INFO.
     * @param msg Received motion-info message.
     */
    void HandleMotionInfo(const drdds::msg::MotionInfo::SharedPtr msg) {
        RCLCPP_INFO(get_logger(), "motion_state=0x%x gait=0x%x", msg->data.motion_state.state,
                    msg->data.gait_state.gait);
        if (once_) {
            rclcpp::shutdown();
        }
    }

    bool once_ = false;
    rclcpp::Subscription<drdds::msg::MotionInfo>::SharedPtr motion_info_sub_;
};

int main(int argc, char* argv[]) {
    if (argc > 2 || (argc == 2 && std::string(argv[1]) != "--once")) {
        PrintUsage(argv[0]);
        return 1;
    }

    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<MotionInfoSubscriber>(argc == 2));
    rclcpp::shutdown();
    return 0;
}
