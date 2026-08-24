/**
 * @file developer_mode_example.cpp
 * @brief Developer mode enter and exit example.
 * @author DEEPRobotics
 * @date 2026-08-13
 *
 * @copyright Copyright (c) 2026 DEEPRobotics
 *
 */
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "drdds/msg/motion_info.hpp"
#include "drdds/msg/motion_state.hpp"
#include "drdds/srv/std_srv_string.hpp"
#include "nlohmann/json.hpp"
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

namespace {

enum class RobotMotionState : int32_t {
    Idle = 0x0,                    // Zero torque, idle state.
    JointDamping = 0x2,            // Soft emergency stop, damping state.
    RLControl = 0x11,              // RL control state.
    SuspendedStand = 0x20006,      // Suspended stand completed; RL control can be entered.
};

enum class DeveloperMode : uint32_t {
    None = 0,       // Developer access enabled without a selected control mode.
    UpperBody = 1,  // Upper-body joint control.
    WholeBody = 2,  // Whole-body joint control.
    HighLevel = 3,  // High-level motion control.
};

constexpr auto kServiceTimeout = std::chrono::seconds(5);
constexpr auto kStateTimeout = std::chrono::seconds(60);

/**
 * @brief Return the display name of a developer control mode.
 * @param mode Developer control mode.
 * @return Mode name used in logs.
 */
const char* ModeName(DeveloperMode mode) {
    switch (mode) {
        case DeveloperMode::None:
            return "none";
        case DeveloperMode::UpperBody:
            return "upper_body";
        case DeveloperMode::WholeBody:
            return "whole_body";
        case DeveloperMode::HighLevel:
            return "high_level";
        default:
            return "unknown";
    }
}

/**
 * @brief Return the display name of a motion state.
 * @param state Robot motion state.
 * @return Motion-state name used in logs.
 */
const char* MotionStateName(RobotMotionState state) {
    switch (state) {
        case RobotMotionState::Idle:
            return "Idle";
        case RobotMotionState::JointDamping:
            return "JointDamping";
        case RobotMotionState::RLControl:
            return "RLControl";
        case RobotMotionState::SuspendedStand:
            return "SuspendedStand";
        default:
            return "unknown";
    }
}

/**
 * @brief Print command-line usage.
 * @param program Program name.
 */
void PrintUsage(const char* program) {
    std::cerr << "Usage:\n"
              << "  " << program << " high_level\n"
              << "  " << program << " whole_body\n"
              << "  " << program << " upper_body\n"
              << "  " << program << " exit\n"
              << "  " << program << " damping_exit\n";
}

}  // namespace

/**
 * @brief Enter or exit a real-robot developer control mode.
 */
class DeveloperModeExample : public rclcpp::Node {
public:
    /**
     * @brief Create the service client, status subscribers, and motion-state publisher.
     */
    DeveloperModeExample() : Node("developer_mode_example") {
        developer_mode_client_ = create_client<drdds::srv::StdSrvString>("/DEVELOPER_MODE");
        developer_mode_status_sub_ = create_subscription<std_msgs::msg::String>(
            "/DEVELOPER_MODE_STATUS", 10,
            std::bind(&DeveloperModeExample::HandleDeveloperModeStatus, this, std::placeholders::_1));
        motion_info_sub_ = create_subscription<drdds::msg::MotionInfo>(
            "/MOTION_INFO", 10,
            std::bind(&DeveloperModeExample::HandleMotionInfo, this, std::placeholders::_1));
        motion_state_pub_ = create_publisher<drdds::msg::MotionState>("/MOTION_STATE", 10);
    }

private:
    using DeveloperModeService = drdds::srv::StdSrvString;

