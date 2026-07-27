/**
 * @file policy_runner_base.hpp
 * @brief Base interface for policy runners.
 * @author DEEPRobotics
 * @date 2026-06-27
 * 
 * @copyright Copyright (c) 2026 DEEPRobotics
 * 
 */

#pragma once

#include <string>

#include "types.h"
#include "utils.hpp"
#include "time.hpp"
#include "onnxruntime_cxx_api.h"

namespace deep_robotics::dr02_pro {

class PolicyRunnerBase{
public:
    PolicyRunnerBase(std::string policy_name, RobotName robot_name) : policy_name_(policy_name), robot_name_(robot_name)  {
        vel_delta_const_ << 0.3, 0.2, 0.3;
        cmd_vel_input_.setZero();
    }
    virtual ~PolicyRunnerBase(){}
    /**
     * @brief Display policy information.
     */
    virtual void DisplayPolicyInfo() = 0;

    /**
     * @brief Run policy inference for the current robot state and user command.
     * @return Robot action.
     */
    virtual RobotAction GetRobotAction(const RobotBasicState&, const UserCommand&) = 0;

    /**
     * @brief Execute logic when entering the policy runner.
     */
    virtual void OnEnter() = 0;

    /**
     * @brief Execute logic when exiting the policy runner.
     */
    virtual void OnExit() = 0;

    /**
     * @brief Set policy decimation.
     * @param d Decimation value.
     */
    virtual void SetDecimation(int d){
        decimation_ = d;
    }

    const std::string policy_name_;
    const RobotName robot_name_;

    int decimation_;
    int run_cnt_;
    int run_cnt_record_;
    Vec3f vel_delta_const_, cmd_vel_input_;
};

}  // namespace deep_robotics::dr02_pro
