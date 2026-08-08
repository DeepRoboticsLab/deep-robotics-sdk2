/**
 * @file gamepad_key_example.cpp
 * @brief Gamepad key subscriber example.
 * @author DEEPRobotics
 * @date 2026-06-27
 *
 * @copyright Copyright (c) 2026 DEEPRobotics
 *
 */
#include <functional>
#include <memory>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

enum class GamepadKey { L1, L2, R1, R2, Unknown };

class GamepadKeySubscriber : public rclcpp::Node {
public:
    GamepadKeySubscriber() : Node("gamepad_key_example") {
        gamepad_key_sub_ = create_subscription<std_msgs::msg::String>(
            "/GAMEPAD_KEY", 10, std::bind(&GamepadKeySubscriber::HandleGamepadKey, this, std::placeholders::_1));
    }

private:
    /**
     * @brief Handle gamepad key messages from /GAMEPAD_KEY.
     * @param msg Received key message.
     */
    void HandleGamepadKey(const std_msgs::msg::String::SharedPtr msg) const {
        GamepadKey key = GamepadKey::Unknown;
        if (msg->data == "G20_KEY_L1") {
            key = GamepadKey::L1;
        } else if (msg->data == "G20_KEY_L2") {
            key = GamepadKey::L2;
        } else if (msg->data == "G20_KEY_R1") {
            key = GamepadKey::R1;
        } else if (msg->data == "G20_KEY_R2") {
            key = GamepadKey::R2;
        }

        switch (key) {
            case GamepadKey::L1:
                RCLCPP_INFO(get_logger(), "/GAMEPAD_KEY msg->data = G20_KEY_L1");
                break;
            case GamepadKey::L2:
                RCLCPP_INFO(get_logger(), "/GAMEPAD_KEY msg->data = G20_KEY_L2");
                break;
            case GamepadKey::R1:
                RCLCPP_INFO(get_logger(), "/GAMEPAD_KEY msg->data = G20_KEY_R1");
                break;
            case GamepadKey::R2:
                RCLCPP_INFO(get_logger(), "/GAMEPAD_KEY msg->data = G20_KEY_R2");
                break;
            default:
                RCLCPP_INFO(get_logger(),
                            "Unsupported /GAMEPAD_KEY msg->data = %s. Supported: G20_KEY_L1, G20_KEY_L2, G20_KEY_R1, "
                            "G20_KEY_R2",
                            msg->data.c_str());
                break;
        }
    }

    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr gamepad_key_sub_;
};

int main(int argc, char* argv[]) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<GamepadKeySubscriber>());
    rclcpp::shutdown();
    return 0;
}