    /**
     * @brief Cache the latest developer-mode status reported by robot-server.
     * @param msg JSON status from /DEVELOPER_MODE_STATUS.
     */
    void HandleDeveloperModeStatus(const std_msgs::msg::String::SharedPtr msg) {
        try {
            const auto status = nlohmann::json::parse(msg->data);
            if (!status.contains("Enabled") || !status.contains("Mode") ||
                !status["Enabled"].is_boolean() || !status["Mode"].is_number_unsigned()) {
                RCLCPP_WARN(get_logger(), "Invalid /DEVELOPER_MODE_STATUS fields");
                return;
            }

            {
                std::lock_guard<std::mutex> lock(status_mutex_);
                developer_mode_status_received_ = true;
                developer_mode_enabled_ = status["Enabled"].get<bool>();
                developer_mode_ = static_cast<DeveloperMode>(status["Mode"].get<uint32_t>());
            }
            status_cv_.notify_all();
        } catch (const nlohmann::json::exception& error) {
            RCLCPP_WARN(get_logger(), "Invalid /DEVELOPER_MODE_STATUS JSON: %s", error.what());
        }
    }

    /**
     * @brief Cache the latest motion state reported by motion control.
     * @param msg Motion information from /MOTION_INFO.
     */
    void HandleMotionInfo(const drdds::msg::MotionInfo::SharedPtr msg) {
        {
            std::lock_guard<std::mutex> lock(motion_mutex_);
            motion_state_received_ = true;
            motion_state_ = static_cast<RobotMotionState>(msg->data.motion_state.state);
        }
        motion_cv_.notify_all();
    }

    /**
     * @brief Send one JSON command to /DEVELOPER_MODE.
     * @param command Service request JSON.
     * @return true when robot-server returns success.
     */
    bool CallDeveloperMode(const std::string& command) {
        if (!developer_mode_client_->wait_for_service(kServiceTimeout)) {
            RCLCPP_ERROR(get_logger(), "Service /DEVELOPER_MODE is not available");
            return false;
        }

        auto request = std::make_shared<DeveloperModeService::Request>();
        request->command = command;
        auto future = developer_mode_client_->async_send_request(request);
        if (future.wait_for(kServiceTimeout) != std::future_status::ready) {
            RCLCPP_ERROR(get_logger(), "Timed out calling /DEVELOPER_MODE: %s", command.c_str());
            return false;
        }

        const auto response = future.get();
        RCLCPP_INFO(get_logger(), "/DEVELOPER_MODE result: %s", response->result.c_str());
        if (response->result != "success") {
            RCLCPP_ERROR(get_logger(), "Mode operation was rejected: %s", response->result.c_str());
            return false;
        }
        return true;
    }

    /**
     * @brief Wait until /MOTION_INFO reports the requested motion state.
     * @param expected_state Requested motion state.
     * @return true when the state is observed before timeout.
     */
    bool WaitForMotionState(RobotMotionState expected_state) {
        const auto deadline = std::chrono::steady_clock::now() + kStateTimeout;
        while (rclcpp::ok() && std::chrono::steady_clock::now() < deadline) {
            std::unique_lock<std::mutex> lock(motion_mutex_);
            if (motion_cv_.wait_for(lock, std::chrono::milliseconds(100), [&] {
                    return motion_state_received_ && motion_state_ == expected_state;
                })) {
                RCLCPP_INFO(get_logger(), "Motion state: %s (0x%x)", MotionStateName(expected_state),
                            static_cast<unsigned>(expected_state));
                return true;
            }
        }
        RCLCPP_ERROR(get_logger(), "Timed out waiting for motion state %s (0x%x)", MotionStateName(expected_state),
                     static_cast<unsigned>(expected_state));
        return false;
    }

