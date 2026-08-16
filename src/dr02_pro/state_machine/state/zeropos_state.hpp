/**
 * @file zeropos_state.hpp
 * @brief Zero-position state: cubic-spline interpolation to all-zero joint pose.
 * @author DEEPRobotics
 * @date 2026-06-27
 *
 * @copyright Copyright (c) 2026 DEEPRobotics
 *
 * State flow:
 *   Idle → ZeroPos → (press 'c'/'v') → StandUp → RLControlAMP/Mimic
 *
 * ZeroPos interpolates all joints from their current position to the zero
 * pose over 3 seconds using cubic splines, then holds until the user
 * requests a state transition.
 */
#pragma once
#include "state_base.h"

namespace deep_robotics::dr02_pro {

class ZeroPosState : public StateBase {
private:
    float state_enter_time_;  ///< Timestamp when OnEnter was called.
    float run_time_;           ///< Latest interface timestamp.
    VecXf joint_pos_init_, joint_vel_init_;  ///< Joint state at entry.
    VecXf joint_pos_, joint_vel_;             ///< Latest joint state.
    VecXf kp_, kd_;                           ///< PD gains for all joints.
    VecXf joint_pos_goal_;                    ///< Target joint positions (all zeros).
    MatXf joint_cmd_;                        ///< Joint command matrix (dof×5).

    /**
     * @brief Read the latest robot joint state and timestamp.
     */
    void GetRobotJointValue() {
        run_time_ = ri_ptr_->GetInterfaceTimeStamp();
        joint_pos_ = ri_ptr_->GetJointPosition();
        joint_vel_ = ri_ptr_->GetJointVelocity();
    }

    /**
     * @brief Record the joint state at state entry for interpolation.
     */
    void RecordJointData() {
        state_enter_time_ = run_time_;
        joint_pos_init_ = joint_pos_;
        joint_vel_init_ = joint_vel_;
    }

public:
    /**
     * @brief Construct the ZeroPos state.
     *
     * Allocates joint command buffers and initializes PD gains from
     * ControlParameters (layout: waist(3), L_arm(7), R_arm(7),
     * L_leg(6), R_leg(6), neck(2)).
     *
     * @param robot_name Robot identifier.
     * @param state_name Human-readable state name.
     * @param data_ptr   Shared controller data.
     */
    ZeroPosState(const RobotName& robot_name, const std::string& state_name,
                 std::shared_ptr<ControllerData> data_ptr)
        : StateBase(robot_name, state_name, data_ptr) {
        joint_pos_goal_ = VecXf::Zero(cp_ptr_->dof_num_);
        joint_cmd_ = MatXf::Zero(cp_ptr_->dof_num_, 5);
        kp_ = VecXf::Zero(cp_ptr_->dof_num_);
        kd_ = VecXf::Zero(cp_ptr_->dof_num_);

        if (robot_name_ == RobotName::DR2PRO) {
            kp_ << cp_ptr_->waist_kp_,
                cp_ptr_->arm_kp_, cp_ptr_->arm_kp_,
                cp_ptr_->leg_kp_, cp_ptr_->leg_kp_,
                cp_ptr_->neck_kp_;
            kd_ << cp_ptr_->waist_kd_,
                cp_ptr_->arm_kd_, cp_ptr_->arm_kd_,
                cp_ptr_->leg_kd_, cp_ptr_->leg_kd_,
                cp_ptr_->neck_kd_;
        }
    }

    ~ZeroPosState() {}

    /**
     * @brief Enter the zero-position state.
     *
     * Records the current joint state as the interpolation start point
     * and sets the motion state to ZeroPos.
     */
    virtual void OnEnter() {
        GetRobotJointValue();
        RecordJointData();
        motion_state_store_->SetState(RobotMotionState::ZeroPos);
        std::cout << "[zeropos] entering, target=all-zeros, duration=3.0s" << std::endl;
    }

    /**
     * @brief Exit the zero-position state (no-op).
     */
    virtual void OnExit() {}

    /**
     * @brief Run one zero-position interpolation update.
     *
     * Computes cubic-spline interpolation from the initial joint state
     * to the zero pose over 3 seconds, then holds at the target.
     * Sends the resulting joint command to the robot interface.
     */
    virtual void Run() {
        GetRobotJointValue();

        VecXf joint_pos_plan = VecXf::Zero(cp_ptr_->dof_num_);
        VecXf joint_vel_plan = VecXf::Zero(cp_ptr_->dof_num_);
        VecXf joint_tor_plan = VecXf::Zero(cp_ptr_->dof_num_);

        const float init_time = 3.0f;
        float elapsed = run_time_ - state_enter_time_;

        if (elapsed <= init_time) {
            // Cubic-spline interpolation phase
            for (int i = 0; i < joint_pos_.rows(); ++i) {
                joint_pos_plan(i) = GetCubicSplinePos(
                    joint_pos_init_(i), joint_vel_init_(i),
                    joint_pos_goal_(i), 0, elapsed, init_time);
                joint_vel_plan(i) = GetCubicSplineVel(
                    joint_pos_init_(i), joint_vel_init_(i),
                    joint_pos_goal_(i), 0, elapsed, init_time);
            }
        } else {
            // Hold at target pose
            joint_pos_plan = joint_pos_goal_;
            joint_vel_plan.setZero();
        }

        // Assemble joint command: [kp, goal_pos, kd, goal_vel, tau_ff]
        joint_cmd_.col(0) = kp_;
        joint_cmd_.col(2) = kd_;
        joint_cmd_.col(1) = joint_pos_plan;
        joint_cmd_.col(3) = joint_vel_plan;
        joint_cmd_.col(4) = joint_tor_plan;

        ri_ptr_->SetJointCommand(joint_cmd_);
    }

    /**
     * @brief Determine the next state based on user commands.
     *
     * Priority:
     * 1. Safety override → kJointDamping
     * 2. User requested AMP (press 'c') → kStandUp (then → RLControlAMP)
     * 3. User requested Mimic (press 'v') → kStandUp (then → RLControlMimic)
     * 4. Otherwise → stay in kZeroPos
     *
     * @return Next state name.
     */
    virtual StateName GetNextStateName() {
        // Safety check: force damping on safety violation
        if (uc_ptr_->GetUserCommand()->safe_control_mode != 0 ||
            uc_ptr_->GetUserCommand()->target_mode ==
                uint8_t(RobotMotionState::JointDamping)) {
            std::cout << "safe_control_mode:"
                      << std::dec << uc_ptr_->GetUserCommand()->safe_control_mode
                      << std::endl;
            return StateName::kJointDamping;
        }

        // Transition to AMP via StandUp
        if (uc_ptr_->GetUserCommand()->target_mode ==
            uint8_t(RobotMotionState::RLControlAMP)) {
            std::cout << "go to standup -> amp" << std::endl;
            return StateName::kStandUp;
        }

        // Transition to Mimic via StandUp
        if (uc_ptr_->GetUserCommand()->target_mode ==
            uint8_t(RobotMotionState::RLControlMimic)) {
            std::cout << "go to standup -> mimic" << std::endl;
            return StateName::kStandUp;
        }

        return StateName::kZeroPos;
    }
};

}  // namespace deep_robotics::dr02_pro
