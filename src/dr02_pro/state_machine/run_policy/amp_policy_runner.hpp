/**
 * @file amp_policy_runner.hpp
 * @brief AMP (Adversarial Motion Prior) ONNX policy runner for RL control.
 * @author DEEPRobotics
 * @date 2026-07-24
 *
 * @copyright Copyright (c) 2026 DEEPRobotics
 *
 */

#pragma once

#include "policy_runner_base.hpp"
#include <filesystem>
#include <cmath>
#include <iostream>
#include <onnxruntime_cxx_api.h>
#include <unordered_map>

namespace deep_robotics::dr02_pro {

class AmpPolicyRunner : public PolicyRunnerBase {
private:
    // Control parameters
    VecXf kp_, kd_;
    VecXf dof_pos_default_;
    Vec3f max_cmd_vel_;
    VecXf action_scale_;
    float lin_vel_scale_, ang_vel_scale_, dof_vel_scale_;
    Vec3f cmd_vel_scale_;
    float zero_cmd_threshold_xy_, zero_cmd_threshold_z_;

    // Observation/action buffers
    int obs_dim_, obs_history_num_, act_dim_;
    int policy_input_dim_;
    VecXf current_observation_, observation_history_;
    VecXf action_, last_action_;
    VecXf obs_joint_pos_, obs_joint_vel_;
    VecXf action_min_, action_max_;

    // Joint order mappings
    std::vector<int> robot_to_policy_idx_;
    std::vector<int> policy_to_output_idx_;
    std::vector<int> policy_to_arm_idx_;

    // ONNX runtime resources
    Ort::SessionOptions session_options_;
    Ort::Session session_{nullptr};
    Ort::MemoryInfo memory_info_{nullptr};
    Ort::Env env_;
    std::vector<Ort::Value> ort_inputs_;