    /**
     * @brief Wait until /DEVELOPER_MODE_STATUS reports the requested mode.
     * @param expected_enabled Expected outer developer-access state.
     * @param expected_mode Expected developer control mode.
     * @return true when the status is observed before timeout.
     */
    bool WaitForDeveloperMode(bool expected_enabled, DeveloperMode expected_mode) {
        const auto deadline = std::chrono::steady_clock::now() + kStateTimeout;
        while (rclcpp::ok() && std::chrono::steady_clock::now() < deadline) {
            std::unique_lock<std::mutex> lock(status_mutex_);
            if (status_cv_.wait_for(lock, std::chrono::milliseconds(100), [&] {
                    return developer_mode_status_received_ && developer_mode_enabled_ == expected_enabled &&
                           developer_mode_ == expected_mode;
                })) {
                RCLCPP_INFO(get_logger(), "Developer mode status: enabled=%s mode=%s (%u)",
                            expected_enabled ? "true" : "false", ModeName(expected_mode),
                            static_cast<uint32_t>(expected_mode));
                return true;
            }
        }
        RCLCPP_ERROR(get_logger(), "Timed out waiting for developer mode: enabled=%s mode=%u",
                     expected_enabled ? "true" : "false", static_cast<uint32_t>(expected_mode));
        return false;
    }

    /**
     * @brief Read the latest developer-mode status before a normal exit.
     * @param enabled Current outer developer-access state.
     * @param mode Current developer control mode.
     * @return true after a valid status has been received.
     */
    bool GetDeveloperModeStatus(bool& enabled, DeveloperMode& mode) {
        const auto deadline = std::chrono::steady_clock::now() + kStateTimeout;
        while (rclcpp::ok() && std::chrono::steady_clock::now() < deadline) {
            std::unique_lock<std::mutex> lock(status_mutex_);
            if (status_cv_.wait_for(lock, std::chrono::milliseconds(100), [&] {
                    return developer_mode_status_received_;
                })) {
                enabled = developer_mode_enabled_;
                mode = developer_mode_;
                RCLCPP_INFO(get_logger(), "Current developer mode: enabled=%s mode=%s (%u)",
                            enabled ? "true" : "false", ModeName(mode), static_cast<uint32_t>(mode));
                return true;
            }
        }
        RCLCPP_ERROR(get_logger(), "Timed out waiting for /DEVELOPER_MODE_STATUS");
        return false;
    }

    /**
     * @brief Read the latest motion state before exiting developer mode.
     * @param state Current robot motion state.
     * @return true after a valid state has been received.
     */
    bool GetCurrentMotionState(RobotMotionState& state) {
        const auto deadline = std::chrono::steady_clock::now() + kStateTimeout;
        while (rclcpp::ok() && std::chrono::steady_clock::now() < deadline) {
            std::unique_lock<std::mutex> lock(motion_mutex_);
            if (motion_cv_.wait_for(lock, std::chrono::milliseconds(100), [&] {
                    return motion_state_received_;
                })) {
                state = motion_state_;
                RCLCPP_INFO(get_logger(), "Current motion state: %s (0x%x)", MotionStateName(state),
                            static_cast<unsigned>(state));
                return true;
            }
        }
        RCLCPP_ERROR(get_logger(), "Timed out waiting for /MOTION_INFO");
        return false;
    }

