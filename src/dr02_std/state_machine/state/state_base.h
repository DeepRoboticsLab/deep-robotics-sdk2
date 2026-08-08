/**
 * @file state_base.h
 * @brief Base state interface for robot control.
 * @author DEEPRobotics
 * @date 2026-06-27
 *
 * @copyright Copyright (c) 2026 DEEPRobotics
 *
 */
#pragma once

#include <memory>
#include <string>

#include "utils.hpp"
#include "time.hpp"
#include "types.h"
#include "control_parameters.hpp"
#include "motion_state_store.hpp"
#include "robot_interface.hpp"
#include "user_command_interface.hpp"

namespace deep_robotics::dr02_std {

struct ControllerData {
    std::shared_ptr<RobotInterface> ri_ptr;
    std::shared_ptr<UserCommandInterface> uc_ptr;
    std::shared_ptr<ControlParameters> cp_ptr;
    std::shared_ptr<MotionStateStore> motion_state_store;
};

class StateBase {
private:
public:
    StateBase(const RobotName& robot_name, const std::string& state_name, std::shared_ptr<ControllerData> data_ptr)
        : robot_name_(robot_name), state_name_(state_name), data_ptr_(data_ptr) {
        ri_ptr_ = data_ptr_->ri_ptr;
        uc_ptr_ = data_ptr_->uc_ptr;
        cp_ptr_ = data_ptr_->cp_ptr;
        motion_state_store_ = data_ptr_->motion_state_store;
    }

    ~StateBase() {}

    /**
     * @brief Execute logic when entering the state.
     */
    virtual void OnEnter() = 0;

    /**
     * @brief Execute logic when exiting the state.
     */
    virtual void OnExit() = 0;

    /**
     * @brief Run one state update.
     */
    virtual void Run() = 0;

    /**
     * @brief Get the next state name.
     * @return Next state name.
     */
    virtual StateName GetNextStateName() = 0;

    /**
     * @brief state name for print
     */
    std::string state_name_;

    /**
     * @brief robot_name for print
     */
    RobotName robot_name_;

    std::shared_ptr<ControllerData> data_ptr_;      // control data pointer
    std::shared_ptr<RobotInterface> ri_ptr_;        // robot interface pointer to control your robot
    std::shared_ptr<UserCommandInterface> uc_ptr_;  // control your robot by design user command
    std::shared_ptr<ControlParameters> cp_ptr_;     // control parameters
    std::shared_ptr<MotionStateStore> motion_state_store_;
};

}  // namespace deep_robotics::dr02_std
