/**
 * @file robot_interface.hpp
 * @brief Robot data and joint-command interface for the state machine.
 * @author DEEPRobotics
 * @date 2026-06-27
 *
 * @copyright Copyright (c) 2026 DEEPRobotics
 *
 */
#pragma once

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#include <unistd.h>

#include <atomic>
#include <cstring>
#include <functional>

#include "utils.hpp"
#include "time.hpp"
#include "types.h"
#include "drdds/msg/joints.hpp"
#include "drdds/msg/joints_cmd.hpp"
#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/imu.hpp"

namespace deep_robotics::dr02_std {
class RobotInterface {
protected:
    // Timing information
    double ri_ts_;   // Robot interface timestamp (seconds)
    double imu_base_ts_;  // IMU timestamp (milliseconds)

    // Sensor data
    Vec3f omega_base_;  // Angular velocity from IMU (rad/s)
    Vec3f rpy_base_;    // Roll-Pitch-Yaw from IMU (radians)
    Vec3f acc_base_;    // Linear acceleration from IMU (m/s²)

    // Joint data
    VecXf joint_pos_;           // Joint positions (rad)
    VecXf joint_vel_;           // Joint velocities (rad/s)
    VecXf joint_tau_;           // Joint torques (Nm)
    VecXf motor_temperature_;   // Motor temperatures (°C)
    VecXf driver_temperature_;  // Driver temperatures (°C)

    // Status monitoring
    std::vector<uint16_t> driver_status_;  // Joint driver status words
    std::vector<uint16_t> joint_data_id_;  // Joint data packet IDs

    rclcpp::Publisher<drdds::msg::JointsCmd>::SharedPtr joint_cmd_pub_;
    rclcpp::Subscription<drdds::msg::Joints>::SharedPtr joint_data_sub_;
    rclcpp::Subscription<sensor_msgs::msg::Imu>::SharedPtr imu_data_base_sub_;

    int run_cnt_ = 0;  // Operation cycle counter

public:
    /**
     * @brief Construct a robot interface.
     * @param robot_name Robot name.
     * @param dof_num Total robot degree-of-freedom count.
     * @param node ROS2 node used to create publishers and subscribers.
     */
    RobotInterface(const std::string& robot_name, int dof_num, int leg_dof_num, int arm_dof_num, int waist_dof_num,
                   int neck_dof_num, rclcpp::Node::SharedPtr node)
        : robot_name_(robot_name),
          dof_num_(dof_num),
          leg_dof_num_(leg_dof_num),
          arm_dof_num_(arm_dof_num),
          waist_dof_num_(waist_dof_num),
          neck_dof_num_(neck_dof_num),
          joint_cmd_(Eigen::Matrix<float, Eigen::Dynamic, 5>::Zero(dof_num_, 5)),
          start_flag_(true),
          node_(node) {
        std::cout << robot_name << std::endl;
        std::cout << "Total DOF: " << dof_num_ << std::endl;
        std::cout << "  Leg DOF: " << leg_dof_num_ << std::endl;
        std::cout << "  Arm DOF: " << arm_dof_num_ << std::endl;
        std::cout << "Waist DOF: " << waist_dof_num_ << std::endl;
        std::cout << " Neck DOF: " << neck_dof_num_ << std::endl;

        if (!node_) {
            std::cerr << "RobotInterface node is null" << std::endl;
        }

        joint_pos_ = VecXf::Zero(dof_num_);
        joint_vel_ = VecXf::Zero(dof_num_);
        joint_tau_ = VecXf::Zero(dof_num_);
        motor_temperature_ = VecXf::Zero(dof_num_);
        driver_temperature_ = VecXf::Zero(dof_num_);

        driver_status_.resize(dof_num_);
        joint_data_id_.resize(dof_num_);
    }

    virtual ~RobotInterface() {
    };

    std::string robot_name_;
    const int dof_num_, leg_dof_num_, arm_dof_num_, waist_dof_num_, neck_dof_num_;
    Eigen::Matrix<float, Eigen::Dynamic, 5> joint_cmd_;
    std::atomic<bool> start_flag_;
    rclcpp::Node::SharedPtr node_;

    /**
     * @brief Start joint, IMU, and command topic interfaces.
     */
    virtual void Start() {
        joint_cmd_pub_ = node_->create_publisher<drdds::msg::JointsCmd>("/JOINTS_CMD", 10);
        joint_data_sub_ = node_->create_subscription<drdds::msg::Joints>(
            "/JOINTS_DATA", 10, std::bind(&RobotInterface::HandleJointData, this, std::placeholders::_1));
        imu_data_base_sub_ = node_->create_subscription<sensor_msgs::msg::Imu>(
            "/IMU_DATA_BASE", 10, std::bind(&RobotInterface::HandleBaseImuData, this, std::placeholders::_1));
        WaitDataUpdate();
    }

    /**
     * @brief Stop the robot interface.
     */
    virtual void Stop() {}

    /**
     * @brief Get the robot interface timestamp.
     * @return Interface timestamp in seconds.
     */
    virtual double GetInterfaceTimeStamp() { return ri_ts_; }

    /**
     * @brief Get current joint positions.
     * @return Joint position vector.
     */
    virtual VecXf GetJointPosition() { return joint_pos_; }

    /**
     * @brief Get current joint velocities.
     * @return Joint velocity vector.
     */
    virtual VecXf GetJointVelocity() { return joint_vel_; }

    /**
     * @brief Get current joint torques.
     * @return Joint torque vector.
     */
    virtual VecXf GetJointTorque() { return joint_tau_; }

    /**
     * @brief Get current base IMU roll-pitch-yaw data.
     * @return Base IMU roll-pitch-yaw vector.
     */
    virtual Vec3f GetImuBaseRpy() { return rpy_base_; }

