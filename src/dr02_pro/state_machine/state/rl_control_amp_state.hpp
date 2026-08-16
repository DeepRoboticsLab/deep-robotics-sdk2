/**
 * @file rl_control_amp_state.hpp
 * @brief RL control state using AmpPolicyRunner (21-DOF AMP policy).
 * @author DEEPRobotics
 * @date 2026-08-04
 *
 * @copyright Copyright (c) 2026 DEEPRobotics
 *
 * State flow:
 *   ZeroPos → RLControlAMP → (press 'v') → RLControlMimic (direct switch)
 *
 * AmpPolicyRunner outputs 21-DOF in output_order. This state maps the 21-dim
 * action to a 31-DOF joint command, filling uncontrolled joints (waist_x/y,
 * wrists, neck) with PD-locked zeros using ControlParameters gains.
 */
#pragma once

#include <atomic>
#include <chrono>
#include <cstring>
#include <mutex>
#include <thread>
#include <unordered_map>

#include "amp_policy_runner.hpp"
#include "state_base.h"

namespace deep_robotics::dr02_pro {

class RLControlAMPState : public StateBase {
private:
    RobotBasicState rbs_;
    std::mutex rbs_mutex_;
    std::atomic<int> state_run_cnt_{0};

    std::shared_ptr<AmpPolicyRunner> policy_ptr_;

    UserCommand user_command;
    std::mutex user_command_policy_mutex_;

    std::thread run_policy_thread_;
    std::atomic<bool> start_flag_{false};

    int acc_rot_count = 0;

    /// Mapping from 21-dim output_order index → 31-dim robot_order index
    std::vector<int> output_to_robot_idx_;

    /// Default PD for all 31 joints (from ControlParameters), used for uncontrolled joints
    VecXf kp_default_, kd_default_;

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
     * @brief Build the output_order → robot_order index mapping.
     *
     * AMP output_order (21-dim): waist_z, L_arm(4), R_arm(4), L_leg(6), R_leg(6)
     * Robot order (31-dim): waist(3), L_arm(7), R_arm(7), L_leg(6), R_leg(6), neck(2)
     *
     * @return Permutation vector mapping each 21-dim output index to a 31-dim robot index.
     */
    static std::vector<int> BuildOutputToRobotIdx() {
        // 31-dim robot_order (hardware order)
        std::vector<std::string> robot_order = {
            "waist_z_joint", "waist_x_joint", "waist_y_joint",
            "left_shoulder_y_joint", "left_shoulder_x_joint", "left_shoulder_z_joint",
            "left_elbow_joint", "left_wrist_z_joint", "left_wrist_y_joint", "left_wrist_x_joint",
            "right_shoulder_y_joint", "right_shoulder_x_joint", "right_shoulder_z_joint",
            "right_elbow_joint", "right_wrist_z_joint", "right_wrist_y_joint", "right_wrist_x_joint",
            "left_hip_y_joint", "left_hip_x_joint", "left_hip_z_joint",
            "left_knee_joint", "left_ankle_y_joint", "left_ankle_x_joint",
            "right_hip_y_joint", "right_hip_x_joint", "right_hip_z_joint",
            "right_knee_joint", "right_ankle_y_joint", "right_ankle_x_joint",
            "neck_z_joint", "neck_y_joint",
        };

        // 21-dim output_order (policy output order)
        std::vector<std::string> output_order = {
            "waist_z_joint",
            "left_shoulder_y_joint", "left_shoulder_x_joint", "left_shoulder_z_joint",
            "left_elbow_joint",
            "right_shoulder_y_joint", "right_shoulder_x_joint", "right_shoulder_z_joint",
            "right_elbow_joint",
            "left_hip_y_joint", "left_hip_x_joint", "left_hip_z_joint",
            "left_knee_joint", "left_ankle_y_joint", "left_ankle_x_joint",
            "right_hip_y_joint", "right_hip_x_joint", "right_hip_z_joint",
            "right_knee_joint", "right_ankle_y_joint", "right_ankle_x_joint",
        };

        // Build name → index map for robot_order
        std::unordered_map<std::string, int> idx_map;
        for (int i = 0; i < static_cast<int>(robot_order.size()); ++i) {
            idx_map[robot_order[i]] = i;
        }

        // For each output_order joint, find its index in robot_order
        std::vector<int> perm;
        for (const auto& name : output_order) {
            perm.push_back(idx_map.at(name));
        }
        return perm;
    }

    /**
     * @brief Build default 31-dim PD gains from ControlParameters.
     *
     * Layout: waist(3), L_arm(7), R_arm(7), L_leg(6), R_leg(6), neck(2).
     * These gains are used for joints NOT covered by the 21-dim policy output
     * (i.e. waist_x/y, wrists, neck).
     */
    void BuildDefaultPD() {
        int n = cp_ptr_->dof_num_;  // 31
        kp_default_ = VecXf::Zero(n);
        kd_default_ = VecXf::Zero(n);

        kp_default_ << cp_ptr_->waist_kp_,
            cp_ptr_->arm_kp_, cp_ptr_->arm_kp_,
            cp_ptr_->leg_kp_, cp_ptr_->leg_kp_,
            cp_ptr_->neck_kp_;

        kd_default_ << cp_ptr_->waist_kd_,
            cp_ptr_->arm_kd_, cp_ptr_->arm_kd_,
            cp_ptr_->leg_kd_, cp_ptr_->leg_kd_,
            cp_ptr_->neck_kd_;
    }

