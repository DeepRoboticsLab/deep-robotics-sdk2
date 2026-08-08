/**
 * @file audio_record_example.cpp
 * @brief Interactive audio recording control example.
 * @author DEEPRobotics
 * @date 2026-06-27
 *
 * @copyright Copyright (c) 2026 DEEPRobotics
 *
 */
#include <atomic>
#include <chrono>
#include <csignal>
#include <functional>
#include <iostream>
#include <memory>
#include <thread>

#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

#include "drdds/msg/std_msg_int32.hpp"
#include "drdds/msg/std_status.hpp"
#include "rclcpp/rclcpp.hpp"

namespace {

std::atomic<bool> g_running{true};

/**
 * @brief Handle process termination signals.
 */
void SignalHandler(int) {
    g_running.store(false);
}

/**
 * @brief Convert a recording state code to display text.
 * @param state Recording state code.
 * @return Recording state name.
 */
const char* RecordStateName(int state) {
    switch (state) {
        case 0:
            return "IDLE";
        case 1:
            return "RECORDING";
        case 2:
            return "DONE";
        case 3:
            return "FAULT";
        default:
            return "UNKNOWN";
    }
}

class TerminalInput {
public:
    TerminalInput() {
        if (!isatty(STDIN_FILENO)) return;
        if (tcgetattr(STDIN_FILENO, &old_termios_) != 0) return;

        termios raw = old_termios_;
        raw.c_lflag &= static_cast<tcflag_t>(~(ICANON | ECHO));
        raw.c_cc[VMIN] = 0;
        raw.c_cc[VTIME] = 0;
        if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) == 0) {
            raw_enabled_ = true;
        }
    }

    ~TerminalInput() {
        if (raw_enabled_) {
            tcsetattr(STDIN_FILENO, TCSANOW, &old_termios_);
        }
    }

    /**
     * @brief Read one key from terminal input.
     * @return Key code, -1 when no key is available, or q on EOF.
     */
    int ReadKey() {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);

        timeval timeout{};
        timeout.tv_sec = 0;
        timeout.tv_usec = 100 * 1000;

        const int ready = select(STDIN_FILENO + 1, &read_fds, nullptr, nullptr, &timeout);
        if (ready < 0) {
            g_running.store(false);
            return 'q';
        }
        if (ready == 0 || !FD_ISSET(STDIN_FILENO, &read_fds)) {
            return -1;
        }

        char key = 0;
        const ssize_t n = read(STDIN_FILENO, &key, 1);
        if (n <= 0) {
            g_running.store(false);
            return 'q';
        }

        if (raw_enabled_) {
            std::cout << key << "\n";
        }

        return static_cast<unsigned char>(key);
    }

private:
    bool raw_enabled_ = false;
    termios old_termios_{};
};

class AudioRecordExample : public rclcpp::Node {
public:
    AudioRecordExample()
        : Node("audio_record_example") {
        publisher_ = create_publisher<drdds::msg::StdMsgInt32>("/AUDIO/RECORD_CMD", 10);
        status_subscriber_ = create_subscription<drdds::msg::StdStatus>(
            "/AUDIO/RECORD_STATUS",
            10,
            std::bind(&AudioRecordExample::HandleRecordStatus, this, std::placeholders::_1));
    }

    /**
     * @brief Wait briefly for a record command subscriber.
     */
    void WaitForSubscriber() {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
        while (rclcpp::ok() && publisher_->get_subscription_count() == 0 &&
               std::chrono::steady_clock::now() < deadline) {
            rclcpp::spin_some(shared_from_this());
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        if (publisher_->get_subscription_count() == 0) {
            RCLCPP_WARN(get_logger(), "No subscriber on /AUDIO/RECORD_CMD; commands may be missed");
        }
    }

    /**
     * @brief Publish a start-recording command.
     */
    void StartRecording() {
        drdds::msg::StdMsgInt32 msg;
        msg.value = 1;
        publisher_->publish(msg);
        recording_requested_.store(true);
        RCLCPP_INFO(get_logger(), "Published /AUDIO/RECORD_CMD value=1 (start)");
    }

    /**
     * @brief Publish a stop-recording command.
     */
    void StopRecording() {
        drdds::msg::StdMsgInt32 msg;
        msg.value = 0;
        publisher_->publish(msg);
        recording_requested_.store(false);
        RCLCPP_INFO(get_logger(), "Published /AUDIO/RECORD_CMD value=0 (stop)");
    }

    /**
     * @brief Stop recording when a recording request is active.
     */
    void StopIfNeeded() {
        if (!recording_requested_.load()) return;
        StopRecording();
        rclcpp::spin_some(shared_from_this());
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }

private:
    /**
     * @brief Handle recording status messages from /AUDIO/RECORD_STATUS.
     * @param msg Received recording status message.
     */
    void HandleRecordStatus(const drdds::msg::StdStatus::SharedPtr msg) {
        if (!msg) return;
        const int state = msg->state;
        const int error_code = msg->error_code;

        if (state == 1) {
            recording_requested_.store(true);
        } else if (state == 0 || state == 2 || state == 3) {
            recording_requested_.store(false);
        }

        if (state == 2) {
            RCLCPP_INFO(get_logger(),
                        "/AUDIO/RECORD_STATUS state=%s(%d) file_size=%d KB. Recordings are stored under /var/opt/robot/data/audio/ on the robot.",
                        RecordStateName(state), state, error_code);
        } else if (state == 3) {
            RCLCPP_ERROR(get_logger(), "/AUDIO/RECORD_STATUS state=%s(%d) error_code=%d",
                         RecordStateName(state), state, error_code);
        } else {
            RCLCPP_INFO(get_logger(), "/AUDIO/RECORD_STATUS state=%s(%d) extra=%d",
                        RecordStateName(state), state, error_code);
        }
    }

    rclcpp::Publisher<drdds::msg::StdMsgInt32>::SharedPtr publisher_;
    rclcpp::Subscription<drdds::msg::StdStatus>::SharedPtr status_subscriber_;
    std::atomic<bool> recording_requested_{false};
};

}  // namespace

int main(int argc, char* argv[]) {
    if (argc != 1) {
        std::cout << "Usage:\n"
                  << "  " << argv[0] << "\n";
        return 1;
    }

    rclcpp::init(argc, argv);
    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);

    auto node = std::make_shared<AudioRecordExample>();
    node->WaitForSubscriber();

    TerminalInput terminal_input;
    std::cout << "Commands:\n"
              << "  1: start recording\n"
              << "  0: stop recording\n"
              << "  q: stop and quit\n\n"
              << "Recording files are stored on the robot audio node under:\n"
              << "  /var/opt/robot/data/audio/\n"
              << "Log in to the robot-side device to list or copy generated audio_record_*.wav files.\n\n";

    while (rclcpp::ok() && g_running.load()) {
        rclcpp::spin_some(node);

        const int key = terminal_input.ReadKey();
        if (key < 0) continue;
        if (key == '\n' || key == '\r') continue;

        if (key == '1') {
            node->StartRecording();
        } else if (key == '0') {
            node->StopRecording();
        } else if (key == 'q' || key == 'Q') {
            g_running.store(false);
            break;
        } else {
            std::cout << "Unknown command. Press 1, 0, or q.\n";
        }
    }

    node->StopIfNeeded();
    rclcpp::shutdown();
    return 0;
}
