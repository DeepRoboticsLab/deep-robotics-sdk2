/**
 * @file dr2_std_interface.hpp
 * @brief Robot interface specialization.
 * @author DEEPRobotics
 * @date 2026-06-27
 *
 * @copyright Copyright (c) 2026 DEEPRobotics
 *
 */
#pragma once

#include "utils.hpp"
#include "time.hpp"
#include "robot_interface.hpp"

namespace deep_robotics::dr02_std {

/**
 * @class DR2StdInterface
 * @brief Hardware interface implementation for robots.
 */
class DR2StdInterface : public RobotInterface {
public:
    explicit DR2StdInterface(rclcpp::Node::SharedPtr node, const std::string& robot_name = "DR2STD")
        : RobotInterface(robot_name, 21, 12, 8, 1, 0, node) {
    }
    ~DR2StdInterface() {};
};

}  // namespace deep_robotics::dr02_std
