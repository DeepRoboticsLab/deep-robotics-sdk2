/**
 * @file battery_state_example.cpp
 * @brief Battery-state subscriber example.
 * @author DEEPRobotics
 * @date 2026-06-27
 *
 * @copyright Copyright (c) 2026 DEEPRobotics
 *
 */
#include <cstddef>
#include <functional>
#include <memory>

#include "drdds/msg/battery_data.hpp"
#include "rclcpp/rclcpp.hpp"

class BatteryStateSubscriber : public rclcpp::Node {
public:
    BatteryStateSubscriber() : Node("battery_state_example") {
        battery_data_sub_ = create_subscription<drdds::msg::BatteryData>(
            "/BATTERY_DATA", 10, std::bind(&BatteryStateSubscriber::HandleBatteryData, this, std::placeholders::_1));
    }

private:
    /**
     * @brief Handle battery data messages from /BATTERY_DATA.
     * @param msg Received battery data message.
     */
    void HandleBatteryData(const drdds::msg::BatteryData::SharedPtr msg) const {
        if (msg->data.empty()) {
            RCLCPP_WARN(get_logger(), "Received empty /BATTERY_DATA");
            return;
        }

        RCLCPP_INFO(get_logger(), "battery_count=%zu", msg->data.size());
        for (std::size_t i = 0; i < msg->data.size(); ++i) {
            const auto& battery = msg->data[i];
            RCLCPP_INFO(get_logger(), "battery[%zu] battery_level=%u voltage=%u current=%d protected_state=%u", i,
                        static_cast<unsigned>(battery.battery_level), static_cast<unsigned>(battery.voltage),
                        static_cast<int>(battery.current), static_cast<unsigned>(battery.protected_state));
        }
    }

    rclcpp::Subscription<drdds::msg::BatteryData>::SharedPtr battery_data_sub_;
};

int main(int argc, char* argv[]) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<BatteryStateSubscriber>());
    rclcpp::shutdown();
    return 0;
}
