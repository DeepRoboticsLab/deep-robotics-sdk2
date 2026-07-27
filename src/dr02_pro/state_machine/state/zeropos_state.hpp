/**
 * @file zeropos_state.hpp
 * @brief Zero-position state for the state machine.
 * @author DEEPRobotics
 * @date 2026-06-27
 *
 * @copyright Copyright (c) 2026 DEEPRobotics
 *
 */
#pragma once
#include "state_base.h"

namespace deep_robotics::dr02_pro {
class ZeroPosState : public StateBase {
private:
    float state_enter_time_, run_time_;
    VecXf joint_pos_init_, joint_vel_init_, joint_pos_, joint_vel_;
    VecXf kp_, kd_, joint_pos_goal_;
    MatXf joint_cmd_;

    /**
     * @brief Read the latest robot joint state.
     */
    void GetRobotJointValue() {
        run_time_ = ri_ptr_->GetInterfaceTimeStamp();
        joint_pos_ = ri_ptr_->GetJointPosition();
        joint_vel_ = ri_ptr_->GetJointVelocity();
    }

    /**
     * @brief Record the joint state at state entry.
     */
    void RecordJointData() {
        state_enter_time_ = run_time_;
        joint_pos_init_ = joint_pos_;
        joint_vel_init_ = joint_vel_;
    }

public:
    ZeroPosState(const RobotName& robot_name, const std::string& state_name, std::shared_ptr<ControllerData> data_ptr)
        : StateBase(robot_name, state_name, data_ptr) {
        joint_pos_goal_ = VecXf::Zero(cp_ptr_->dof_num_);
        joint_cmd_ = MatXf::Zero(cp_ptr_->dof_num_, 5);
        kp_ = VecXf::Zero(cp_ptr_->dof_num_);
        kd_ = VecXf::Zero(cp_ptr_->dof_num_);

        if (robot_name_ == RobotName::DR2PRO) {
            kp_ << cp_ptr_->waist_kp_, cp_ptr_->arm_kp_, cp_ptr_->arm_kp_, cp_ptr_->leg_kp_, cp_ptr_->leg_kp_,
                cp_ptr_->neck_kp_;
            kd_ << cp_ptr_->waist_kd_, cp_ptr_->arm_kd_, cp_ptr_->arm_kd_, cp_ptr_->leg_kd_, cp_ptr_->leg_kd_,
                cp_ptr_->neck_kd_;
        }
    }

    ~ZeroPosState() {}

    /**
     * @brief Enter the zero-position state.
     */
    virtual void OnEnter() {
        GetRobotJointValue();
        RecordJointData();
        motion_state_store_->SetState(RobotMotionState::ZeroPos);
    };

    /**
     * @brief Exit the zero-position state.
     */
    virtual void OnExit() {}

    /**
     * @brief Run one zero-position interpolation update.
     */
    virtual void Run() {
        GetRobotJointValue();

        VecXf joint_pos_plan = VecXf::Zero(cp_ptr_->dof_num_);
        VecXf joint_vel_plan = VecXf::Zero(cp_ptr_->dof_num_);
        VecXf joint_tor_plan = VecXf::Zero(cp_ptr_->dof_num_);

        float init_time = 3.;
        if (run_time_ - state_enter_time_ <= init_time) {
            for (int i = 0; i < joint_pos_.rows(); ++i) {
                joint_pos_plan(i) = GetCubicSplinePos(joint_pos_init_(i), joint_vel_init_(i), joint_pos_goal_(i), 0,
                                                      run_time_ - state_enter_time_, init_time);
                joint_vel_plan(i) = GetCubicSplineVel(joint_pos_init_(i), joint_vel_init_(i), joint_pos_goal_(i), 0,
                                                      run_time_ - state_enter_time_, init_time);
            }
        } else {
            for (int i = 0; i < joint_pos_.rows(); ++i) {
                joint_pos_plan(i) = joint_pos_goal_(i);
                joint_vel_plan.setZero();
            }
        }

        joint_cmd_.col(0) = kp_;
        joint_cmd_.col(2) = kd_;

        joint_cmd_.col(1) = joint_pos_plan;
        joint_cmd_.col(3) = joint_vel_plan;
        joint_cmd_.col(4) = joint_tor_plan;
        ri_ptr_->SetJointCommand(joint_cmd_);
    }

    /**
     * @brief Get the next state requested from zero position.
     * @return Next state name.
     */
    virtual StateName GetNextStateName() {
        if (uc_ptr_->GetUserCommand()->safe_control_mode != 0 ||
            uc_ptr_->GetUserCommand()->target_mode == uint8_t(RobotMotionState::JointDamping)) {
            std::cout << "safe_control_mode:" << std::dec << uc_ptr_->GetUserCommand()->safe_control_mode << std::endl;
            return StateName::kJointDamping;
        }

        if (uc_ptr_->GetUserCommand()->target_mode == uint8_t(RobotMotionState::RLControl)) {
            std::cout << "go to rl control" << std::endl;
            return StateName::kRLControl;
        }
        return StateName::kZeroPos;
    }
};

}  // namespace deep_robotics::dr02_pro