    /**
     * @brief Publish one motion-state request.
     * @param state Requested motion state.
     * @return true when a subscriber is available and the request is published.
     */
    bool PublishMotionState(RobotMotionState state) {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
        while (rclcpp::ok() && motion_state_pub_->get_subscription_count() == 0 &&
               std::chrono::steady_clock::now() < deadline) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        if (motion_state_pub_->get_subscription_count() == 0) {
            RCLCPP_ERROR(get_logger(), "No subscriber found on /MOTION_STATE");
            return false;
        }

        drdds::msg::MotionState msg;
        msg.header.frame_id = 0;
        msg.header.stamp = now();
        msg.data.state = static_cast<int32_t>(state);
        motion_state_pub_->publish(msg);
        RCLCPP_INFO(get_logger(), "Published /MOTION_STATE: %s (0x%x)", MotionStateName(state),
                    static_cast<unsigned>(state));
        return true;
    }

public:
    bool EnterHighLevelMode() {
        RCLCPP_WARN(get_logger(), "Entering High-Level Motion Control Mode on the real robot");

        // Enable developer access before selecting a control mode.
        const nlohmann::json enable_command = {
            {"Action", "enable"},
        };
        if (!CallDeveloperMode(enable_command.dump())) {
            return false;
        }
        if (!WaitForDeveloperMode(true, DeveloperMode::None)) {
            return false;
        }

        // Select high-level motion control.
        const nlohmann::json set_mode_command = {
            {"Action", "set_mode"},
            {"Mode", 3},
        };
        if (!CallDeveloperMode(set_mode_command.dump())) {
            return false;
        }
        if (!WaitForDeveloperMode(true, DeveloperMode::HighLevel)) {
            return false;
        }

        RCLCPP_INFO(get_logger(), "High-Level Motion Control Mode is ready");
        return true;
    }

    bool EnterWholeBodyMode() {
        RCLCPP_WARN(get_logger(), "Entering Whole-Body Joint Control Mode on the real robot");

        // Enable developer access before selecting a control mode.
        const nlohmann::json enable_command = {
            {"Action", "enable"},
        };
        if (!CallDeveloperMode(enable_command.dump())) {
            return false;
        }
        if (!WaitForDeveloperMode(true, DeveloperMode::None)) {
            return false;
        }

        // Select whole-body joint control.
        const nlohmann::json set_mode_command = {
            {"Action", "set_mode"},
            {"Mode", 2},
            {"Frequency", 500},
        };
        if (!CallDeveloperMode(set_mode_command.dump())) {
            return false;
        }
        if (!WaitForDeveloperMode(true, DeveloperMode::WholeBody)) {
            return false;
        }

        RCLCPP_INFO(get_logger(), "Whole-Body Joint Control Mode is ready");
        return true;
    }

    bool EnterUpperBodyMode() {
        RCLCPP_WARN(get_logger(), "Entering Upper-Body Joint Control Mode on the real robot");

        // Enable developer access before preparing the robot motion state.
        const nlohmann::json enable_command = {
            {"Action", "enable"},
        };
        if (!CallDeveloperMode(enable_command.dump())) {
            return false;
        }
        if (!WaitForDeveloperMode(true, DeveloperMode::None)) {
            return false;
        }

        // Enter suspended stand before the robot is lowered to the ground.
        std::cout << "The robot will enter SuspendedStand. Make sure it is safely suspended and both feet "
                     "are off the ground.\n"
                  << "Enter yes to continue, or enter anything else to cancel: ";
        std::string suspension_confirmation;
        if (!std::getline(std::cin, suspension_confirmation) || suspension_confirmation != "yes") {
            RCLCPP_WARN(get_logger(), "Upper-body mode entry cancelled before SuspendedStand");
            return false;
        }
        if (!PublishMotionState(RobotMotionState::SuspendedStand)) {
            return false;
        }
        if (!WaitForMotionState(RobotMotionState::SuspendedStand)) {
            return false;
        }

        std::cout << "The robot is in SuspendedStand. Lower it to the ground and confirm that both feet "
                     "are stable.\n"
                  << "Enter yes to continue to RLControl, or enter anything else to cancel: ";
        std::string confirmation;
        if (!std::getline(std::cin, confirmation) || confirmation != "yes") {
            RCLCPP_WARN(get_logger(), "Upper-body mode entry cancelled before RLControl");
            return false;
        }

        // Enter RL control only after both feet are stable on the ground.
        if (!PublishMotionState(RobotMotionState::RLControl)) {
            return false;
        }
        if (!WaitForMotionState(RobotMotionState::RLControl)) {
            return false;
        }

        // Select upper-body joint control.
        const nlohmann::json set_mode_command = {
            {"Action", "set_mode"},
            {"Mode", 1},
            {"Frequency", 500},
        };
        if (!CallDeveloperMode(set_mode_command.dump())) {
            return false;
        }
        if (!WaitForDeveloperMode(true, DeveloperMode::UpperBody)) {
            return false;
        }

        RCLCPP_INFO(get_logger(), "Upper-Body Joint Control Mode is ready");
        return true;
    }