    const char* input_names_[1] = {"observation"};
    const char* output_names_[1] = {"action"};

public:
    AmpPolicyRunner(const std::string& policy_name, RobotName robot_name)
        : PolicyRunnerBase(policy_name, robot_name) {
        kp_ = VecXf::Zero(21);
        kp_ << 250.0, 250.0, 180.0, 250.0, 100.0, 40.0,
               250.0, 250.0, 180.0, 250.0, 100.0, 40.0,
               150.0,
               100.0, 100.0, 100.0, 100.0,
               100.0, 100.0, 100.0, 100.0;
        kd_ = VecXf::Zero(21);
        kd_ << 6., 6., 4., 6., 2.5, 1.0,
               6., 6., 4., 6., 2.5, 1.0,
               3.0,
               2.5, 2.5, 2.5, 2.5,
               2.5, 2.5, 2.5, 2.5;

        // Joint order definitions (DR2PRO == CR1PRO layout, 29 robot DOF)
        std::vector<std::string> robot_order = {
            "waist_z_joint", "waist_x_joint", "waist_y_joint",
            "left_shoulder_y_joint", "left_shoulder_x_joint", "left_shoulder_z_joint",
            "left_elbow_joint", "left_wrist_z_joint", "left_wrist_y_joint", "left_wrist_x_joint",
            "right_shoulder_y_joint", "right_shoulder_x_joint", "right_shoulder_z_joint",
            "right_elbow_joint", "right_wrist_z_joint", "right_wrist_y_joint", "right_wrist_x_joint",
            "left_hip_y_joint", "left_hip_x_joint", "left_hip_z_joint",
            "left_knee_joint", "left_ankle_y_joint", "left_ankle_x_joint",
            "right_hip_y_joint", "right_hip_x_joint", "right_hip_z_joint",
            "right_knee_joint", "right_ankle_y_joint", "right_ankle_x_joint"
        };

        std::vector<std::string> policy_order = {
            "left_hip_y_joint", "left_hip_x_joint", "left_hip_z_joint",
            "left_knee_joint", "left_ankle_y_joint", "left_ankle_x_joint",
            "right_hip_y_joint", "right_hip_x_joint", "right_hip_z_joint",
            "right_knee_joint", "right_ankle_y_joint", "right_ankle_x_joint",
            "waist_z_joint",
            "left_shoulder_y_joint", "left_shoulder_x_joint", "left_shoulder_z_joint",
            "left_elbow_joint",
            "right_shoulder_y_joint", "right_shoulder_x_joint", "right_shoulder_z_joint",
            "right_elbow_joint",
        };

        std::vector<std::string> output_order = {
            "waist_z_joint",
            "left_shoulder_y_joint", "left_shoulder_x_joint", "left_shoulder_z_joint",
            "left_elbow_joint",
            "right_shoulder_y_joint", "right_shoulder_x_joint", "right_shoulder_z_joint",
            "right_elbow_joint",
            "left_hip_y_joint", "left_hip_x_joint", "left_hip_z_joint",
            "left_knee_joint", "left_ankle_y_joint", "left_ankle_x_joint",
            "right_hip_y_joint", "right_hip_x_joint", "right_hip_z_joint",
            "right_knee_joint", "right_ankle_y_joint", "right_ankle_x_joint"
        };

        std::vector<std::string> arm_joints_order = {
            "left_shoulder_y_joint", "left_shoulder_x_joint", "left_shoulder_z_joint",
            "left_elbow_joint",
            "right_shoulder_y_joint", "right_shoulder_x_joint", "right_shoulder_z_joint",
            "right_elbow_joint"
        };

        robot_to_policy_idx_ = GeneratePermutation(robot_order, policy_order);
        policy_to_output_idx_ = GeneratePermutation(policy_order, output_order);
        policy_to_arm_idx_ = GeneratePermutation(policy_order, arm_joints_order);

        decimation_ = 10;
        lin_vel_scale_ = 2.0;
        ang_vel_scale_ = 0.2;
        dof_vel_scale_ = 0.1;
        cmd_vel_scale_ << lin_vel_scale_, lin_vel_scale_, ang_vel_scale_;
        zero_cmd_threshold_xy_ = 0.15;
        zero_cmd_threshold_z_ = 0.15;

        obs_dim_ = 73;
        obs_history_num_ = 10;
        act_dim_ = 21;
        policy_input_dim_ = obs_history_num_ * obs_dim_;

        dof_pos_default_.setZero(act_dim_);
        dof_pos_default_ << -0.1, 0., 0., 0.2, -0.1, 0.,
                           -0.1, 0., 0., 0.2, -0.1, 0.,
                           0.,
                           0.,  0.15, 0., 1.35,
                           0., -0.15, 0., 1.35;

        action_scale_.setZero(act_dim_);
        action_scale_ << 0.25, 0.25, 0.25, 0.25, 0.5, 0.25,
                         0.25, 0.25, 0.25, 0.25, 0.5, 0.25,
                         0.25,
                         0.25, 0.25, 0.25, 0.25,
                         0.25, 0.25, 0.25, 0.25;

        action_min_ = VecXf::Constant(act_dim_, -10.0);
        action_max_ = VecXf::Constant(act_dim_, 10.0);

        max_cmd_vel_.setZero(3);

        session_options_.SetIntraOpNumThreads(1);
        session_options_.SetGraphOptimizationLevel(ORT_ENABLE_ALL);
        ort_inputs_.reserve(1);

        env_ = Ort::Env(ORT_LOGGING_LEVEL_ERROR, policy_name_.data());
        memory_info_ = Ort::MemoryInfo::CreateCpu(OrtAllocatorType::OrtArenaAllocator,
                                                   OrtMemType::OrtMemTypeDefault);

        try {
            namespace fs = std::filesystem;
            const fs::path base = fs::path(__FILE__).parent_path();
            const std::string policy_path = fs::canonical(base / ".." / "policy" / "policy.onnx");
            session_ = Ort::Session(env_, policy_path.c_str(), session_options_);
        } catch (const std::exception& e) {
            std::cerr << "error loading policy as " << policy_name_ << "\n" << e.what();
        }
    }

