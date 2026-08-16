/**
 * @file rl_control_mimic_state.hpp
 * @brief RL control state using MimicPolicyRunner (29-DOF mimic policy).
 * @author DEEPRobotics
 * @date 2026-08-04
 *
 * @copyright Copyright (c) 2026 DEEPRobotics
 *
 * State flow:
 *   ZeroPos → RLControlMimic → (action done / 'c') → RLControlAMP (direct switch)
 */
#pragma once

#include <atomic>
#include <chrono>
#include <cstring>
#include <mutex>
#include <thread>

#include "mimic_policy_runner.hpp"
#include "state_base.h"

namespace deep_robotics::dr02_pro {

class RLControlMimicState : public StateBase {
private:
    RobotBasicState rbs_;
    std::mutex rbs_mutex_;
    std::atomic<int> state_run_cnt_{0};

    std::shared_ptr<MimicPolicyRunner> policy_ptr_;

    UserCommand user_command;
    std::mutex user_command_policy_mutex_;

    std::thread run_policy_thread_;
    std::atomic<bool> start_flag_{false};
    std::atomic<bool> action_done_{false};  ///< set when mimic action completes

    int acc_rot_count = 0;

    /**
     * @brief Initialize the robot basic state buffer to zeros.
     */
    void InitRbs() {
        rbs_.base_rpy.setZero();
        rbs_.base_quat.setZero();
        rbs_.base_rot_mat.setIdentity();
        rbs_.base_omega.setZero();
        rbs_.flt_base_acc_mat = Eigen::MatrixXf::Zero(20, 3);
        rbs_.flt_base_omega_mat = Eigen::MatrixXf::Zero(20, 3);
        rbs_.base_acc.setZero();
        rbs_.joint_pos = VecXf::Zero(cp_ptr_->dof_num_);
        rbs_.joint_vel = VecXf::Zero(cp_ptr_->dof_num_);
        rbs_.flt_joint_vel_mat = Eigen::MatrixXf::Zero(20, cp_ptr_->dof_num_);
        rbs_.joint_tau = VecXf::Zero(cp_ptr_->dof_num_);
    }

    /**
     * @brief Read the latest sensor data from the robot interface.
     *
     * Fills rbs_ with IMU and joint data, and appends to the rolling
     * filter matrices (flt_*_mat) at the current acc_rot_count row.
     */
    void UpdateRobotObservation() {
        rbs_.base_rpy = ri_ptr_->GetImuBaseRpy();
        rbs_.base_rot_mat = RpyToRm(rbs_.base_rpy);
        rbs_.base_omega = ri_ptr_->GetImuBaseOmega();
        rbs_.base_acc = ri_ptr_->GetImuBaseAcc();
        rbs_.joint_pos = ri_ptr_->GetJointPosition();
        rbs_.joint_vel = ri_ptr_->GetJointVelocity();
        rbs_.joint_tau = ri_ptr_->GetJointTorque();

        // Append to rolling filter buffers (20-row circular buffer)
        rbs_.flt_base_acc_mat.row(acc_rot_count) = rbs_.base_acc.transpose();
        rbs_.flt_joint_vel_mat.row(acc_rot_count) = rbs_.joint_vel.transpose();
        rbs_.flt_base_omega_mat.row(acc_rot_count) = rbs_.base_omega.transpose();

        acc_rot_count += 1;
        acc_rot_count = acc_rot_count % 20;
    }

    /**
     * @brief Check if it is safe to switch to the AMP policy.
     *
     * Safety checks:
     * 1. Velocity commands near zero (user not commanding movement).
     * 2. Body posture stable (roll/pitch < 0.15 rad ≈ 8.6°).
     * 3. Joint velocity low (no rapid motion).
     *
     * @return true if all safety checks pass.
     */
    bool IsReadyToSwitch() {
        // 1. Velocity commands near zero
        if (fabs(user_command.forward_vel_scale) > 0.01f ||
            fabs(user_command.side_vel_scale) > 0.01f ||
            fabs(user_command.turning_vel_scale) > 0.01f) {
            std::cout << "[SWITCH] velocity command not zero, cannot switch" << std::endl;
            return false;
        }
        // 2. Body posture stable
        Vec3f rpy = ri_ptr_->GetImuBaseRpy();
        if (fabs(rpy(0)) > 0.15f || fabs(rpy(1)) > 0.15f) {
            std::cout << "[SWITCH] body posture unstable, cannot switch" << std::endl;
            return false;
        }
        // 3. Joint velocity low
        VecXf joint_vel = ri_ptr_->GetJointVelocity();
        float max_vel = joint_vel.cwiseAbs().maxCoeff();
        if (max_vel > 1.0f) {
            std::cout << "[SWITCH] joint velocity too high (" << max_vel
                      << "), cannot switch" << std::endl;
            return false;
        }
        return true;
    }