    bool ExitCurrentMode() {
        bool enabled = false;
        DeveloperMode mode = DeveloperMode::None;
        if (!GetDeveloperModeStatus(enabled, mode)) {
            return false;
        }
        if (!enabled) {
            RCLCPP_INFO(get_logger(), "Developer mode is already disabled");
            return true;
        }

        RobotMotionState motion_state = RobotMotionState::Idle;
        if (!GetCurrentMotionState(motion_state)) {
            return false;
        }
        if (motion_state != RobotMotionState::Idle) {
            switch (mode) {
                case DeveloperMode::None:
                    break;
                case DeveloperMode::HighLevel:
                case DeveloperMode::WholeBody: {
                    RCLCPP_WARN(get_logger(), "Exit will enter JointDamping");

                    std::cout << "The robot will enter JointDamping and may lose active support. Enter yes to continue, "
                                 "or enter anything else to cancel: ";
                    std::string damping_confirmation;
                    if (!std::getline(std::cin, damping_confirmation) || damping_confirmation != "yes") {
                        RCLCPP_INFO(get_logger(), "%s exit cancelled before JointDamping", ModeName(mode));
                        return true;
                    }
                    break;
                }
                case DeveloperMode::UpperBody: {
                    std::cout << "Make sure the upper-body posture is stable. Enter yes to return to RLControl; "
                                 "if the upper body is limp, blocked, or cannot be safely controlled, cancel and use "
                                 "damping_exit instead.\n"
                              << "Enter yes to continue, or enter anything else to cancel: ";
                    std::string rl_control_confirmation;
                    if (!std::getline(std::cin, rl_control_confirmation) || rl_control_confirmation != "yes") {
                        RCLCPP_INFO(
                            get_logger(),
                            "Upper-body exit cancelled before returning to RLControl; use damping_exit if needed");
                        return true;
                    }

                    // Clearing upper-body control makes the lower-level controller return to RLControl.
                    const nlohmann::json set_mode_command = {
                        {"Action", "set_mode"},
                        {"Mode", 0},
                    };
                    if (!CallDeveloperMode(set_mode_command.dump())) {
                        return false;
                    }
                    if (!WaitForDeveloperMode(true, DeveloperMode::None)) {
                        return false;
                    }
                    if (!WaitForMotionState(RobotMotionState::RLControl)) {
                        return false;
                    }

                    std::cout << "The robot has returned to RLControl. Enter yes to continue with JointDamping and "
                                 "exit developer mode, or enter anything else to cancel: ";
                    std::string damping_confirmation;
                    if (!std::getline(std::cin, damping_confirmation) || damping_confirmation != "yes") {
                        RCLCPP_INFO(get_logger(),
                                    "Upper-body exit stopped at RLControl; developer mode remains enabled");
                        return true;
                    }
                    break;
                }
                default:
                    RCLCPP_ERROR(get_logger(), "Unknown developer mode: %u", static_cast<uint32_t>(mode));
                    return false;
            }

            // Return active control modes to Idle before clearing developer control.
            if (mode != DeveloperMode::None) {
                if (!PublishMotionState(RobotMotionState::JointDamping)) {
                    return false;
                }
                if (!WaitForMotionState(RobotMotionState::JointDamping)) {
                    return false;
                }
                if (!WaitForMotionState(RobotMotionState::Idle)) {
                    return false;
                }
            }
        } else {
            RCLCPP_INFO(get_logger(), "Motion state is already Idle; skipping JointDamping");
        }

        // Idempotently clear the current developer control mode.
        const nlohmann::json set_mode_command = {
            {"Action", "set_mode"},
            {"Mode", 0},
        };
        if (!CallDeveloperMode(set_mode_command.dump())) {
            return false;
        }
        if (!WaitForDeveloperMode(true, DeveloperMode::None)) {
            return false;
        }

        // Exit outer developer access after the active control mode has stopped.
        const nlohmann::json disable_command = {
            {"Action", "disable"},
        };
        if (!CallDeveloperMode(disable_command.dump())) {
            return false;
        }
        if (!WaitForDeveloperMode(false, DeveloperMode::None)) {
            return false;
        }
        return true;
    }

