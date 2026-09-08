/**
 * @file mimic_policy_runner.hpp
 * @brief Mimic ONNX policy runner for 29-DOF motion imitation control.
 * @author DEEPRobotics
 * @date 2026-07-29
 *
 * @copyright Copyright (c) 2026 DEEPRobotics
 *
 */

#pragma once

#include "policy_runner_base.hpp"
#include "npz_loader.hpp"
#include <filesystem>
#include <cmath>
#include <chrono>
#include <iostream>
#include <onnxruntime_cxx_api.h>
#include <unordered_map>

namespace deep_robotics::dr02_pro {

/**
 * @class MimicPolicyRunner
 * @brief 29-DOF mimic policy runner using ONNX inference.
 *
 * Robot has 29 controllable joints (no neck). Policy outputs 29-dim action
 * (all joints in policy_order), mapped to 31-dim output_order (robot command
 * order, with neck joints PD-locked to zero).
 *
 * Observation (151-dim, in policy_order):
 *   motion_joint_pos(29) + motion_joint_vel(29) + projected_gravity(3) +
 *   base_omega(3) + joint_pos(29) + joint_vel(29) + last_action(29)
 */
class MimicPolicyRunner : public PolicyRunnerBase {
private:
    // --- Control parameters (in policy_order, 29-dim) ---
    VecXf kp_, kd_;
    VecXf dof_pos_default_;
    float action_scale_;

    // --- Observation scales ---
    float obs_scale_omega_;
    float obs_scale_gravity_;
    float obs_scale_joint_pos_;
    float obs_scale_joint_vel_;

    // --- Dimensions ---
    int obs_dim_, obs_history_num_, act_dim_;
    int dof_dim_;
    int policy_input_dim_;

    // --- Observation/action buffers ---
    VecXf current_observation_;
    VecXf action_, last_action_;
    VecXf obs_joint_pos_, obs_joint_vel_;
    VecXf action_min_, action_max_;

    // --- Joint order mappings ---
    std::vector<int> robot_to_policy_idx_;   ///< robot_order → policy_order
    std::vector<int> policy_to_output_idx_; ///< policy_order → output_order

    // --- Motion data ---
    NpzLoader motion_loader_;
    std::vector<VecXf> motion_joint_pos_;
    std::vector<VecXf> motion_joint_vel_;
    int data_cnt_ = 0;                       ///< current motion frame index
    bool action_completed_ = false;           ///< true when motion data has played through once

    // --- ONNX runtime resources ---
    Ort::SessionOptions session_options_;
    Ort::Session session_{nullptr};
    Ort::MemoryInfo memory_info_{nullptr};
    Ort::Env env_;
    std::vector<Ort::Value> ort_inputs_;

    const char* input_names_[1] = {"obs"};
    const char* output_names_[1] = {"actions"};

