/**
 * @file safe_controller.hpp
 * @brief Runtime safety monitor for the state machine.
 * @author DEEPRobotics
 * @date 2026-06-27
 *
 * @copyright Copyright (c) 2026 DEEPRobotics
 *
 */

/**
 * I. Joint side
 *      1. Joint data abnormal (NaN values)
 *      2. Joint data not updating
 *      3. Joint over-temperature
 * II. IMU
 *      1. IMU data abnormal
 *      2. IMU data not updating
 */

#pragma once

#include <cmath>
#include <chrono>
#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

#include "utils.hpp"
#include "time.hpp"
#include "robot_interface.hpp"
#include "user_command_interface.hpp"

namespace deep_robotics::dr02_std {

union RobotErrorState {
    struct {
        uint32_t imu_num_error : 1;
        uint32_t imu_update_overtime : 1;
        uint32_t motor_heat_warn : 1;
        uint32_t joint_num_error : 1;
        uint32_t joint_update_error : 1;
        uint32_t reserved : 26;
    };
    uint32_t error_code;
};

class SafeController {
private:
    std::shared_ptr<RobotInterface> ri_ptr_;
    LoopTimer loop_timer_{};
    std::thread judge_thread_;
    int run_cnt_ = 0;

    double last_imu_ts_, imu_check_time_;
    bool imu_check_flag_ = true;

    double ri_check_time_, last_ri_ts_;
    VecXf last_joint_pos_, last_joint_vel_, last_joint_tau_;
    std::vector<int> joint_data_same_cnt_;
    bool joint_check_flag_ = true;

    float max_temp_;
    int max_temp_idx_;

    UserCommand* usr_cmd_;
    bool start_thread_flag_ = false;

    double current_time_;

    RobotErrorState robot_error_state_;
    uint32_t last_error_code_;

    /**
     * @brief Check whether base IMU data is updating and numeric.
     * @return true if IMU data is normal; false otherwise.
     */
    bool IsImuDataNormal() {
        double current_imu_ts_ = ri_ptr_->GetImuBaseTimestamp();
        Vec3f rpy = ri_ptr_->GetImuBaseRpy();
        Vec3f acc = ri_ptr_->GetImuBaseAcc();
        Vec3f omg = ri_ptr_->GetImuBaseOmega();
        if (imu_check_flag_) {
            last_imu_ts_ = current_imu_ts_;
            imu_check_time_ = current_time_;
            imu_check_flag_ = false;
        }

        bool res = true;
        robot_error_state_.imu_update_overtime = 0;
        robot_error_state_.imu_num_error = 0;
        if (current_imu_ts_ != last_imu_ts_) {
            last_imu_ts_ = current_imu_ts_;
            imu_check_time_ = current_time_;
        }

        if (current_time_ - imu_check_time_ > 40) {
            robot_error_state_.imu_update_overtime = 1;
            res = false;
            std::cout << (current_time_) << ", " << (imu_check_time_) << std::endl;
        }
        for (int i = 0; i < 3; ++i) {
            if (std::isnan(rpy(i)) || std::isnan(acc(i)) || std::isnan(omg(i))) {
                res = false;
                robot_error_state_.imu_num_error = 1;
            }
        }
        return res;
    }

    /**
     * @brief Check whether joint data is updating and numeric.
     * @return true if joint data is normal; false otherwise.
     */
    bool IsJointDataNormal() {
        double ri_ts_ = ri_ptr_->GetInterfaceTimeStamp();
        VecXf joint_pos = ri_ptr_->GetJointPosition();
        VecXf joint_vel = ri_ptr_->GetJointVelocity();
        VecXf joint_tau = ri_ptr_->GetJointTorque();
        if (joint_check_flag_) {
            joint_check_flag_ = false;
            last_ri_ts_ = ri_ts_;
            last_joint_pos_ = joint_pos;
            last_joint_vel_ = joint_vel;
            last_joint_tau_ = joint_tau;
        }
        bool res = true;
        robot_error_state_.joint_update_error = 0;
        static bool first_flag = false;
        if (last_ri_ts_ != ri_ts_) {
            last_ri_ts_ = ri_ts_;
            ri_check_time_ = current_time_;
            first_flag = true;
        }
        if ((current_time_ - ri_check_time_ > 30) && first_flag) {
            robot_error_state_.joint_update_error = 1;
            res = false;
        }

        robot_error_state_.joint_num_error = 0;
        for (int i = 0; i < joint_pos.size(); ++i) {
            if (std::isnan(joint_pos(i)) || std::isnan(joint_vel(i)) || std::isnan(joint_tau(i))) {
                robot_error_state_.joint_num_error = 1;
                res = false;
            }
            if (joint_pos(i) == last_joint_pos_(i) && joint_vel(i) == last_joint_vel_(i) &&
                joint_tau(i) == last_joint_tau_(i)) {
                joint_data_same_cnt_[i]++;
            } else {
                joint_data_same_cnt_[i] = 0;
            }
            if (joint_data_same_cnt_[i] > 30) {
                std::cout << "joint pos: " << joint_pos[i] << std::endl;
                std::cerr << "joint " << i << " data is not update " << joint_data_same_cnt_[i] << " times at "
                          << ri_ts_ << " | " << current_time_ << std::endl;
                std::cout << "rpy: " << (ri_ptr_->GetImuBaseRpy()).transpose() << std::endl;
            }
            if (joint_data_same_cnt_[i] > 100) {
                robot_error_state_.joint_num_error = 1;
                res = false;
            }
        }
        last_joint_pos_ = joint_pos;
        last_joint_vel_ = joint_vel;
        last_joint_tau_ = joint_tau;
        return res;
    }