    /**
     * @brief Get current base IMU acceleration data.
     * @return Base IMU acceleration vector.
     */
    virtual Vec3f GetImuBaseAcc() { return acc_base_; }

    /**
     * @brief Get current base IMU angular velocity data.
     * @return Base IMU angular velocity vector.
     */
    virtual Vec3f GetImuBaseOmega() { return omega_base_; }

    /**
     * @brief Publish a joint command matrix.
     * @param input Joint command matrix with columns for kp, position, kd, velocity, and feedforward torque.
     */
    virtual void SetJointCommand(Eigen::Matrix<float, Eigen::Dynamic, 5> input) {
        joint_cmd_ = input;
        auto msg = drdds::msg::JointsCmd();
        msg.header.stamp = node_->now();
        msg.data.resize(dof_num_);

        for (int i = 0; i < dof_num_; ++i) {
            msg.data[i].position = input(i, 1);
            msg.data[i].velocity = input(i, 3);
            msg.data[i].torque = input(i, 4);
            msg.data[i].kp = input(i, 0);
            msg.data[i].kd = input(i, 2);
            msg.data[i].control_word = kIndexMotorControl;
        }

        joint_cmd_pub_->publish(msg);
    }

    /**
     * @brief Get the last joint command matrix.
     * @return Joint command matrix.
     */
    virtual MatXf GetJointCommand() { return joint_cmd_; }

    /**
     * @brief Get contact force data.
     * @return Contact force vector.
     */
    virtual VecXf GetContactForce() { return VecXf::Zero(4); }

    /**
     * @brief Get current motor temperatures.
     * @return Motor temperature vector.
     */
    virtual VecXf GetMotorTemperature() { return motor_temperature_; }

    /**
     * @brief Get current driver temperatures.
     * @return Driver temperature vector.
     */
    virtual VecXf GetDriverTemperature() { return driver_temperature_; }

    /**
     * @brief Get the base IMU timestamp.
     * @return Base IMU timestamp.
     */
    virtual double GetImuBaseTimestamp() { return imu_base_ts_; }

    /**
     * @brief Get current driver status words.
     * @return Driver status word vector.
     */
    virtual std::vector<uint16_t> GetDriverStatusWord() { return driver_status_; }

    /**
     * @brief Get current joint data IDs.
     * @return Joint data ID vector.
     */
    virtual std::vector<uint16_t> GetJointDataID() { return joint_data_id_; }

    /**
     * @brief Refresh robot data before running state logic.
     */
    virtual void RefreshRobotData() {}

    /**
     * @brief Wait until the first joint-state frame is received.
     */
    virtual void WaitDataUpdate() {
        int cnt = 0;
        std::cout << "wait data update" << std::endl;
        while (rclcpp::ok()) {
            ++cnt;
            rclcpp::spin_some(node_);
            if (run_cnt_ > 0) {
                std::cout << "data updated at " << cnt << " cnt!" << std::endl;
                break;
            }
            usleep(1000);
            if (cnt == 10000) {
                std::cout << "joint data update is not finished\n";
            }
        }
    }

    /**
     * @brief Handle joint-state messages from /JOINTS_DATA.
     * @param msg Received joint-state message.
     */
    virtual void HandleJointData(const drdds::msg::Joints::SharedPtr msg) {
        ++run_cnt_;
        for (int i = 0; i < dof_num_; ++i) {
            joint_pos_(i) = msg->data[i].position;
            joint_vel_(i) = msg->data[i].velocity;
            joint_tau_(i) = msg->data[i].torque;
            motor_temperature_(i) = float(msg->data[i].motion_temp);
            driver_temperature_(i) = float(msg->data[i].driver_temp);
            driver_status_[i] = msg->data[i].status_word;
            joint_data_id_[i] = uint16_t(run_cnt_);
        }

        ri_ts_ = GetTimestampMs() / 1000.;
    }

    /**
     * @brief Convert an IMU orientation quaternion to ZYX roll-pitch-yaw angles.
     * @param msg Received IMU message.
     * @return Roll-pitch-yaw vector in radians.
     */
    static Vec3f QuaternionToRpy(const sensor_msgs::msg::Imu& msg) {
        const double qx = msg.orientation.x;
        const double qy = msg.orientation.y;
        const double qz = msg.orientation.z;
        const double qw = msg.orientation.w;

        const double sinr_cosp = 2.0 * (qw * qx + qy * qz);
        const double cosr_cosp = 1.0 - 2.0 * (qx * qx + qy * qy);
        const double roll = std::atan2(sinr_cosp, cosr_cosp);

        const double sinp = 2.0 * (qw * qy - qz * qx);
        const double pitch = std::abs(sinp) >= 1.0 ? std::copysign(M_PI / 2.0, sinp) : std::asin(sinp);

        const double siny_cosp = 2.0 * (qw * qz + qx * qy);
        const double cosy_cosp = 1.0 - 2.0 * (qy * qy + qz * qz);
        const double yaw = std::atan2(siny_cosp, cosy_cosp);

        return Vec3f(static_cast<float>(roll), static_cast<float>(pitch), static_cast<float>(yaw));
    }

    /**
     * @brief Handle base IMU messages from /IMU_DATA_BASE.
     * @param msg Received IMU message.
     */
    virtual void HandleBaseImuData(const sensor_msgs::msg::Imu::SharedPtr msg) {
        rpy_base_ = QuaternionToRpy(*msg);
        acc_base_ << msg->linear_acceleration.x, msg->linear_acceleration.y, msg->linear_acceleration.z;
        omega_base_ << msg->angular_velocity.x, msg->angular_velocity.y, msg->angular_velocity.z;
        imu_base_ts_ = GetTimestampMs();
    }

};

}  // namespace deep_robotics::dr02_std
