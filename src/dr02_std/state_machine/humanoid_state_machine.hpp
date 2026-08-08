/**
 * @file humanoid_state_machine.hpp
 * @brief Humanoid state-machine controller.
 * @author DEEPRobotics
 * @date 2026-06-27
 *
 * @copyright Copyright (c) 2026 DEEPRobotics
 *
 */
#pragma once

#include <chrono>
#include <iostream>
#include <memory>
#include <thread>

#include "time.hpp"
#include "dr2_std_interface.hpp"

#include "state_base.h"
#include "zeropos_state.hpp"
#include "idle_state.hpp"
#include "joint_damping_state.hpp"
#include "rl_control_state.hpp"

#include "safe_controller.hpp"
#include "user_command_interface.hpp"

namespace deep_robotics::dr02_std {
class HumanoidStateMachine {
private:
    std::shared_ptr<StateBase> idle_controller_;
    std::shared_ptr<StateBase> zeropos_controller_;
    std::shared_ptr<StateBase> joint_damping_controller_;
    std::shared_ptr<StateBase> rl_control_controller_;

public:
    int run_cnt_ = 0;
    LoopTimer loop_timer_{};
    std::shared_ptr<MotionStateStore> motion_state_store_;

    std::shared_ptr<UserCommandInterface> uc_ptr_;
    std::shared_ptr<RobotInterface> ri_ptr_;
    std::shared_ptr<ControlParameters> cp_ptr_;
    std::shared_ptr<SafeController> sc_ptr_;
    rclcpp::Node::SharedPtr node_;

    std::shared_ptr<StateBase> current_controller_;
    StateName current_state_name_, next_state_name_;

    std::thread run_thread_;

    const RobotName robot_name_;

    /**
     * @brief Construct a humanoid state machine.
     * @param robot_name Robot name.
     */
    HumanoidStateMachine(RobotName robot_name)
        : robot_name_(robot_name),
          motion_state_store_(std::make_shared<MotionStateStore>()) {}

    ~HumanoidStateMachine() {}

    /**
     * @brief Initialize interfaces, controllers, and the first state.
     */
    void Start() {
        node_ = std::make_shared<rclcpp::Node>("sdk_deploy");
        uc_ptr_ = std::make_shared<UserCommandInterface>(node_);
        uc_ptr_->SetMotionStateStore(motion_state_store_);

        ri_ptr_ = std::make_shared<DR2StdInterface>(node_);
        cp_ptr_ = std::make_shared<ControlParameters>(robot_name_);

        std::cout << "HumanoidInterface finish" << std::endl;

        std::shared_ptr<ControllerData> data_ptr = std::make_shared<ControllerData>();
        data_ptr->ri_ptr = ri_ptr_;
        data_ptr->uc_ptr = uc_ptr_;
        data_ptr->cp_ptr = cp_ptr_;
        data_ptr->motion_state_store = motion_state_store_;

        std::cout << "Control mode: rl_control" << std::endl;

        sc_ptr_ = std::make_shared<SafeController>("");
        sc_ptr_->SetRobotDataSource(ri_ptr_);
        sc_ptr_->SetUserCommandDataSource(uc_ptr_);

        idle_controller_ = std::make_shared<IdleState>(robot_name_, "idle_state", data_ptr);
        zeropos_controller_ = std::make_shared<ZeroPosState>(robot_name_, "zeropos_state", data_ptr);
        joint_damping_controller_ = std::make_shared<JointDampingState>(robot_name_, "joint_damping", data_ptr);
        rl_control_controller_ = std::make_shared<RLControlState>(robot_name_, "rl_control", data_ptr);

        current_controller_ = idle_controller_;
        current_state_name_ = kIdle;
        next_state_name_ = kIdle;

        std::this_thread::sleep_for(std::chrono::seconds(3));  // for safety

        ri_ptr_->Start();
        uc_ptr_->Start();
#ifndef USE_SIMULATION
        sc_ptr_->Start();
#endif
        current_controller_->OnEnter();

        std::cout << "start finish" << std::endl;
    }

    /**
     * @brief Run the state-machine control loop.
     */
    void Run() {
        loop_timer_.Start(std::chrono::milliseconds(2));
        run_thread_ = std::thread(&HumanoidStateMachine::RunThread, this);

        while (rclcpp::ok()) {
            if (loop_timer_.Wait() == 0) {
                continue;
            }

            ri_ptr_->RefreshRobotData();

            current_controller_->Run();

            next_state_name_ = current_controller_->GetNextStateName();

            if (next_state_name_ != current_state_name_) {
                current_controller_->OnExit();
                std::cout << current_controller_->state_name_ << " ------------> ";
                current_controller_ = GetStateControllerPtr(next_state_name_);
                std::cout << current_controller_->state_name_ << std::endl;
                current_controller_->OnEnter();
                current_state_name_ = next_state_name_;
            }
            ++run_cnt_;
        }
    }

    /**
     * @brief Spin the ROS2 node in a background thread.
     */
    void RunThread() {
        if (rclcpp::ok()) {
            rclcpp::spin(node_);
        }
    }

    /**
     * @brief Get the controller object for a state name.
     * @param state_name Requested state name.
     * @return State controller pointer.
     */
    std::shared_ptr<StateBase> GetStateControllerPtr(StateName state_name) {
        switch (state_name) {
            case StateName::kInvalid: {
                return nullptr;
            }
            case StateName::kIdle: {
                return idle_controller_;
            }
            case StateName::kJointDamping: {
                return joint_damping_controller_;
            }
            case StateName::kZeroPos: {
                return zeropos_controller_;
            }
            case StateName::kRLControl: {
                return rl_control_controller_;
            }
            default: {
                std::cerr << "error state name" << std::endl;
                return joint_damping_controller_;
            }
        }
        return nullptr;
    }

    /**
     * @brief Stop the state-machine components.
     */
    void Stop() {
        if (run_thread_.joinable()) {
            run_thread_.join();
        }
        sc_ptr_->Stop();
        uc_ptr_->Stop();
        ri_ptr_->Stop();
    }
};
}  // namespace deep_robotics::dr02_std