    /**
     * @brief Check whether motor temperatures are normal.
     * @return true if motor temperatures are normal; false otherwise.
     */
    bool IsMotorTemperatureNormal() {
        VecXf m_t = ri_ptr_->GetMotorTemperature();
        VecXf::Index max_idx, min_idx;
        float max_temp = m_t.maxCoeff(&max_idx);
        float min_temp = m_t.minCoeff(&min_idx);
        max_temp_ = max_temp;
        max_temp_idx_ = max_idx;

        if (run_cnt_ % 5000 == 0) {
            std::cout << "Motor Temperature: " << m_t.transpose() << std::endl;
            std::cout << "Max&Min:          " << max_idx << " : " << max_temp << "  |  " << min_idx << " : " << min_temp
                      << std::endl;
        }

        // Motor over-temperature protection threshold: 110 degC
        if (max_temp > 110) {
            robot_error_state_.motor_heat_warn = 1;
            return false;
        } else {
            robot_error_state_.motor_heat_warn = 0;
        }

        return true;
    }

    /**
     * @brief Print the current robot error state.
     */
    void PrintRobotErrorState() {
        if (robot_error_state_.imu_num_error) {
            std::cout << "imu_num_error: " << ri_ptr_->GetImuBaseRpy().transpose() << " | "
                      << ri_ptr_->GetImuBaseAcc().transpose() << " | " << ri_ptr_->GetImuBaseOmega().transpose()
                      << std::endl;
        }
        if (robot_error_state_.imu_update_overtime) {
            std::cout << "imu update overtime: " << last_imu_ts_ << std::endl;
        }

        if (robot_error_state_.joint_num_error) {
            std::cout << "joint_data_error : \n"
                      << ri_ptr_->GetJointPosition().transpose() << "\n"
                      << ri_ptr_->GetJointVelocity().transpose() << "\n"
                      << ri_ptr_->GetJointTorque().transpose() << "\n";
        }
        if (robot_error_state_.joint_update_error) {
            std::cout << "joint_update_error : " << std::hex << current_time_ << " " << ri_check_time_ << " "
                      << current_time_ - ri_check_time_ << std::endl;
        }
        if (robot_error_state_.motor_heat_warn) {
            std::cout << "motor_over_heat : " << max_temp_idx_ << " | " << max_temp_ << std::endl;
        }
    }

public:
    /**
     * @brief Construct a safety controller.
     * @param path Reserved configuration path.
     */
    SafeController(const std::string& path) {
        robot_error_state_.error_code = 0;
        last_error_code_ = 0;
    }
    ~SafeController() {}

    /**
     * @brief Set the robot data source.
     * @param ri Robot interface.
     */
    void SetRobotDataSource(std::shared_ptr<RobotInterface> ri) {
        ri_ptr_ = ri;
        joint_data_same_cnt_.resize(ri_ptr_->dof_num_, 0);
    }
    /**
     * @brief Set the user command data source.
     * @param uc User command interface.
     */
    void SetUserCommandDataSource(std::shared_ptr<UserCommandInterface> uc) {
        usr_cmd_ = uc->GetUserCommand();
    }

    /**
     * @brief Start the safety monitor thread.
     */
    void Start() {
        start_thread_flag_ = true;
        judge_thread_ = std::thread(std::bind(&SafeController::Run, this));
    }

    /**
     * @brief Run the safety monitor loop.
     */
    void Run() {
        loop_timer_.Start(std::chrono::milliseconds(2));
        while (start_thread_flag_) {
            if (loop_timer_.Wait() == 0) {
                continue;
            }
            run_cnt_++;
            current_time_ = GetTimestampMs();

            if (!IsJointDataNormal()) {
                usr_cmd_->safe_control_mode = 3;
                std::cout << "Joint data error!" << std::endl;
            }
            if (run_cnt_ % 1000 == 0 && !IsMotorTemperatureNormal()) {
                usr_cmd_->safe_control_mode = 2;
                std::cout << "Motor temperature error!" << std::endl;
            }
            if (!IsImuDataNormal()) {
                usr_cmd_->safe_control_mode = 4;
            }

            if (last_error_code_ != robot_error_state_.error_code) {
                if (robot_error_state_.error_code != 0) {
                    std::cout << "error_code: " << std::hex << robot_error_state_.error_code << std::endl;
                    PrintRobotErrorState();
                }
                last_error_code_ = robot_error_state_.error_code;
            }
        }
    }

    /**
     * @brief Stop the safety monitor thread.
     */
    void Stop() {
        start_thread_flag_ = false;
        if (judge_thread_.joinable()) {
            judge_thread_.join();
        }
    }

    /**
     * @brief Get the current safety error code.
     * @return Safety error code.
     */
    inline uint32_t GetErrorCode() { return robot_error_state_.error_code; }
};

}  // namespace deep_robotics::dr02_std
