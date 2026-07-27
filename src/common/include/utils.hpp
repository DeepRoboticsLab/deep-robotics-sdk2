/**
 * @file utils.hpp
 * @brief Math and interpolation utility functions.
 * @author DEEPRobotics
 * @date 2026-06-27
 *
 * @copyright Copyright (c) 2026 DEEPRobotics
 *
 */
#pragma once

#include <cmath>
#include <iostream>

#include "common_types.hpp"

namespace deep_robotics::common {

inline float Deg2Rad(float deg) { return deg / 180.0f * static_cast<float>(M_PI); }

inline float Rad2Deg(float rad) { return rad / static_cast<float>(M_PI) * 180.0f; }

/**
 * @brief Clamp a floating-point value in place.
 * @param num Value to clamp.
 * @param low Lower bound.
 * @param up Upper bound.
 */
inline void ClipNumber(float& num, float low, float up) {
    if (low > up) std::cerr << "error clip" << std::endl;
    if (num < low) num = low;
    if (num > up) num = up;
}

/**
 * @brief Return a clamped copy of a floating-point value.
 * @param data Value to clamp.
 * @param low Lower bound.
 * @param high Upper bound.
 * @return Clamped value.
 */
inline float RetLimitNumber(float data, float low, float high) {
    float ret = 0.0f;
    if (low > high) {
        std::cerr << "error limit range" << std::endl;
        ret = data;
        return ret;
    }
    if (data > high) ret = high;
    else if (data < low) ret = low;
    return ret;
}

/**
 * @brief Convert roll-pitch-yaw angles to a rotation matrix.
 * @param rpy Roll, pitch, and yaw angles.
 * @return Rotation matrix.
 */
inline Mat3f RpyToRm(const Vec3f& rpy) {
    Eigen::AngleAxisf yawAngle(rpy(2), Vec3f::UnitZ());
    Eigen::AngleAxisf pitchAngle(rpy(1), Vec3f::UnitY());
    Eigen::AngleAxisf rollAngle(rpy(0), Vec3f::UnitX());
    Eigen::Quaternion<float> q = yawAngle * pitchAngle * rollAngle;
    return q.matrix();
}

/**
 * @brief Normalize an angle to the [-pi, pi] range.
 * @param angle Input angle.
 * @return Normalized angle.
 */
inline float NormalizeAngle(float angle) {
    float result = std::fmod(angle, 2 * static_cast<float>(M_PI));
    if (result < 0) {
        result += 2 * static_cast<float>(M_PI);
    }
    if (result > static_cast<float>(M_PI)) {
        result -= 2 * static_cast<float>(M_PI);
    }
    return result;
}

/**
 * @brief Return the sign of a floating-point value.
 * @param i Input value.
 * @return -1, 0, or 1 as a floating-point value.
 */
inline float Sign(const float& i) {
    if (i > 0) {
        return 1.0f;
    } else if (i == 0) {
        return 0.0f;
    } else {
        return -1.0f;
    }
}

/**
 * @brief Evaluate cubic spline position.
 * @param x0 Initial position.
 * @param v0 Initial velocity.
 * @param xf Final position.
 * @param vf Final velocity.
 * @param t Current time.
 * @param T Total interpolation time.
 * @return Interpolated position.
 */
inline float GetCubicSplinePos(float x0, float v0, float xf, float vf, float t, float T) {
    if (t >= T) return xf;
    float a, b, c, d;
    d = x0;
    c = v0;
    a = (vf * T - 2 * xf + v0 * T + 2 * x0) / std::pow(T, 3);
    b = (3 * xf - vf * T - 2 * v0 * T - 3 * x0) / std::pow(T, 2);
    return a * std::pow(t, 3) + b * std::pow(t, 2) + c * t + d;
}

/**
 * @brief Evaluate cubic spline velocity.
 * @param x0 Initial position.
 * @param v0 Initial velocity.
 * @param xf Final position.
 * @param vf Final velocity.
 * @param t Current time.
 * @param T Total interpolation time.
 * @return Interpolated velocity.
 */
inline float GetCubicSplineVel(float x0, float v0, float xf, float vf, float t, float T) {
    if (t >= T) return 0;
    float a, b, c;
    c = v0;
    a = (vf * T - 2 * xf + v0 * T + 2 * x0) / std::pow(T, 3);
    b = (3 * xf - vf * T - 2 * v0 * T - 3 * x0) / std::pow(T, 2);
    return 3.0f * a * std::pow(t, 2) + 2.0f * b * t + c;
}

}  // namespace deep_robotics::common
