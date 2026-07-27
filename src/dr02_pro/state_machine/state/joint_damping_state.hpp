/**
 * @file joint_damping_state.hpp
 * @brief Joint damping state for the state machine.
 * @author DEEPRobotics
 * @date 2026-06-27
 *
 * @copyright Copyright (c) 2026 DEEPRobotics
 *
 */
#pragma once

#include "state_base.h"

namespace deep_robotics::dr02_pro {
class JointDampingState : public StateBase {
 private:
  float time_record_, run_time_;
  MatXf joint_cmd_;

 public:
  JointDampingState(const RobotName& robot_name, const std::string& state_name, std::shared_ptr<ControllerData> data_ptr)
      : StateBase(robot_name, state_name, data_ptr) {
    VecXf kd_ = VecXf::Zero(cp_ptr_->dof_num_);
    if(robot_name_ == RobotName::DR2PRO) 
      kd_<<cp_ptr_->waist_kd_, cp_ptr_->arm_kd_, cp_ptr_->arm_kd_, cp_ptr_->leg_kd_, cp_ptr_->leg_kd_, cp_ptr_->neck_kd_;

    joint_cmd_ = MatXf::Zero(cp_ptr_->dof_num_, 5);
    joint_cmd_.col(2) = kd_;
  }

  ~JointDampingState() {}

  /**
   * @brief Enter the joint damping state.
   */
  virtual void OnEnter() {
    time_record_ = ri_ptr_->GetInterfaceTimeStamp();
    run_time_ = ri_ptr_->GetInterfaceTimeStamp();
    motion_state_store_->SetState(RobotMotionState::JointDamping);
  };

  /**
   * @brief Exit the joint damping state.
   */
  virtual void OnExit() {}

  /**
   * @brief Publish the damping joint command.
   */
  virtual void Run() {
    run_time_ = ri_ptr_->GetInterfaceTimeStamp();
    ri_ptr_->SetJointCommand(joint_cmd_);
  }

  /**
   * @brief Get the next state requested from joint damping.
   * @return Next state name.
   */
  virtual StateName GetNextStateName() {
    if (run_time_ - time_record_ < 3.) {
      return StateName::kJointDamping;
    }
    return StateName::kIdle;
  }
};
}  // namespace deep_robotics::dr02_pro
