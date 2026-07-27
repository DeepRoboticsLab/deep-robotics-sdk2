/**
 * @file idle_state.hpp
 * @brief Idle state for the state machine.
 * @author DEEPRobotics
 * @date 2026-06-27
 *
 * @copyright Copyright (c) 2026 DEEPRobotics
 *
 */
#pragma once
#include "state_base.h"

namespace deep_robotics::dr02_pro {
class IdleState : public StateBase {
private:
    MatXf joint_cmd_;
public:
    IdleState(const RobotName& robot_name, const std::string& state_name, std::shared_ptr<ControllerData> data_ptr)
        : StateBase(robot_name, state_name, data_ptr) {
        joint_cmd_ = MatXf::Zero(cp_ptr_->dof_num_, 5);
      }
  ~IdleState() {}

    /**
     * @brief Enter the idle state.
     */
    virtual void OnEnter() {
        motion_state_store_->SetState(RobotMotionState::Idle);
    };

    /**
     * @brief Exit the idle state.
     */
    virtual void OnExit() { }

    /**
     * @brief Publish the idle joint command.
     */
    virtual void Run() {
        ri_ptr_->SetJointCommand(joint_cmd_);
    }

    /**
     * @brief Get the next state requested from idle.
     * @return Next state name.
     */
    virtual StateName GetNextStateName() {

        if (uc_ptr_->GetUserCommand()->safe_control_mode != 0) {
            std::cout << "safe_control_mode:" << std::dec << uc_ptr_->GetUserCommand()->safe_control_mode << std::endl;
            return StateName::kIdle;
        }

        if (uc_ptr_->GetUserCommand()->target_mode == uint8_t(RobotMotionState::ZeroPos)) {
            return StateName::kZeroPos;
        }

        return StateName::kIdle;
    }
};

}  // namespace deep_robotics::dr02_pro
