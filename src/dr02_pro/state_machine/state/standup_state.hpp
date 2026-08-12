/**
 * @file standup_state.hpp
 * @brief StandUp transition state: smooth interpolation to default standing pose.
 * @author DEEPRobotics
 * @date 2026-08-04
 *
 * @copyright Copyright (c) 2026 DEEPRobotics
 *
 * StandUp is a pure transition state — it never stays. After 3 seconds of
 * cubic-spline interpolation to the default standing pose, it auto-switches
 * to the target RLControl state specified by target_mode.
 *
 * Flow:
 *   ZeroPos --c/v--> StandUp --3s--> RLControlAMP / RLControlMimic
 *   RLControlMimic --c/auto--> StandUp --3s--> RLControlAMP
 *   RLControlAMP --v--> StandUp --3s--> RLControlMimic
 */
#pragma once
#include "state_base.h"

namespace deep_robotics::dr02_pro {

class StandUpState : public StateBase {
private:
    float state_enter_time_, run_time_;
    VecXf joint_pos_init_, joint_vel_init_, joint_pos_, joint_vel_;
    VecXf kp_, kd_, joint_pos_goal_;
    MatXf joint_cmd_;
    float stand_duration_ = 1.5;

    void GetRobotJointValue() {
        run_time_ = ri_ptr_->GetInterfaceTimeStamp();
        joint_pos_ = ri_ptr_->GetJointPosition();
        joint_vel_ = ri_ptr_->GetJointVelocity();
    }

    void RecordJointData() {
        state_enter_time_ = run_time_;
        joint_pos_init_ = joint_pos_;
        joint_vel_init_ = joint_vel_;
    }

public:
    StandUpState(const RobotName& robot_name, const std::string& state_name,
                 std::shared_ptr<ControllerData> data_ptr)
        : StateBase(robot_name, state_name, data_ptr) {
        joint_pos_goal_ = VecXf::Zero(cp_ptr_->dof_num_);
        joint_cmd_ = MatXf::Zero(cp_ptr_->dof_num_, 5);
        kp_ = VecXf::Zero(cp_ptr_->dof_num_);
        kd_ = VecXf::Zero(cp_ptr_->dof_num_);

        if (robot_name_ == RobotName::DR2PRO) {
            // PD gains in robot_order (hardware): waist(3), arm(7)×2, leg(6)×2, neck(2)
            kp_ << 600., 2800., 2300.,                                      // waist
                600., 600., 600., 600., 90., 90., 90.,                    // left arm
                600., 600., 600., 600., 90., 90., 90.,                    // right arm
                1800., 600., 600., 1800., 600., 90.,                         // left leg
                1800., 600., 600., 1800., 600., 90.,                         // right leg
                0., 0.;                                                      // neck (locked)

            kd_ << 6., 15., 20.,                                           // waist
                6., 6., 6., 6., 2., 2., 2.,                               // left arm
                6., 6., 6., 6., 2., 2., 2.,                               // right arm
                24., 6., 6., 24., 6., 2.,                               // left leg
                24., 6., 6., 24., 6., 2.,                               // right leg
                0., 0.;                                                      // neck (locked)



            joint_pos_goal_ <<  0., 0., 0.,
                                0.,  0.15, 0., 1.35, 0., 0., 0.,
                                0., -0.15, 0., 1.35, 0., 0., 0.,
                                -0.1, 0., 0., 0.2, -0.1, 0.,
                                -0.1, 0., 0., 0.2, -0.1, 0.,
                                0., 0.;                                // neck
        }
    }

    ~StandUpState() {}

    virtual void OnEnter() {
        GetRobotJointValue();
        RecordJointData();
        motion_state_store_->SetState(RobotMotionState::StandUp);
        std::cout << "[standup] entering, target standing pose" << std::endl;
    };

    virtual void OnExit() {}

    virtual void Run() {
        GetRobotJointValue();

        VecXf joint_pos_plan = VecXf::Zero(cp_ptr_->dof_num_);
        VecXf joint_vel_plan = VecXf::Zero(cp_ptr_->dof_num_);
        VecXf joint_tor_plan = VecXf::Zero(cp_ptr_->dof_num_);

        float t = run_time_ - state_enter_time_;
        if (t <= stand_duration_) {
            for (int i = 0; i < joint_pos_.rows(); ++i) {
                joint_pos_plan(i) = GetCubicSplinePos(
                    joint_pos_init_(i), joint_vel_init_(i),
                    joint_pos_goal_(i), 0, t, stand_duration_);
                joint_vel_plan(i) = GetCubicSplineVel(
                    joint_pos_init_(i), joint_vel_init_(i),
                    joint_pos_goal_(i), 0, t, stand_duration_);
            }
        } else {
            joint_pos_plan = joint_pos_goal_;
            joint_vel_plan.setZero();
        }

        joint_cmd_.col(0) = kp_;
        joint_cmd_.col(2) = kd_;
        joint_cmd_.col(1) = joint_pos_plan;
        joint_cmd_.col(3) = joint_vel_plan;
        joint_cmd_.col(4) = joint_tor_plan;

        ri_ptr_->SetJointCommand(joint_cmd_);
    }

    virtual StateName GetNextStateName() {
        if (uc_ptr_->GetUserCommand()->safe_control_mode != 0 ||
            uc_ptr_->GetUserCommand()->target_mode == uint8_t(RobotMotionState::JointDamping)) {
            return StateName::kJointDamping;
        }

        // Wait for 3-second transition to complete
        float t = run_time_ - state_enter_time_;
        if (t < stand_duration_) {
            return StateName::kStandUp;
        }

        // Auto-switch to target state
        uint8_t target = uc_ptr_->GetUserCommand()->target_mode;
        if (target == uint8_t(RobotMotionState::RLControlAMP)) {
            std::cout << "[standup] -> RLControlAMP" << std::endl;
            return StateName::kRLControlAMP;
        }
        if (target == uint8_t(RobotMotionState::RLControlMimic)) {
            std::cout << "[standup] -> RLControlMimic" << std::endl;
            return StateName::kRLControlMimic;
        }

        // Fallback: go to AMP (default)
        std::cout << "[standup] default -> RLControlAMP" << std::endl;
        return StateName::kRLControlAMP;
    }
};

}  // namespace deep_robotics::dr02_pro
