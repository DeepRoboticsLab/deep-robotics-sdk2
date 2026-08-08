/**
 * @file imu_example.cpp
 * @brief IMU subscriber example.
 * @author DEEPRobotics
 * @date 2026-06-27
 *
 * @copyright Copyright (c) 2026 DEEPRobotics
 *
 */
#include <functional>
#include <memory>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/imu.hpp"

class ImuSubscriber : public rclcpp::Node {
public:
    ImuSubscriber() : Node("imu_example") {
        base_imu_sub_ = create_subscription<sensor_msgs::msg::Imu>(
            "/IMU_DATA_BASE", 10, std::bind(&ImuSubscriber::HandleBaseImuData, this, std::placeholders::_1));
    }

private:
    /**
     * @brief Handle base IMU messages from /IMU_DATA_BASE.
     * @param msg Received IMU message.
     */
    void HandleBaseImuData(const sensor_msgs::msg::Imu::SharedPtr msg) {
        PrintImu("base", "/IMU_DATA_BASE", msg);
    }

    /**
     * @brief Print an IMU message.
     * @param name Display name for the IMU source.
     * @param topic Topic name.
     * @param msg Received IMU message.
     */
    void PrintImu(const char* name, const char* topic, const sensor_msgs::msg::Imu::SharedPtr& msg) const {
        RCLCPP_INFO(get_logger(),
                    "%s %s frame_id=%s orientation=(%.4f, %.4f, %.4f, %.4f) "
                    "angular_velocity=(%.4f, %.4f, %.4f) linear_acceleration=(%.4f, %.4f, %.4f)",
                    name, topic, msg->header.frame_id.c_str(), msg->orientation.x, msg->orientation.y,
                    msg->orientation.z, msg->orientation.w, msg->angular_velocity.x, msg->angular_velocity.y,
                    msg->angular_velocity.z, msg->linear_acceleration.x, msg->linear_acceleration.y,
                    msg->linear_acceleration.z);
    }

    rclcpp::Subscription<sensor_msgs::msg::Imu>::SharedPtr base_imu_sub_;
};

int main(int argc, char* argv[]) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<ImuSubscriber>());
    rclcpp::shutdown();
    return 0;
}
