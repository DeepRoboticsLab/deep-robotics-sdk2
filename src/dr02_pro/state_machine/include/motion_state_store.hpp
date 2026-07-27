/**
 * @file motion_state_store.hpp
 * @brief Thread-safe motion-state storage for the state machine.
 * @author DEEPRobotics
 * @date 2026-06-27
 *
 * @copyright Copyright (c) 2026 DEEPRobotics
 *
 */
#pragma once

#include <atomic>

#include "types.h"

namespace deep_robotics::dr02_pro {

class MotionStateStore {
public:
    MotionStateStore() : current_state_(RobotMotionState::Idle) {}

    /**
     * @brief Store the current motion state.
     * @param state Motion state to store.
     */
    void SetState(RobotMotionState state) { current_state_.store(state, std::memory_order_release); }

    /**
     * @brief Load the current motion state.
     * @return Current motion state.
     */
    RobotMotionState GetState() const { return current_state_.load(std::memory_order_acquire); }

private:
    std::atomic<RobotMotionState> current_state_;
};

}  // namespace deep_robotics::dr02_pro