    /**
     * @brief Map 21-dim policy output to 31-dim joint command.
     *
     * 1. Start with all 31 joints: goal=0, vel=0, tau=0, PD from ControlParameters.
     * 2. Overwrite the 21 controlled joints with policy output (including kp/kd).
     *
     * @param res 21×5 matrix from AmpPolicyRunner::RobotAction::ConvertToMat().
     * @return 31×5 joint command matrix (cols: kp, goal_pos, kd, goal_vel, tau_ff).
     */
    MatXf MapToJointCommand(const MatXf& res) {
        int n = cp_ptr_->dof_num_;  // 31
        MatXf joint_cmd = MatXf::Zero(n, 5);

        // Step 1: Fill all joints with default PD (goal=0, vel=0, tau=0)
        joint_cmd.col(0) = kp_default_;  // kp
        joint_cmd.col(2) = kd_default_;  // kd

        // Step 2: Overwrite 21 controlled joints from policy output
        for (int i = 0; i < static_cast<int>(output_to_robot_idx_.size()); ++i) {
            int robot_idx = output_to_robot_idx_[i];
            joint_cmd(robot_idx, 0) = res(i, 0);  // kp
            joint_cmd(robot_idx, 1) = res(i, 1);  // goal_pos
            joint_cmd(robot_idx, 2) = res(i, 2);  // kd
            joint_cmd(robot_idx, 3) = res(i, 3);  // goal_vel
            joint_cmd(robot_idx, 4) = res(i, 4);  // tau_ff
        }

        return joint_cmd;
    }

    /**
     * @brief Check if it is safe to switch to the Mimic policy.
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
     * inference at the configured decimation rate, and sends joint
     * commands to the robot interface.
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

                // Inference → 21-dim action → 31-dim joint command
                auto ra = policy_ptr_->GetRobotAction(robot_state_snapshot, user_command_tmp);
                MatXf res = ra.ConvertToMat();          // 21×5
                MatXf joint_cmd = MapToJointCommand(res);  // 31×5
                ri_ptr_->SetJointCommand(joint_cmd);
                run_cnt_record = current_run_cnt;
            }
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    }

public:
    /**
     * @brief Construct the AMP RL control state.
     *
     * Initializes joint order mapping, default PD gains, and the AmpPolicyRunner.
     *
     * @param robot_name Robot identifier.
     * @param state_name Human-readable state name for logging.
     * @param data_ptr   Shared controller data (robot interface, user command, etc.).
     */
    RLControlAMPState(const RobotName& robot_name, const std::string& state_name,
                      std::shared_ptr<ControllerData> data_ptr)
        : StateBase(robot_name, state_name, data_ptr) {
        output_to_robot_idx_ = BuildOutputToRobotIdx();
        BuildDefaultPD();

        policy_ptr_ = std::make_shared<AmpPolicyRunner>("amp", robot_name);
        policy_ptr_->SetCmdMaxVel(Vec3f(1.0, 0.5, 1.2));
        policy_ptr_->DisplayPolicyInfo();
        InitRbs();
    }

    ~RLControlAMPState() {
        if (run_policy_thread_.joinable()) {
            start_flag_.store(false, std::memory_order_release);
            run_policy_thread_.join();
        }
    }

    /**
     * @brief Enter the AMP control state.
     *
     * Resets run counter, snapshots robot observation, starts the policy
     * thread, and sets the motion state to RLControlAMP.
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
        motion_state_store_->SetState(RobotMotionState::RLControlAMP);
        run_policy_thread_ = std::thread(std::bind(&RLControlAMPState::PolicyRunner, this));
    }

    /**
     * @brief Exit the AMP control state.
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
     * 2. User requested Mimic switch (press 'v') → kRLControlMimic (direct)
     * 3. Otherwise → stay in kRLControlAMP
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

        // Switch directly to Mimic ('v' key sets target_mode = RLControlMimic)
        if (user_command.target_mode == uint8_t(RobotMotionState::RLControlMimic)) {
            if (IsReadyToSwitch()) {
                std::cout << "[SWITCH] AMP -> Mimic (direct)" << std::endl;
                return StateName::kRLControlMimic;
            } else {
                // Not safe to switch — reset target and stay in AMP
                user_command.target_mode = uint8_t(RobotMotionState::RLControlAMP);
                uc_ptr_->GetUserCommand()->target_mode =
                    uint8_t(RobotMotionState::RLControlAMP);
            }
        }

        return StateName::kRLControlAMP;
    }
};

}  // namespace deep_robotics::dr02_pro
