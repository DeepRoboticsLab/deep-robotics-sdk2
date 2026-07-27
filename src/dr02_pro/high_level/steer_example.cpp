/**
 * @file steer_example.cpp
 * @brief High-level steer command example.
 * @author DEEPRobotics
 * @date 2026-06-27
 *
 * @copyright Copyright (c) 2026 DEEPRobotics
 *
 */
#include <cctype>
#include <cstdint>
#include <fcntl.h>
#include <iostream>
#include <memory>
#include <string>
#include <termios.h>
#include <unistd.h>

#include "drdds/msg/steer.hpp"
#include "rclcpp/rclcpp.hpp"
#include "time.hpp"

namespace {

constexpr float kSteerSpeed = 0.6f;
constexpr double kKeyHoldTimeoutMs = 320.0;
constexpr int kPublishHz = 20;
constexpr double kRealSteerDemoDurationSec = 3.0;

/**
 * @brief Print command-line usage.
 * @param program Program name.
 */
void PrintUsage(const char* program) {
    std::cerr << "Usage:\n"
              << "  " << program << "\n"
              << "  " << program << " --real\n\n"
              << "Default mode publishes /STEER from keyboard input.\n"
              << "--real publishes /REAL_STEER with yaw=0.4 for 3 seconds.\n";
}

struct SteerCommand {
    float x = 0.0f;
    float y = 0.0f;
    float yaw = 0.0f;
};

using SteerPublisher = rclcpp::Publisher<drdds::msg::Steer>::SharedPtr;

/**
 * @brief Publish a steer command message.
 * @param node ROS2 node used for timestamps.
 * @param publisher Steer command publisher.
 * @param command Steer command values.
 */
void PublishSteer(const rclcpp::Node::SharedPtr& node, const SteerPublisher& publisher, const SteerCommand& command) {
    static uint64_t frame_id = 0;

    drdds::msg::Steer msg;
    msg.header.frame_id = frame_id++;
    msg.header.stamp = node->now();
    msg.data.x = command.x;
    msg.data.y = command.y;
    msg.data.z = 0.0f;
    msg.data.roll = 0.0f;
    msg.data.pitch = 0.0f;
    msg.data.yaw = command.yaw;

    publisher->publish(msg);
}

class KeyboardSteerInput {
public:
    ~KeyboardSteerInput() { RestoreTerminal(); }

    /**
     * @brief Configure terminal input for keyboard steering.
     * @param logger ROS2 logger for status messages.
     * @return true if terminal setup succeeds; false otherwise.
     */
    bool Start(const rclcpp::Logger& logger) {
        if (!ConfigureTerminal()) {
            return false;
        }
        RCLCPP_INFO(logger, "W/S: forward/back, A/D: left/right, Q/E: turn. Release key to stop.");
        RCLCPP_INFO(logger, "Use Ctrl+C to exit.");
        return true;
    }

    /**
     * @brief Read keyboard input and return the current steer command.
     * @return Current steer command.
     */
    SteerCommand Update() {
        const double now_ms = deep_robotics::common::GetTimestampMs();
        ReadKeyboard(now_ms);
        ApplyTimeout(now_ms);
        return {x_, y_, yaw_};
    }

private:
    bool ConfigureTerminal() {
        if (!isatty(STDIN_FILENO)) {
            std::cerr << "steer_example requires a terminal for keyboard input.\n";
            return false;
        }

        if (tcgetattr(STDIN_FILENO, &old_termios_) != 0) {
            std::cerr << "failed to read terminal settings\n";
            return false;
        }

        termios new_termios = old_termios_;
        new_termios.c_lflag &= ~(ICANON | ECHO);
        if (tcsetattr(STDIN_FILENO, TCSANOW, &new_termios) != 0) {
            std::cerr << "failed to set terminal mode\n";
            return false;
        }

        terminal_configured_ = true;
        old_flags_ = fcntl(STDIN_FILENO, F_GETFL, 0);
        if (old_flags_ == -1 || fcntl(STDIN_FILENO, F_SETFL, old_flags_ | O_NONBLOCK) == -1) {
            RestoreTerminal();
            std::cerr << "failed to set non-blocking input\n";
            return false;
        }

        return true;
    }

