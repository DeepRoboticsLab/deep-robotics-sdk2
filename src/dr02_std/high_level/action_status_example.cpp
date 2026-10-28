/**
 * @file action_status_example.cpp
 * @brief High-level action execution status subscriber example.
 * @author DEEPRobotics
 * @date 2026-08-25
 *
 * @copyright Copyright (c) 2026 DEEPRobotics
 *
 */
#include <cstdint>
#include <functional>
#include <memory>

#include "drdds/msg/motion_state.hpp"
#include "rclcpp/rclcpp.hpp"

namespace {

// Execution stages encoded in bits 7..0 of /ACTION_STATUS data.state.
enum ActionExecState : uint8_t {
    ACTION_STATE_IDLE = 0,     // No action is running.
    ACTION_STATE_PLAYING = 1,  // The action is running.
    ACTION_STATE_ERROR = 2,    // The action failed; check the error code.
    ACTION_STATE_DONE = 3,     // Completed; does not imply a return to the default gait.
};

// Error codes encoded in bits 15..8 of /ACTION_STATUS data.state.
enum ActionExecError : uint8_t {
    ACTION_ERROR_OK = 0,                  // No error.
    ACTION_ERROR_BUSY = 1,                // Another upper-body action is running.
    ACTION_ERROR_INVALID_COMMAND = 2,     // Invalid or unsupported command.
    ACTION_ERROR_OUTPUT_FAILURE = 3,      // Execution, communication or feedback failure, or execution timeout.
    ACTION_ERROR_UNSUPPORTED_MODE = 4,    // The current motion state or gait is unsupported.
    ACTION_ERROR_IMU_NOT_READY = 5,       // IMU data is unavailable or has timed out.
    ACTION_ERROR_FEEDBACK_NOT_READY = 6,  // Required action feedback is unavailable or has timed out.
};

}  // namespace

class ActionStatusSubscriber : public rclcpp::Node {
public:
    ActionStatusSubscriber() : Node("action_status_example") {
        action_status_sub_ = create_subscription<drdds::msg::MotionState>(
            "/ACTION_STATUS", 10, std::bind(&ActionStatusSubscriber::HandleActionStatus, this, std::placeholders::_1));
        RCLCPP_INFO(get_logger(), "Subscribed /ACTION_STATUS. Press Ctrl+C to exit.");
    }

private:
    /**
     * @brief Decode action ID, error and execution state from /ACTION_STATUS.
     * @param msg Received packed action-status message.
     */
    void HandleActionStatus(const drdds::msg::MotionState::SharedPtr msg) {
        const uint32_t packed = static_cast<uint32_t>(msg->data.state);
        if (received_ && packed == last_status_) {
            return;
        }

        received_ = true;
        last_status_ = packed;

        // Bits 31..16: action ID; bits 15..8: error; bits 7..0: execution state.
        const uint32_t action_id = (packed >> 16U) & 0xffffU;
        const uint32_t error = (packed >> 8U) & 0xffU;
        const uint32_t state = packed & 0xffU;
        RCLCPP_INFO(get_logger(), "action=%u state=%u error=%u", action_id, state, error);
    }

    bool received_ = false;
    uint32_t last_status_ = 0;
    rclcpp::Subscription<drdds::msg::MotionState>::SharedPtr action_status_sub_;
};

int main(int argc, char* argv[]) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<ActionStatusSubscriber>());
    rclcpp::shutdown();
    return 0;
}