    /**
     * @brief Policy inference loop (runs in a dedicated thread).
     *
     * Periodically reads the latest robot state snapshot, runs policy
     * inference at the configured decimation rate, sends joint commands
     * to the robot interface, and checks if the mimic action has completed.
     */
    void PolicyRunner() {
        int run_cnt_record = -1;

        while (start_flag_.load(std::memory_order_acquire)) {
            // Snapshot user command under lock
            UserCommand user_command_tmp;
            {
                std::lock_guard<std::mutex> lock(user_command_policy_mutex_);
                memcpy(&user_command_tmp, &user_command, sizeof(UserCommand));
            }

            // Run inference at decimation rate
            const int current_run_cnt = state_run_cnt_.load(std::memory_order_relaxed);
            if (current_run_cnt - run_cnt_record >= policy_ptr_->decimation_) {
                // Snapshot robot state under lock
                RobotBasicState robot_state_snapshot;
                {
                    std::lock_guard<std::mutex> lock(rbs_mutex_);
                    robot_state_snapshot = rbs_;
                }

                // Inference → 31-dim joint command → send to robot
                auto ra = policy_ptr_->GetRobotAction(robot_state_snapshot, user_command_tmp);
                MatXf res = ra.ConvertToMat();
                ri_ptr_->SetJointCommand(res);
                run_cnt_record = current_run_cnt;

                // Check if mimic action has completed (single playback)
                if (policy_ptr_->IsActionCompleted()) {
                    action_done_.store(true, std::memory_order_relaxed);
                }
            }
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    }

public:
    /**
     * @brief Construct the Mimic RL control state.
     *
     * Initializes the MimicPolicyRunner and robot state buffer.
     *
     * @param robot_name Robot identifier.
     * @param state_name Human-readable state name for logging.
     * @param data_ptr   Shared controller data (robot interface, user command, etc.).
     */
    RLControlMimicState(const RobotName& robot_name, const std::string& state_name,
                        std::shared_ptr<ControllerData> data_ptr)
        : StateBase(robot_name, state_name, data_ptr) {
        policy_ptr_ = std::make_shared<MimicPolicyRunner>("mimic", robot_name);
        policy_ptr_->DisplayPolicyInfo();
        InitRbs();
    }

    ~RLControlMimicState() {
        if (run_policy_thread_.joinable()) {
            start_flag_.store(false, std::memory_order_release);
            run_policy_thread_.join();
        }
    }

    /**
     * @brief Enter the Mimic control state.
     *
     * Resets run counter and action_done flag, snapshots robot observation,
     * starts the policy thread, and sets the motion state to RLControlMimic.
     */
    virtual void OnEnter() {
        state_run_cnt_.store(-1, std::memory_order_relaxed);

        {
            std::lock_guard<std::mutex> lock(rbs_mutex_);
            UpdateRobotObservation();
        }
        start_flag_.store(true, std::memory_order_release);
        {
            std::lock_guard<std::mutex> lock(user_command_policy_mutex_);
            policy_ptr_->OnEnter();
            memcpy(&user_command, uc_ptr_->GetUserCommand(), sizeof(UserCommand));
        }
        motion_state_store_->SetState(RobotMotionState::RLControlMimic);
        action_done_.store(false, std::memory_order_relaxed);
        run_policy_thread_ = std::thread(std::bind(&RLControlMimicState::PolicyRunner, this));
    }

    /**
     * @brief Exit the Mimic control state.
     *
     * Stops the policy thread, calls policy OnExit, and resets the run counter.
     */
    virtual void OnExit() {
        start_flag_.store(false, std::memory_order_release);
        if (run_policy_thread_.joinable()) {
            run_policy_thread_.join();
        }
        policy_ptr_->OnExit();
        state_run_cnt_.store(-1, std::memory_order_relaxed);
    }

    /**
     * @brief Run one state update tick (called by main control loop).
     *
     * Updates robot observation and user command under their respective locks,
     * then increments the run counter for the policy thread to consume.
     */
    virtual void Run() {
        {
            std::lock_guard<std::mutex> lock(rbs_mutex_);
            UpdateRobotObservation();
        }

        state_run_cnt_.fetch_add(1, std::memory_order_relaxed);
        {
            std::lock_guard<std::mutex> lock(user_command_policy_mutex_);
            memcpy(&user_command, uc_ptr_->GetUserCommand(), sizeof(UserCommand));
        }
    }

    /**
     * @brief Determine the next state based on safety checks and user commands.
     *
     * Priority:
     * 1. Safety override → kJointDamping
     * 2. Mimic action completed → kRLControlAMP (direct auto-switch)
     * 3. User requested AMP switch (press 'c') → kRLControlAMP (direct)
     * 4. Otherwise → stay in kRLControlMimic
     *
     * @return Next state name.
     */
    virtual StateName GetNextStateName() {
        std::lock_guard<std::mutex> lock(user_command_policy_mutex_);

        // Safety check: force damping on safety violation
        if (user_command.safe_control_mode != 0 ||
            user_command.target_mode == uint8_t(RobotMotionState::JointDamping)) {
            return StateName::kJointDamping;
        }

        // Auto-switch to AMP when mimic action completes (direct switch)
        if (action_done_.load(std::memory_order_relaxed)) {
            std::cout << "[MIMIC] action completed, -> AMP (direct)" << std::endl;
            user_command.target_mode = uint8_t(RobotMotionState::RLControlAMP);
            uc_ptr_->GetUserCommand()->target_mode =
                uint8_t(RobotMotionState::RLControlAMP);
            return StateName::kRLControlAMP;
        }

        // Manual switch to AMP ('c' key sets target_mode = RLControlAMP)
        if (user_command.target_mode == uint8_t(RobotMotionState::RLControlAMP)) {
            if (IsReadyToSwitch()) {
                std::cout << "[SWITCH] Mimic -> AMP (direct)" << std::endl;
                return StateName::kRLControlAMP;
            } else {
                // Not safe to switch — reset target and stay in Mimic
                user_command.target_mode = uint8_t(RobotMotionState::RLControlMimic);
                uc_ptr_->GetUserCommand()->target_mode =
                    uint8_t(RobotMotionState::RLControlMimic);
            }
        }

        return StateName::kRLControlMimic;
    }
};

}  // namespace deep_robotics::dr02_pro