    void RestoreTerminal() {
        if (!terminal_configured_) {
            return;
        }

        tcsetattr(STDIN_FILENO, TCSANOW, &old_termios_);
        if (old_flags_ != -1) {
            fcntl(STDIN_FILENO, F_SETFL, old_flags_);
        }
        terminal_configured_ = false;
    }

    void ReadKeyboard(double now_ms) {
        char input = 0;
        while (read(STDIN_FILENO, &input, 1) == 1) {
            const char key = static_cast<char>(std::tolower(static_cast<unsigned char>(input)));
            if (key == 'w') {
                x_ = kSteerSpeed;
                x_last_input_ms_ = now_ms;
                x_active_ = true;
            } else if (key == 's') {
                x_ = -kSteerSpeed;
                x_last_input_ms_ = now_ms;
                x_active_ = true;
            } else if (key == 'a') {
                y_ = kSteerSpeed;
                y_last_input_ms_ = now_ms;
                y_active_ = true;
            } else if (key == 'd') {
                y_ = -kSteerSpeed;
                y_last_input_ms_ = now_ms;
                y_active_ = true;
            } else if (key == 'q') {
                yaw_ = kSteerSpeed;
                yaw_last_input_ms_ = now_ms;
                yaw_active_ = true;
            } else if (key == 'e') {
                yaw_ = -kSteerSpeed;
                yaw_last_input_ms_ = now_ms;
                yaw_active_ = true;
            }
        }
    }

    void ApplyTimeout(double now_ms) {
        if (x_active_ && now_ms - x_last_input_ms_ > kKeyHoldTimeoutMs) {
            x_ = 0.0f;
            x_active_ = false;
        }
        if (y_active_ && now_ms - y_last_input_ms_ > kKeyHoldTimeoutMs) {
            y_ = 0.0f;
            y_active_ = false;
        }
        if (yaw_active_ && now_ms - yaw_last_input_ms_ > kKeyHoldTimeoutMs) {
            yaw_ = 0.0f;
            yaw_active_ = false;
        }
    }

    float x_ = 0.0f;
    float y_ = 0.0f;
    float yaw_ = 0.0f;
    double x_last_input_ms_ = -1e9;
    double y_last_input_ms_ = -1e9;
    double yaw_last_input_ms_ = -1e9;
    bool x_active_ = false;
    bool y_active_ = false;
    bool yaw_active_ = false;
    bool terminal_configured_ = false;
    int old_flags_ = -1;
    termios old_termios_{};
};

}  // namespace

int main(int argc, char* argv[]) {
    if (argc > 2 || (argc == 2 && std::string(argv[1]) != "--real")) {
        PrintUsage(argv[0]);
        return 1;
    }

    rclcpp::init(argc, argv);
    auto node = std::make_shared<rclcpp::Node>("steer_example");
    const bool use_real_steer = argc == 2;

    SteerPublisher publisher;
    std::unique_ptr<KeyboardSteerInput> keyboard;
    if (use_real_steer) {
        publisher = node->create_publisher<drdds::msg::Steer>("/REAL_STEER", 10);
        RCLCPP_INFO(node->get_logger(), "Publishing /REAL_STEER with yaw=0.4 for %.1f seconds.",
                    kRealSteerDemoDurationSec);
    } else {
        publisher = node->create_publisher<drdds::msg::Steer>("/STEER", 10);

        RCLCPP_WARN(node->get_logger(), "Keyboard steer publishes /STEER and can move the robot.");
        keyboard = std::make_unique<KeyboardSteerInput>();
        if (!keyboard->Start(node->get_logger())) {
            rclcpp::shutdown();
            return 1;
        }
    }

    rclcpp::WallRate rate(kPublishHz);
    const double start_ms = deep_robotics::common::GetTimestampMs();
    while (rclcpp::ok()) {
        if (use_real_steer && deep_robotics::common::GetTimestampMs() - start_ms >=
                                  kRealSteerDemoDurationSec * 1000.0) {
            break;
        }

        const SteerCommand command = use_real_steer ? SteerCommand{0.0f, 0.0f, 0.4f} : keyboard->Update();
        PublishSteer(node, publisher, command);
        rclcpp::spin_some(node);
        rate.sleep();
    }

    keyboard.reset();
    if (rclcpp::ok()) {
        PublishSteer(node, publisher, SteerCommand{0.0f, 0.0f, 0.0f});
        rclcpp::spin_some(node);
        rclcpp::shutdown();
    }
    return 0;
}
