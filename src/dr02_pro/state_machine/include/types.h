/**
 * @file types.h
 * @brief Shared state-machine data types.
 * @author DEEPRobotics
 * @date 2026-06-27
 *
 * @copyright Copyright (c) 2026 DEEPRobotics
 *
 */
#pragma once

#include <cstdint>

#include "common_types.hpp"

namespace deep_robotics::dr02_pro {

using namespace deep_robotics::common;

constexpr float gravity = 9.815f;

struct RobotBasicState {
  Vec3f base_rpy;
  Vec4f base_quat;
  Mat3f base_rot_mat;
  Vec3f base_omega;
  MatXf flt_base_omega_mat;
  Vec3f base_acc;
  MatXf flt_base_acc_mat;
  VecXf joint_pos;
  VecXf joint_vel;
  MatXf flt_joint_vel_mat;
  VecXf joint_tau;
};

struct RobotAction {
  VecXf goal_joint_pos;
  VecXf goal_joint_vel;
  VecXf kp;
  VecXf kd;
  VecXf tau_ff;

  /**
   * @brief Convert the action fields into a joint command matrix.
   * @return Joint command matrix.
   */
  MatXf ConvertToMat() {
    MatXf res(goal_joint_pos.rows(), 5);
    res.col(0) = kp;
    res.col(1) = goal_joint_pos;
    res.col(2) = kd;
    res.col(3) = goal_joint_vel;
    res.col(4) = tau_ff;
    return res;
  }
};

struct UserCommand {
  double time_stamp;
  int safe_control_mode;
  uint8_t target_mode;
  float forward_vel_scale;
  float side_vel_scale;
  float turning_vel_scale;
};

enum RobotName {
    DR2PRO = 0,
};

enum RobotMotionState {
    Idle = 0,
    JointDamping = 1,
    ZeroPos = 2,
    RLControl = 3,
};

enum StateName {
    kInvalid = -1,
    kIdle = RobotMotionState::Idle,
    kJointDamping = RobotMotionState::JointDamping,
    kZeroPos = RobotMotionState::ZeroPos,
    kRLControl = RobotMotionState::RLControl,
};

enum KeyCode {
    L1,
    L2,
    R1,
    R2,
    UNKNOWN
};

enum CommandIndex {
  kIndexDisable = 1,
  kIndexEnable = 2,
  kIndexMotorControl = 4,
  kIndexErrorReset = 17
};

}  // namespace deep_robotics::dr02_pro
