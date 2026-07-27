/**
 * @file common_policy_runner.hpp
 * @brief ONNX policy runner for RL control.
 * @author DEEPRobotics
 * @date 2026-06-27
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

class CommonPolicyRunner : public PolicyRunnerBase {
private:
    // Control parameters
    VecXf kp_, kd_;
    VecXf dof_pos_default_;
    Vec3f max_cmd_vel_;
    float action_scale_;
    float omega_scale_;
    float dof_vel_scale_;
    Vec3f cmd_vel_scale_;

    // Observation/action buffers
    int obs_dim_, obs_history_num_, act_dim_;
    int obs_total_dim_;
    VecXf current_observation_, observation_history_, observation_total_;
    VecXf action_, last_action_;
    VecXf obs_joint_pos_, obs_joint_vel_;

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

    const char* input_names_[1] = {"state"};
    const char* output_names_[1] = {"output"};

public:
    CommonPolicyRunner(const std::string& policy_name, RobotName robot_name) : PolicyRunnerBase(policy_name, robot_name) {
        kp_ = VecXf::Zero(21);
        kp_ << 300.0, 300.0,
               200.0,
               300.0, 300.0,
               100.0, 100.0,
               300.0, 300.0,
               100.0, 100.0,
               300.0, 300.0,
               100.0, 100.0,
               80.0, 80.0,
               100.0, 100.0,
               30.0, 30.0;
        kd_ = VecXf::Zero(21);
        kd_ << 10, 10,
               10,
               10, 10,
               5, 5,
               10, 10,
               5, 5,
               10, 10,
               5, 5,
               3, 3,
               5, 5,
               1, 1;

        std::vector<std::string> robot_order;
        //DR2PRO
        robot_order = {
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
            "left_hip_y_joint", "right_hip_y_joint",
            "waist_z_joint",
            "left_hip_x_joint", "right_hip_x_joint",
            "left_shoulder_y_joint", "right_shoulder_y_joint",
            "left_hip_z_joint", "right_hip_z_joint",
            "left_shoulder_x_joint", "right_shoulder_x_joint",
            "left_knee_joint", "right_knee_joint",
            "left_shoulder_z_joint", "right_shoulder_z_joint",
            "left_ankle_y_joint", "right_ankle_y_joint",
            "left_elbow_joint", "right_elbow_joint",
            "left_ankle_x_joint", "right_ankle_x_joint"
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
        action_scale_ = 0.25;
        omega_scale_ = 0.25;
        dof_vel_scale_ = 0.05;
        cmd_vel_scale_ = Vec3f(2., 2., 0.25);
        obs_dim_ = 72;
        obs_history_num_ = 5;
        act_dim_ = 21;
        obs_total_dim_ = obs_dim_ + obs_dim_ * obs_history_num_;

        dof_pos_default_.setZero(act_dim_);
        dof_pos_default_ << -0.4, -0.4, 0, 0, 0, 0, 0, 0., 0., 0, 0, 0.8, 0.8, 0, 0, -0.4, -0.4, 0.5, 0.5, 0, 0;

        max_cmd_vel_.setZero(3);
        session_options_.SetIntraOpNumThreads(1);
        session_options_.SetGraphOptimizationLevel(ORT_ENABLE_ALL);
        ort_inputs_.reserve(1);

        env_ = Ort::Env(ORT_LOGGING_LEVEL_ERROR, policy_name_.data());
        memory_info_ = Ort::MemoryInfo::CreateCpu(OrtAllocatorType::OrtArenaAllocator, OrtMemType::OrtMemTypeDefault);

        try {
            namespace fs = std::filesystem;
            const fs::path base = fs::path(__FILE__).parent_path();
            const std::string policy_path = fs::canonical(base / ".." / "policy" / "policy.onnx");
            session_ = Ort::Session(env_, policy_path.c_str(), session_options_);
        } catch (const std::exception& e) {
            std::cerr << "error loading policy as " << policy_name_ << "\n" << e.what();
        }
    }

    ~CommonPolicyRunner() override {
        std::cout << "CommonPolicyRunner " << policy_name_ << " destroyed" << std::endl;
    }

    /**
     * @brief Display common policy information.
     */
    void DisplayPolicyInfo() override {
        std::cout << policy_name_ << " network test success" << std::endl;
        std::cout << "dim  : " << obs_dim_ << " " << obs_history_num_ << " " << obs_total_dim_ << " " << act_dim_ << std::endl;
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
        observation_total_.setZero(obs_total_dim_);

        run_cnt_ = 0;
        cmd_vel_input_.setZero();
    }

    /**
     * @brief Execute policy runner exit logic.
     */
    void OnExit() override {
    }

    /**
     * @brief Run common policy inference for a robot state and user command.
     * @param ro Current robot basic state.
     * @param uc Current user command.
     * @return Robot action produced by the policy.
     */
    RobotAction GetRobotAction(const RobotBasicState &ro, const UserCommand &uc) override {
        Vec3f user_cmd_vel = Vec3f(uc.forward_vel_scale, uc.side_vel_scale, uc.turning_vel_scale);
        Eigen::Vector3f vel_delta = user_cmd_vel - cmd_vel_input_;
        vel_delta_const_ << 0.02, 0.02, 0.04;
        for (int i = 0; i < 3; ++i) {
            if (fabs(vel_delta(i)) > vel_delta_const_(i)) {
                vel_delta(i) = Sign(vel_delta(i)) * vel_delta_const_(i);
            }
        }
        cmd_vel_input_ += vel_delta;
        Vec3f cmd_vel = cmd_vel_input_.cwiseProduct(max_cmd_vel_);

        current_observation_.setZero(obs_dim_);
        Vec3f project_gravity = ro.base_rot_mat.transpose() * Vec3f(0., 0., -1);

        for (int i = 0; i < act_dim_; i++) {
            obs_joint_pos_(i) = ro.joint_pos(robot_to_policy_idx_[i]);
            obs_joint_vel_(i) = ro.joint_vel(robot_to_policy_idx_[i]);
        }
        current_observation_ << omega_scale_ * ro.base_omega,
                                project_gravity,
                                cmd_vel.cwiseProduct(cmd_vel_scale_),
                                obs_joint_pos_ - dof_pos_default_,
                                dof_vel_scale_ * obs_joint_vel_,
                                last_action_;

        VecXf obs_history_record = observation_history_.segment(obs_dim_, (obs_history_num_ - 1) * obs_dim_).eval();
        observation_history_.segment(0, (obs_history_num_ - 1) * obs_dim_) = obs_history_record;
        observation_history_.segment((obs_history_num_ - 1) * obs_dim_, obs_dim_) = current_observation_;

        observation_total_.segment(0, obs_dim_) = current_observation_;
        observation_total_.segment(obs_dim_, obs_dim_ * obs_history_num_) = observation_history_;

        const std::vector<int64_t> state_shape = {1, obs_total_dim_};
        auto state_tensor = Ort::Value::CreateTensor<float>(
            memory_info_, observation_total_.data(), observation_total_.size(), state_shape.data(), state_shape.size());
        ort_inputs_.clear();
        ort_inputs_.push_back(std::move(state_tensor));

        auto output_tensors = session_.Run(
            Ort::RunOptions{nullptr}, input_names_, ort_inputs_.data(), ort_inputs_.size(), output_names_, 1);
        action_ = Eigen::Map<VecXf>(output_tensors[0].GetTensorMutableData<float>(), act_dim_);

        last_action_ = action_;
        RobotAction ra;
        ra.goal_joint_pos.setZero(act_dim_);
        ra.goal_joint_vel.setZero(act_dim_);
        ra.tau_ff.setZero(act_dim_);
        ra.kp.setZero(act_dim_);
        ra.kd.setZero(act_dim_);
        for (int i = 0; i < act_dim_; i++) {
            const int idx = policy_to_output_idx_[i];
            ra.goal_joint_pos(i) = action_scale_ * action_(idx) + dof_pos_default_(idx);
            ra.kp(i) = kp_(idx);
            ra.kd(i) = kd_(idx);
        }

        if (run_cnt_ < 50) {
            VecXf init_goal_pos_delta(8);
            for (int i = 0; i < 8; ++i) {
                const int arm_idx = policy_to_arm_idx_[i];
                init_goal_pos_delta(i) = dof_pos_default_(arm_idx) - obs_joint_pos_(arm_idx);
                if (fabs(init_goal_pos_delta(i)) > 0.08) {
                    init_goal_pos_delta(i) = Sign(init_goal_pos_delta(i)) * 0.08;
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
    void SetCmdMaxVel(const Vec3f& vel) {
        for (int i = 0; i < 3; ++i) {
            if (vel(i) < 0) {
                std::cerr << policy_name_ << " max_vel " << i << " set error" << std::endl;
            }
        }
        max_cmd_vel_ = vel;
    }

    /**
     * @brief Generate an index permutation between two joint-name orders.
     * @param from Source joint-name order.
     * @param to Target joint-name order.
     * @param default_index Fallback index when a target name is missing.
     * @return Index permutation.
     */
    static std::vector<int> GeneratePermutation(
        const std::vector<std::string>& from,
        const std::vector<std::string>& to,
        int default_index = 0)
    {
        std::unordered_map<std::string, int> idx_map;
        for (int i = 0; i < static_cast<int>(from.size()); ++i) {
            idx_map[from[i]] = i;
        }

        std::vector<int> perm;
        for (const auto& name : to) {
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
