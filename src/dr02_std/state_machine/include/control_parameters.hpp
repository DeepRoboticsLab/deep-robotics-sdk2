/**
 * @file control_parameters.hpp
 * @brief DR02 Std control parameter definitions.
 * @author DEEPRobotics
 * @date 2026-08-06
 *
 * @copyright Copyright (c) 2026 DEEPRobotics
 *
 */
#pragma once

#include <iostream>
#include <string>

#include "types.h"

namespace deep_robotics::dr02_std {

class ControlParameters {
 private:
  void GenerateDR2StdParameters();
 public:
  ControlParameters(RobotName robot_name) {
    if (robot_name == RobotName::DR2STD)
      GenerateDR2StdParameters();
    else {
      std::cerr << "Unsupported robot type for DR02 Std control parameters." << std::endl;
    }
  }
  ~ControlParameters() {}

  /**
   * @brief robot dof numbers
   */
  int dof_num_;

  /**
   * @brief waist dof numbers
   */
  int waist_dof_num_;

  /**
   * @brief arm dof numbers
   */
  int arm_dof_num_;

  /**
   * @brief leg dof numbers
   */
  int leg_dof_num_;

  /**
   * @brief neck dof numbers
   */
  int neck_dof_num_;

  /**
   * @brief robot link length
   */
  float body_len_x_, body_len_y_;
  float hip_len_, thigh_len_, shank_len_;

  /**
   * @brief stand height configure
   */
  float pre_height_, stand_height_;

  /**
   * @brief one leg joint PD gain
   */
  Vec3f swing_leg_kp_, swing_leg_kd_;

  /**
   * @brief joint position limitation
   */
  Vec3f fl_joint_lower_, fl_joint_upper_;

  /**
   * @brief joint velocity limitation
   */
  VecXf joint_vel_limit_;

  /**
   * @brief joint torque limitation
   */
  VecXf torque_limit_;

  /**
   * @brief stand up duration
   */
  float stand_duration_ = 1.5;

  /**
   * @brief lie down duration
   */
  float liedown_duration_ = 2.0;

  /**
   * @brief wheel vel limit for wheel-legged robot
   */
  float wheel_vel_limit_ = 100;

  /**
   * @brief wheel link length to shank
   */
  float wheel_link_len_ = 0.040575;

  /**
   * @brief arm joint position limitation
   */
  VecXf arm_joint_lower_, arm_joint_upper_;

  /**
   * @brief arm joint kp
   */
  VecXf arm_kp_;

  /**
   * @brief arm joint kd
   */
  VecXf arm_kd_;

  /**
   * @brief arm link length
   */
  VecXf arm_link_len_;

  /**
   * @brief hand link length
   */
  float hand_link_len_;

  /**
   * @brief human leg joint position limitation
   */
  Vec6f leg_joint_lower_, leg_joint_upper_;

  /**
   * @brief human leg joint kp
   */
  Vec6f leg_kp_;

  /**
   * @brief human leg joint kd
   */
  Vec6f leg_kd_;

  /**
   * @brief human leg link length
   */
  Vec6f leg_link_len_;

  /**
   * @brief human waist joint position limitation
   */
  VecXf waist_joint_lower_, waist_joint_upper_;

  /**
   * @brief human waist joint kp
   */
  VecXf waist_kp_;

  /**
   * @brief human waist joint kd
   */
  VecXf waist_kd_;

  /**
   * @brief human neck joint position limitation
   */
  Vec2f neck_joint_lower_, neck_joint_upper_;

  /**
   * @brief human neck joint kp
   */
  Vec2f neck_kp_;

  /**
   * @brief human neck joint kd
   */
  Vec2f neck_kd_;
  /**
   * @brief policy path
   */
  std::string common_policy_path_;
  Vec3f common_policy_p_gain_, common_policy_d_gain_;
  VecXf humanleg_policy_p_gain_, humanleg_policy_d_gain_;

};

void ControlParameters::GenerateDR2StdParameters() {
    dof_num_ = 21;
    waist_dof_num_ = 1;
    arm_dof_num_ = 4;
    leg_dof_num_ = 6;
    neck_dof_num_ = 0;

    pre_height_ = 0.45;
    stand_height_ = 0.75;
    stand_duration_ = 8.;
    liedown_duration_ = 5.;

    arm_joint_lower_ = VecXf::Zero(arm_dof_num_);
    arm_joint_upper_ = VecXf::Zero(arm_dof_num_);
    arm_kp_ = VecXf::Zero(arm_dof_num_);
    arm_kd_ = VecXf::Zero(arm_dof_num_);
    arm_link_len_ = VecXf::Zero(arm_dof_num_);
    waist_joint_lower_ = VecXf::Zero(waist_dof_num_);
    waist_joint_upper_ = VecXf::Zero(waist_dof_num_);
    waist_kp_ = VecXf::Zero(waist_dof_num_);
    waist_kd_ = VecXf::Zero(waist_dof_num_);
    joint_vel_limit_ = VecXf::Zero(dof_num_);
    torque_limit_ = VecXf::Zero(dof_num_);

    joint_vel_limit_ << 19.38, 19.38, 19.38, 19.38, 19.38, 19.38, 19.38, 19.38, 19.38, 20., 19.38, 19.38, 20., 19.38,
        23.76, 20., 19.38, 19.38, 20., 19.38, 23.76;

    torque_limit_ << 107., 107., 107., 107., 107., 107., 107., 107., 107., 413., 107., 107., 413., 107., 31., 413.,
        107., 107., 413., 107., 31.;

    waist_joint_lower_ << -3.6652;
    waist_joint_upper_ << 1.7453;
    waist_kp_ << 600.;
    waist_kd_ << 6.;

    arm_joint_lower_ << -3.3161, -0.43633, -2.9671, -0.767945;
    arm_joint_upper_ << 1.5708, 3.5799, 2.9671, 1.74533;

    arm_kp_ << 200., 200., 200., 200.;
    arm_kd_ << 5., 5., 5., 5.;
    arm_link_len_ << 0.10135, 0.1486, 0.1164, 0.34;
    hand_link_len_ = 0.15;

    leg_joint_lower_ << -2.7925, -0.4363, -0.5236, -0.1745, -1.0472, -0.61087;
    leg_joint_upper_ << 1.5708, 2.618, 3.6652, 2.5307, 0.7854, 0.61087;

    leg_kp_ << 1800.0, 600.0, 600.0, 1800.0, 600.0, 90.0;
    leg_kd_ << 24.0, 6.0, 6.0, 24.0, 6.0, 3.0;
    leg_link_len_ << 0.0483, 0.1635, 0.2565, 0.410, 0.04665, 0.0;


    VecXf kp_temp;
    kp_temp = VecXf::Zero(21);
    kp_temp << 150.0, 150.0, 100.0, 150.0, 100.0, 30.0, 150.0, 150.0, 100.0, 150.0, 100.0, 30.0, 150.0, 100.0, 100.0,
        100.0, 100.0, 100.0, 100.0, 100.0, 100.0;
    humanleg_policy_p_gain_ = kp_temp * 0.9;
    humanleg_policy_d_gain_ = VecXf::Zero(21);
    humanleg_policy_d_gain_ << 3.75, 3.75, 2.5, 3.75, 2.5, 1.0, 3.75, 3.75, 2.5, 3.75, 2.5, 1.0, 3.75, 2.5, 2.5, 2.5,
        2.5, 2.5, 2.5, 2.5, 2.5;
}

}  // namespace deep_robotics::dr02_std
