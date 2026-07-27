/**
 * @file rl_control_state.hpp
 * @brief RL control state for the state machine.
 * @author DEEPRobotics
 * @date 2026-06-27
 *
 * @copyright Copyright (c) 2026 DEEPRobotics
 *
 */
#pragma once

#include <atomic>
#include <chrono>
#include <cstring>
#include <mutex>
#include <thread>

#include "common_policy_runner.hpp"
#include "state_base.h"

namespace deep_robotics::dr02_pro {
class RLControlState : public StateBase {
private:
    RobotBasicState rbs_;
    std::mutex rbs_mutex_;
    std::atomic<int> state_run_cnt_{0};

    std::shared_ptr<PolicyRunnerBase> policy_ptr_;
    std::shared_ptr<CommonPolicyRunner> common_policy_;

    UserCommand user_command;
    std::mutex user_command_policy_mutex_;

    std::thread run_policy_thread_;
    std::atomic<bool> start_flag_{false};

    int acc_rot_count = 0;

    /**
     * @brief Initialize the robot basic state.
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
     * @brief Update the robot observation snapshot.
     */
    void UpdateRobotObservation() {
        rbs_.base_rpy = ri_ptr_->GetImuBaseRpy();
        rbs_.base_rot_mat = RpyToRm(rbs_.base_rpy);
        rbs_.base_omega = ri_ptr_->GetImuBaseOmega();
        rbs_.base_acc = ri_ptr_->GetImuBaseAcc();
        rbs_.joint_pos = ri_ptr_->GetJointPosition();
        rbs_.joint_vel = ri_ptr_->GetJointVelocity();
        rbs_.joint_tau = ri_ptr_->GetJointTorque();

        rbs_.flt_base_acc_mat.row(acc_rot_count) = rbs_.base_acc.transpose();
        rbs_.flt_joint_vel_mat.row(acc_rot_count) = rbs_.joint_vel.transpose();
        rbs_.flt_base_omega_mat.row(acc_rot_count) = rbs_.base_omega.transpose();

        acc_rot_count += 1;
        acc_rot_count = acc_rot_count % 20;
    }

    /**
     * @brief Run policy inference and publish policy joint commands.
     */
    void PolicyRunner() {
        int run_cnt_record = -1;
        MatXf joint_cmd(cp_ptr_->dof_num_, 5);
        MatXf leg_joint_cmd(2 * cp_ptr_->leg_dof_num_, 5);
        MatXf arm_joint_cmd(2 * cp_ptr_->arm_dof_num_, 5);
        MatXf waist_joint_cmd(cp_ptr_->waist_dof_num_, 5);
        MatXf neck_joint_cmd(cp_ptr_->neck_dof_num_, 5);

        UserCommand user_command_tmp;
        while (start_flag_.load(std::memory_order_acquire)) {
            {
                std::lock_guard<std::mutex> lock(user_command_policy_mutex_);
                memcpy(&user_command_tmp, &user_command, sizeof(UserCommand));
            }

            const int current_run_cnt = state_run_cnt_.load(std::memory_order_relaxed);
            if (current_run_cnt - run_cnt_record >= policy_ptr_->decimation_) {
                RobotBasicState robot_state_snapshot;
                {
                    std::lock_guard<std::mutex> lock(rbs_mutex_);
                    robot_state_snapshot = rbs_;
                }

                auto ra = policy_ptr_->GetRobotAction(robot_state_snapshot, user_command_tmp);  // actdim(12)*5
                MatXf res = ra.ConvertToMat();

                waist_joint_cmd.setZero();
                arm_joint_cmd.setZero();
                neck_joint_cmd.setZero();
                leg_joint_cmd.setZero();

                arm_joint_cmd.col(0) = cp_ptr_->arm_kp_.replicate(2, 1);
                arm_joint_cmd.col(2) = cp_ptr_->arm_kd_.replicate(2, 1);
                waist_joint_cmd.col(0) = cp_ptr_->waist_kp_;
                waist_joint_cmd.col(2) = cp_ptr_->waist_kd_;

                // Only replace joints covered by the policy output; keep PD lock for the rest
                waist_joint_cmd.block<1, 5>(0, 0) = res.block<1, 5>(0, 0);
                arm_joint_cmd.block<4, 5>(0, 0) = res.block<4, 5>(1, 0);
                arm_joint_cmd.block<4, 5>(7, 0) = res.block<4, 5>(5, 0);
                leg_joint_cmd = res.block<12, 5>(9, 0);
                joint_cmd << waist_joint_cmd, arm_joint_cmd, leg_joint_cmd, neck_joint_cmd;

                ri_ptr_->SetJointCommand(joint_cmd);
                run_cnt_record = current_run_cnt;
            }
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    }

public:
    RLControlState(const RobotName& robot_name, const std::string& state_name, std::shared_ptr<ControllerData> data_ptr)
        : StateBase(robot_name, state_name, data_ptr) {
        common_policy_ = std::make_shared<CommonPolicyRunner>("common", robot_name);
        common_policy_->SetCmdMaxVel(Vec3f(1.8, 0.5, 2.5));
        common_policy_->DisplayPolicyInfo();

        std::lock_guard<std::mutex> lock(user_command_policy_mutex_);
        policy_ptr_ = common_policy_;
        if (!policy_ptr_) {
            std::cerr << "error policy" << std::endl;
            exit(0);
        }
        InitRbs();
    }

    ~RLControlState() {
        if (run_policy_thread_.joinable()) {
            start_flag_.store(false, std::memory_order_release);
            run_policy_thread_.join();
        }
    }

    /**
     * @brief Enter the RL control state.
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
        motion_state_store_->SetState(RobotMotionState::RLControl);
        run_policy_thread_ = std::thread(std::bind(&RLControlState::PolicyRunner, this));
    };

    /**
     * @brief Exit the RL control state.
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
     * @brief Run one RL control state update.
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
     * @brief Get the next state requested from RL control.
     * @return Next state name.
     */
    virtual StateName GetNextStateName() {
        std::lock_guard<std::mutex> lock(user_command_policy_mutex_);
        if (user_command.safe_control_mode != 0 ||
            user_command.target_mode == uint8_t(RobotMotionState::JointDamping)) {
            return StateName::kJointDamping;
        }
        return StateName::kRLControl;
    }
};
}  // namespace deep_robotics::dr02_pro