    ~AmpPolicyRunner() override {
        std::cout << "AmpPolicyRunner " << policy_name_ << " destroyed" << std::endl;
    }

    /**
     * @brief Display AMP policy information.
     */
    void DisplayPolicyInfo() override {
        std::cout << policy_name_ << " network test success" << std::endl;
        std::cout << "dim  : " << obs_dim_ << " " << obs_history_num_ << " "
                  << policy_input_dim_ << " " << act_dim_ << std::endl;
        std::cout << "dof  : " << dof_pos_default_.transpose() << std::endl;
        std::cout << "kp   : " << kp_.transpose() << std::endl;
        std::cout << "kd   : " << kd_.transpose() << std::endl;
        std::cout << "max_v: " << max_cmd_vel_.transpose() << std::endl;
    }

    /**
     * @brief Reset policy runner buffers on entry.
     */
    void OnEnter() override {
        action_.setZero(act_dim_);
        last_action_.setZero(act_dim_);
        obs_joint_pos_.setZero(act_dim_);
        obs_joint_vel_.setZero(act_dim_);

        current_observation_.setZero(obs_dim_);
        observation_history_.setZero(obs_dim_ * obs_history_num_);

        cmd_vel_input_.setZero();
        run_cnt_ = 0;
        std::cout << "[amp policy runner] on enter" << std::endl;
    }

    /**
     * @brief Execute policy runner exit logic.
     */
    void OnExit() override {
    }

