/**
 * @file common_types.hpp
 * @brief Common Eigen vector and matrix aliases.
 * @author DEEPRobotics
 * @date 2026-06-27
 *
 * @copyright Copyright (c) 2026 DEEPRobotics
 *
 */
#pragma once

#include <Eigen/Dense>

namespace deep_robotics::common {

using Vec2f = Eigen::Vector2f;
using Vec3f = Eigen::Vector3f;
using Vec3d = Eigen::Vector3d;
using Vec4f = Eigen::Vector4f;
using Vec4d = Eigen::Vector4d;
using Vec6f = Eigen::Matrix<float, 6, 1>;
using Vec6d = Eigen::Matrix<double, 6, 1>;
using VecXf = Eigen::VectorXf;
using VecXd = Eigen::VectorXd;

using Mat3f = Eigen::Matrix3f;
using Mat3d = Eigen::Matrix3d;
using MatXf = Eigen::MatrixXf;
using MatXd = Eigen::MatrixXd;

}  // namespace deep_robotics::common