    /**
     * @brief Get interpolated motion frame data between adjacent frames.
     *
     * Performs linear interpolation between two consecutive motion frames
     * to produce smooth joint position and velocity.
     *
     * @param scaled_frame  Frame index (fractional part used as interpolation alpha).
     * @param out_pos       Output interpolated joint positions (29-dim).
     * @param out_vel       Output interpolated joint velocities (29-dim).
     */
    void GetScaledFrameData(int scaled_frame, VecXf& out_pos, VecXf& out_vel) {
        int original_size = static_cast<int>(motion_joint_pos_.size());
        if (original_size == 0) {
            out_pos.setZero(act_dim_);
            out_vel.setZero(act_dim_);
            return;
        }

        int frame_a = static_cast<int>(scaled_frame);
        int frame_b = frame_a + 1;
        float alpha = static_cast<float>(scaled_frame) - static_cast<float>(frame_a);

        frame_a = std::max(0, std::min(frame_a, original_size - 1));
        frame_b = std::max(0, std::min(frame_b, original_size - 1));

        if (frame_a == frame_b) {
            out_pos = motion_joint_pos_[frame_a];
            out_vel = motion_joint_vel_[frame_a];
        } else {
            out_pos = motion_joint_pos_[frame_a] * (1.0f - alpha) +
                      motion_joint_pos_[frame_b] * alpha;
            out_vel = motion_joint_vel_[frame_a] * (1.0f - alpha) +
                      motion_joint_vel_[frame_b] * alpha;
        }
    }

public:
    /**
     * @brief Construct a MimicPolicyRunner.
     *
     * Initializes joint order mappings, control parameters, ONNX session,
     * and loads motion data from NPZ.
     *
     * @param policy_name Name used for logging.
     * @param robot_name  Robot identifier (determines parameter layout).
     */
    MimicPolicyRunner(const std::string& policy_name, RobotName robot_name)
        : PolicyRunnerBase(policy_name, robot_name) {

        // --- Joint order definitions (29 DOF, no neck) ---
        // robot_order: hardware order, 31-dim (includes neck_z and neck_y at end)
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

        // policy_order: the order the ONNX model expects for observation/action
        std::vector<std::string> policy_order = {
            "left_hip_y_joint", "right_hip_y_joint", "waist_z_joint",
            "left_hip_x_joint", "right_hip_x_joint", "waist_x_joint",
            "left_hip_z_joint", "right_hip_z_joint", "waist_y_joint",
            "left_knee_joint", "right_knee_joint",
            "left_shoulder_y_joint", "right_shoulder_y_joint",
            "left_ankle_y_joint", "right_ankle_y_joint",
            "left_shoulder_x_joint", "right_shoulder_x_joint",
            "left_ankle_x_joint", "right_ankle_x_joint",
            "left_shoulder_z_joint", "right_shoulder_z_joint",
            "left_elbow_joint", "right_elbow_joint",
            "left_wrist_z_joint", "right_wrist_z_joint",
            "left_wrist_y_joint", "right_wrist_y_joint",
            "left_wrist_x_joint", "right_wrist_x_joint",
        };

        // output_order: same as robot_order (31-dim), policy output mapped back
        std::vector<std::string> output_order = robot_order;

        robot_to_policy_idx_ = GeneratePermutation(robot_order, policy_order);
        policy_to_output_idx_ = GeneratePermutation(policy_order, output_order);

        // --- Dimensions ---
        dof_dim_ = 31;
        obs_dim_ = 151;
        obs_history_num_ = 1;
        act_dim_ = 29;
        policy_input_dim_ = obs_history_num_ * obs_dim_;
        decimation_ = 10;  // 50Hz policy frequency (matches training)

        // --- Observation scales ---
        obs_scale_omega_ = 1.0f;
        obs_scale_gravity_ = 1.0f;
        obs_scale_joint_pos_ = 1.0f;
        obs_scale_joint_vel_ = 1.0f;
        action_scale_ = 0.5f;

        // --- Control parameters in policy_order (29-dim) ---
        kp_ = VecXf::Zero(act_dim_);
        kp_ << 300., 300., 200.,    // left_hip_y, right_hip_y, waist_z
               300., 300., 200.,    // left_hip_x, right_hip_x, waist_x
               300., 300., 200.,    // left_hip_z, right_hip_z, waist_y
               300., 300.,          // left_knee, right_knee
               100., 100.,          // left_shoulder_y, right_shoulder_y
               80., 80.,            // left_ankle_y, right_ankle_y
               100., 100.,          // left_shoulder_x, right_shoulder_x
               30., 30.,            // left_ankle_x, right_ankle_x
               100., 100.,          // left_shoulder_z, right_shoulder_z
               100., 100.,          // left_elbow, right_elbow
               80., 80.,            // left_wrist_z, right_wrist_z
               80., 80.,            // left_wrist_y, right_wrist_y
               80., 80.;            // left_wrist_x, right_wrist_x

        kd_ = VecXf::Zero(act_dim_);
        kd_ << 10., 10., 10.,      // left_hip_y, right_hip_y, waist_z
               10., 10., 3.,        // left_hip_x, right_hip_x, waist_x
               10., 10., 6.,        // left_hip_z, right_hip_z, waist_y
               10., 10.,            // left_knee, right_knee
               5., 5.,              // left_shoulder_y, right_shoulder_y
               3., 3.,              // left_ankle_y, right_ankle_y
               5., 5.,              // left_shoulder_x, right_shoulder_x
               1., 1.,              // left_ankle_x, right_ankle_x
               5., 5.,              // left_shoulder_z, right_shoulder_z
               5., 5.,              // left_elbow, right_elbow
               3., 3.,              // left_wrist_z, right_wrist_z
               3., 3.,              // left_wrist_y, right_wrist_y
               3., 3.;              // left_wrist_x, right_wrist_x

        // dof_pos_default: all zeros (matches rl_deploy reference)
        dof_pos_default_ = VecXf::Zero(act_dim_);

        // --- Buffer allocation ---
        current_observation_ = VecXf::Zero(obs_dim_);
        action_ = VecXf::Zero(act_dim_);
        last_action_ = VecXf::Zero(act_dim_);
        obs_joint_pos_ = VecXf::Zero(act_dim_);
        obs_joint_vel_ = VecXf::Zero(act_dim_);
        action_min_ = VecXf::Constant(act_dim_, -10.0);
        action_max_ = VecXf::Constant(act_dim_, 10.0);

        // --- ONNX session setup ---
        session_options_.SetIntraOpNumThreads(1);
        session_options_.SetGraphOptimizationLevel(ORT_ENABLE_ALL);
        ort_inputs_.reserve(1);

        env_ = Ort::Env(ORT_LOGGING_LEVEL_ERROR, policy_name_.data());
        memory_info_ = Ort::MemoryInfo::CreateCpu(OrtAllocatorType::OrtArenaAllocator,
                                                   OrtMemType::OrtMemTypeDefault);

        try {
            namespace fs = std::filesystem;
            const fs::path base = fs::path(__FILE__).parent_path();
            const std::string policy_path =
                fs::canonical(base / ".." / "policy" / "fist_routine.onnx");
            session_ = Ort::Session(env_, policy_path.c_str(), session_options_);
        } catch (const std::exception& e) {
            std::cerr << "error loading mimic policy as " << policy_name_
                      << "\n" << e.what();
        }

        // --- Load motion data ---
        try {
            namespace fs = std::filesystem;
            const fs::path base = fs::path(__FILE__).parent_path();
            const std::string motion_path =
                fs::canonical(base / ".." / "motion_data" / "fist_routine.npz");
            if (motion_loader_.load(motion_path)) {
                motion_loader_.get_key_data("joint_pos", motion_joint_pos_);
                motion_loader_.get_key_data("joint_vel", motion_joint_vel_);
                std::cout << "[mimic] loaded " << motion_joint_pos_.size()
                          << " motion frames" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "[mimic] motion data not loaded: " << e.what() << std::endl;
        }
    }

    ~MimicPolicyRunner() override {
        std::cout << "MimicPolicyRunner " << policy_name_ << " destroyed" << std::endl;
    }

    /**
     * @brief Display mimic policy configuration information.
     */
    void DisplayPolicyInfo() override {
        std::cout << policy_name_ << " (mimic) network info" << std::endl;
        std::cout << "dim  : " << obs_dim_ << " " << obs_history_num_ << " "
                  << policy_input_dim_ << " " << act_dim_ << std::endl;
        std::cout << "dof  : " << dof_pos_default_.transpose() << std::endl;
        std::cout << "kp   : " << kp_.transpose() << std::endl;
        std::cout << "kd   : " << kd_.transpose() << std::endl;
        std::cout << "motion_frames: " << motion_joint_pos_.size() << std::endl;
    }

    /**
     * @brief Reset all policy runner buffers on state entry.
     */
    void OnEnter() override {
        action_.setZero(act_dim_);
        last_action_.setZero(act_dim_);
        obs_joint_pos_.setZero(act_dim_);
        obs_joint_vel_.setZero(act_dim_);
        current_observation_.setZero(obs_dim_);
        data_cnt_ = 0;
        action_completed_ = false;
        cmd_vel_input_.setZero();
        run_cnt_ = 0;
        std::cout << "[mimic policy runner] on enter" << std::endl;
    }

    /**
     * @brief Execute policy runner exit logic (no-op).
     */
    void OnExit() override {}

    /**
     * @brief Check if the motion data has been fully played once.
     * @return true if action playback is completed.
     */
    bool IsActionCompleted() const { return action_completed_; }

    /**
     * @brief Run mimic policy inference for a robot state and user command.
     *
     * Builds the 151-dim observation from motion data and robot state,
     * runs ONNX inference, and maps the 29-dim action to a 31-dim
     * RobotAction in robot command order.
     *
     * @param ro Current robot basic state (joint data in robot_order, 31-dim).
     * @param uc Current user command (unused in mimic mode).
     * @return Robot action in robot_order (31-dim, neck joints PD-locked).
     */
    RobotAction GetRobotAction(const RobotBasicState& ro,
                               const UserCommand& /*uc*/) override {
        ++data_cnt_;

        // --- Get motion frame data (in policy_order, 29-dim) ---
        int total_frames = static_cast<int>(motion_joint_pos_.size());
        VecXf motion_joint_pos, motion_joint_vel;
        if (total_frames == 0) {
            motion_joint_pos = VecXf::Zero(act_dim_);
            motion_joint_vel = VecXf::Zero(act_dim_);
        } else {
            // Hold last frame when playback reaches the end
            if (data_cnt_ >= total_frames) {
                action_completed_ = true;
                data_cnt_ = total_frames - 1;
            }
            GetScaledFrameData(data_cnt_, motion_joint_pos, motion_joint_vel);
        }

        // --- Build observation (all 29-dim in policy_order) ---
        // Project gravity vector into body frame
        Vec3f base_omega = ro.base_omega * obs_scale_omega_;
        Vec3f projected_gravity =
            ro.base_rot_mat.transpose() * Vec3f(0., 0., -1.) * obs_scale_gravity_;

        // Read joint data from robot_order and remap to policy_order
        VecXf robot_joint_pos_default = VecXf(act_dim_);
        for (int i = 0; i < act_dim_; i++) {
            obs_joint_pos_(i) =
                ro.joint_pos(robot_to_policy_idx_[i]) * obs_scale_joint_pos_;
            obs_joint_vel_(i) =
                ro.joint_vel(robot_to_policy_idx_[i]) * obs_scale_joint_vel_;
            robot_joint_pos_default(i) =
                dof_pos_default_(robot_to_policy_idx_[i]) * obs_scale_joint_pos_;
        }
        // Subtract default pose to get position error
        obs_joint_pos_ = obs_joint_pos_ - robot_joint_pos_default;

        // Assemble 151-dim observation vector
        current_observation_.setZero(obs_dim_);
        current_observation_ << motion_joint_pos,    // 29
                               motion_joint_vel,      // 29
                               projected_gravity,     // 3
                               base_omega,            // 3
                               obs_joint_pos_,        // 29
                               obs_joint_vel_,        // 29
                               last_action_;           // 29

        // --- ONNX inference ---
        if (!session_) {
            std::cerr << "[mimic] ONNX session not loaded, skipping inference"
                      << std::endl;
            action_.setZero(act_dim_);
        } else {
            const std::vector<int64_t> state_shape = {1, policy_input_dim_};
            auto state_tensor = Ort::Value::CreateTensor<float>(
                memory_info_, current_observation_.data(),
                current_observation_.size(),
                state_shape.data(), state_shape.size());
            ort_inputs_.clear();
            ort_inputs_.push_back(std::move(state_tensor));

            auto output_tensors = session_.Run(
                Ort::RunOptions{nullptr}, input_names_, ort_inputs_.data(),
                ort_inputs_.size(), output_names_, 1);

            float* policy_output = output_tensors[0].GetTensorMutableData<float>();
            Eigen::Map<VecXf> action_output(policy_output, act_dim_);
            action_ = action_output.cwiseMax(action_min_).cwiseMin(action_max_);

            // Guard against NaN from inference
            if (action_.array().hasNaN()) {
                std::cerr << "[mimic] action contains NaN!" << std::endl;
                action_.setZero();
            }
        }

        last_action_ = action_;

        // --- Map action from policy_order to output_order (31-dim) ---
        RobotAction ra;
        ra.goal_joint_pos.setZero(dof_dim_);
        ra.goal_joint_vel.setZero(dof_dim_);
        ra.tau_ff.setZero(dof_dim_);
        ra.kp.setZero(dof_dim_);
        ra.kd.setZero(dof_dim_);
        for (int i = 0; i < dof_dim_; i++) {
            const int idx = policy_to_output_idx_[i];
            if (idx >= 0) {
                ra.goal_joint_pos(i) =
                    action_(idx) * action_scale_ + dof_pos_default_(idx);
                ra.kp(i) = kp_(idx);
                ra.kd(i) = kd_(idx);
            }
        }

        ++run_cnt_;
        return ra;
    }

    /**
     * @brief Generate an index permutation between two joint-name orders.
     *
     * For each joint name in @p to, finds its index in @p from.
     * If a name is not found, uses @p default_index instead.
     *
     * @param from Source joint-name order.
     * @param to Target joint-name order.
     * @param default_index Fallback index when a target name is missing.
     * @return Index permutation vector.
     */
    static std::vector<int> GeneratePermutation(
        const std::vector<std::string>& from,
        const std::vector<std::string>& to,
        int default_index = 0) {
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