    /**
     * @brief Run AMP policy inference for a robot state and user command.
     * @param ro Current robot basic state.
     * @param uc Current user command.
     * @return Robot action produced by the policy.
     */
    RobotAction GetRobotAction(const RobotBasicState &ro, const UserCommand &uc) override {
        Vec3f user_cmd_vel = Vec3f(uc.forward_vel_scale,
                                   uc.side_vel_scale,
                                   uc.turning_vel_scale);
        if (uc.forward_vel_scale < 0) user_cmd_vel[0] = 0.6f * uc.forward_vel_scale;

        Eigen::Vector3f vel_delta = user_cmd_vel - cmd_vel_input_;
        vel_delta_const_ << 0.02, 0.02, 0.08;
        for (int i = 0; i < 3; ++i) {
            if (fabs(vel_delta(i)) > vel_delta_const_(i)) {
                vel_delta(i) = Sign(vel_delta(i)) * vel_delta_const_(i);
            }
        }
        cmd_vel_input_ += vel_delta;
        Vec3f cmd_vel = cmd_vel_input_.cwiseProduct(max_cmd_vel_);

        Vec3f project_gravity = ro.base_rot_mat.transpose() * Vec3f(0., 0., -1);
        float cmd_flag = 0.0f;
        if (fabs(cmd_vel[2]) > zero_cmd_threshold_z_ ||
            cmd_vel.head(2).norm() > zero_cmd_threshold_xy_) {
            cmd_flag = 1.0f;
        }

        current_observation_.setZero(obs_dim_);

        for (int i = 0; i < act_dim_; i++) {
            obs_joint_pos_(i) = ro.joint_pos(robot_to_policy_idx_[i]);
            obs_joint_vel_(i) = ro.joint_vel(robot_to_policy_idx_[i]);
        }

        current_observation_ << ro.base_omega * ang_vel_scale_,
                                project_gravity,
                                cmd_vel.cwiseProduct(cmd_vel_scale_),
                                cmd_flag,
                                (obs_joint_pos_ - dof_pos_default_).cwiseQuotient(action_scale_),
                                obs_joint_vel_ * dof_vel_scale_,
                                last_action_;

        // Shift history and append current observation at the tail
        VecXf obs_history_record =
            observation_history_.segment(obs_dim_, (obs_history_num_ - 1) * obs_dim_).eval();
        observation_history_.segment(0, (obs_history_num_ - 1) * obs_dim_) = obs_history_record;
        observation_history_.segment((obs_history_num_ - 1) * obs_dim_, obs_dim_) =
            current_observation_;

        const std::vector<int64_t> state_shape = {1, policy_input_dim_};
        auto state_tensor = Ort::Value::CreateTensor<float>(
            memory_info_, observation_history_.data(), observation_history_.size(),
            state_shape.data(), state_shape.size());
        ort_inputs_.clear();
        ort_inputs_.push_back(std::move(state_tensor));

        auto output_tensors = session_.Run(
            Ort::RunOptions{nullptr}, input_names_, ort_inputs_.data(), ort_inputs_.size(),
            output_names_, 1);

        float *policy_output = output_tensors[0].GetTensorMutableData<float>();
        Eigen::Map<VecXf> action_output(policy_output, act_dim_);
        action_ = action_output.cwiseMax(action_min_).cwiseMin(action_max_);
        last_action_ = action_;
        VecXf action_norm = action_.cwiseProduct(action_scale_);

        RobotAction ra;
        ra.goal_joint_pos.setZero(act_dim_);
        ra.goal_joint_vel.setZero(act_dim_);
        ra.tau_ff.setZero(act_dim_);
        ra.kp.setZero(act_dim_);
        ra.kd.setZero(act_dim_);
        for (int i = 0; i < act_dim_; i++) {
            const int idx = policy_to_output_idx_[i];
            ra.goal_joint_pos(i) = action_norm(idx) + dof_pos_default_(idx);
            ra.kp(i) = kp_(idx);
            ra.kd(i) = kd_(idx);
        }

        // Smooth arm transition during the first 100 steps
        if (run_cnt_ < 100) {
            VecXf init_goal_pos_delta(8);
            for (int i = 0; i < 8; ++i) {
                const int arm_idx = policy_to_arm_idx_[i];
                init_goal_pos_delta(i) = dof_pos_default_(arm_idx) - obs_joint_pos_(arm_idx);
                if (fabs(init_goal_pos_delta(i)) > 0.01) {
                    init_goal_pos_delta(i) = Sign(init_goal_pos_delta(i)) * 0.01f;
                }
                ra.goal_joint_pos(1 + i) = init_goal_pos_delta(i) + obs_joint_pos_(arm_idx);
            }
        }
        ++run_cnt_;
        return ra;
    }

    /**
     * @brief Set maximum command velocity.
     * @param vel Maximum command velocity vector.
     */
    void SetCmdMaxVel(const Vec3f &vel) {
        for (int i = 0; i < 3; ++i) {
            if (vel(i) < 0) {
                std::cerr << policy_name_ << " max_vel " << i << " set error" << std::endl;
            }
        }
        max_cmd_vel_ = vel;
    }

    /**
     * @brief Get maximum command velocity.
     * @return Maximum command velocity vector.
     */
    Vec3f GetCmdMaxVel() {
        return max_cmd_vel_;
    }

    /**
     * @brief Generate an index permutation between two joint-name orders.
     * @param from Source joint-name order.
     * @param to Target joint-name order.
     * @param default_index Fallback index when a target name is missing.
     * @return Index permutation.
     */
    static std::vector<int> GeneratePermutation(
        const std::vector<std::string> &from,
        const std::vector<std::string> &to,
        int default_index = 0) {
        std::unordered_map<std::string, int> idx_map;
        for (int i = 0; i < static_cast<int>(from.size()); ++i) {
            idx_map[from[i]] = i;
        }

        std::vector<int> perm;
        for (const auto &name : to) {
            auto it = idx_map.find(name);
            if (it != idx_map.end()) {
                perm.push_back(it->second);
            } else {
                perm.push_back(default_index);
            }
        }
        return perm;
    }
};

}  // namespace deep_robotics::dr02_pro
