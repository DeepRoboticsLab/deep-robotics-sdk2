/**
 * @file dr2_pro_interface.hpp
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

namespace deep_robotics::dr02_pro {

/**
 * @class DR2ProInterface
 * @brief Hardware interface implementation for robots.
 */
class DR2ProInterface : public RobotInterface {
public:
    explicit DR2ProInterface(rclcpp::Node::SharedPtr node, const std::string& robot_name = "DR2PRO") 
        : RobotInterface(robot_name, 31, 12, 14, 3, 2, node) {
    }
    ~DR2ProInterface() {};
};

}  // namespace deep_robotics::dr02_pro