    bool DampingExit() {
        bool enabled = false;
        DeveloperMode mode = DeveloperMode::None;
        if (!GetDeveloperModeStatus(enabled, mode)) {
            return false;
        }
        if (!enabled) {
            RCLCPP_INFO(get_logger(), "Developer mode is already disabled");
            return true;
        }

        RobotMotionState motion_state = RobotMotionState::Idle;
        if (!GetCurrentMotionState(motion_state)) {
            return false;
        }

        if (motion_state != RobotMotionState::Idle) {
            RCLCPP_WARN(get_logger(),
                        "Damping exit sends JointDamping immediately. The robot may lose active support and fall.");

            if (!PublishMotionState(RobotMotionState::JointDamping)) {
                return false;
            }
            if (!WaitForMotionState(RobotMotionState::JointDamping)) {
                return false;
            }
            if (!WaitForMotionState(RobotMotionState::Idle)) {
                return false;
            }
        }

        const nlohmann::json set_mode_command = {
            {"Action", "set_mode"},
            {"Mode", 0},
        };
        if (!CallDeveloperMode(set_mode_command.dump())) {
            return false;
        }
        if (!WaitForDeveloperMode(true, DeveloperMode::None)) {
            return false;
        }
        const nlohmann::json disable_command = {
            {"Action", "disable"},
        };
        if (!CallDeveloperMode(disable_command.dump())) {
            return false;
        }
        if (!WaitForDeveloperMode(false, DeveloperMode::None)) {
            return false;
        }
        return true;
    }

private:
    rclcpp::Client<DeveloperModeService>::SharedPtr developer_mode_client_;
    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr developer_mode_status_sub_;
    rclcpp::Subscription<drdds::msg::MotionInfo>::SharedPtr motion_info_sub_;
    rclcpp::Publisher<drdds::msg::MotionState>::SharedPtr motion_state_pub_;

    std::mutex status_mutex_;
    std::condition_variable status_cv_;
    bool developer_mode_status_received_ = false;
    bool developer_mode_enabled_ = false;
    DeveloperMode developer_mode_ = DeveloperMode::None;

    std::mutex motion_mutex_;
    std::condition_variable motion_cv_;
    bool motion_state_received_ = false;
    RobotMotionState motion_state_ = RobotMotionState::Idle;
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        PrintUsage(argv[0]);
        return 1;
    }

    const std::string command = argv[1];
    rclcpp::init(argc, argv);
    auto node = std::make_shared<DeveloperModeExample>();
    rclcpp::executors::SingleThreadedExecutor executor;
    executor.add_node(node);
    std::thread spin_thread([&executor] { executor.spin(); });

    bool success = false;
    if (command == "high_level") {
        success = node->EnterHighLevelMode();
    } else if (command == "whole_body") {
        success = node->EnterWholeBodyMode();
    } else if (command == "upper_body") {
        success = node->EnterUpperBodyMode();
    } else if (command == "exit") {
        success = node->ExitCurrentMode();
    } else if (command == "damping_exit") {
        success = node->DampingExit();
    } else {
        PrintUsage(argv[0]);
    }

    executor.cancel();
    rclcpp::shutdown();
    if (spin_thread.joinable()) {
        spin_thread.join();
    }
    return success ? 0 : 1;
}
