/**
 * @file action_info_example.cpp
 * @brief High-level current-action subscriber example.
 * @author DEEPRobotics
 * @date 2026-08-25
 *
 * @copyright Copyright (c) 2026 DEEPRobotics
 *
 */
#include <cstdint>
#include <functional>
#include <memory>

#include "drdds/msg/std_msg_int32.hpp"
#include "rclcpp/rclcpp.hpp"

class ActionInfoSubscriber : public rclcpp::Node {
public:
    ActionInfoSubscriber() : Node("action_info_example") {
        action_info_sub_ = create_subscription<drdds::msg::StdMsgInt32>(
            "/ACTION_INFO", 10, std::bind(&ActionInfoSubscriber::HandleActionInfo, this, std::placeholders::_1));
        RCLCPP_INFO(get_logger(), "Subscribed /ACTION_INFO. Press Ctrl+C to exit.");
    }

private:
    /**
     * @brief Handle current action information from /ACTION_INFO.
     * @param msg Received current-action message.
     */
    void HandleActionInfo(const drdds::msg::StdMsgInt32::SharedPtr msg) {
        if (received_ && msg->value == last_action_) {
            return;
        }

        received_ = true;
        last_action_ = msg->value;
        if (msg->value == 0) {
            RCLCPP_INFO(get_logger(), "No action is running.");
        } else {
            RCLCPP_INFO(get_logger(), "Action is running: 0x%x", static_cast<uint32_t>(msg->value));
        }
    }

    bool received_ = false;
    int32_t last_action_ = 0;
    rclcpp::Subscription<drdds::msg::StdMsgInt32>::SharedPtr action_info_sub_;
};

int main(int argc, char* argv[]) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<ActionInfoSubscriber>());
    rclcpp::shutdown();
    return 0;
}
