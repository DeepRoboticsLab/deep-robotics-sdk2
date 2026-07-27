/**
 * @file time.hpp
 * @brief Time helpers and loop timer utilities.
 * @author DEEPRobotics
 * @date 2026-06-27
 *
 * @copyright Copyright (c) 2026 DEEPRobotics
 *
 */
#pragma once

#include <cerrno>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <sys/timerfd.h>
#include <unistd.h>

namespace deep_robotics::common {

class LoopTimer {
public:
    LoopTimer() = default;

    explicit LoopTimer(std::chrono::microseconds period) {
        Start(period);
    }

    ~LoopTimer() {
        Close();
    }

    LoopTimer(const LoopTimer&) = delete;
    LoopTimer& operator=(const LoopTimer&) = delete;

    LoopTimer(LoopTimer&& other) noexcept : fd_(other.fd_) {
        other.fd_ = -1;
    }

    LoopTimer& operator=(LoopTimer&& other) noexcept {
        if (this != &other) {
            Close();
            fd_ = other.fd_;
            other.fd_ = -1;
        }
        return *this;
    }

    /**
     * @brief Start the periodic timer.
     * @param period Timer period.
     * @return true if the timer starts successfully; false otherwise.
     */
    bool Start(std::chrono::microseconds period) {
        if (period.count() <= 0) {
            return false;
        }

        if (fd_ == -1) {
            fd_ = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC);
            if (fd_ == -1) {
                return false;
            }
        }

        itimerspec timer_spec{};
        timer_spec.it_value = ToTimespec(period);
        timer_spec.it_interval = timer_spec.it_value;

        return timerfd_settime(fd_, 0, &timer_spec, nullptr) == 0;
    }

    /**
     * @brief Wait for the next timer expiration.
     * @return Number of timer expirations consumed by this wait.
     */
    uint64_t Wait() {
        if (fd_ == -1) {
            return 0;
        }

        uint64_t expirations = 0;
        while (true) {
            const ssize_t bytes = read(fd_, &expirations, sizeof(expirations));
            if (bytes == sizeof(expirations)) {
                return expirations;
            }
            if (bytes == -1 && errno == EINTR) {
                continue;
            }
            return 0;
        }
    }

    /**
     * @brief Check whether the timer file descriptor is valid.
     * @return true if the timer is valid; false otherwise.
     */
    bool IsValid() const {
        return fd_ != -1;
    }

private:
    /**
     * @brief Convert a microsecond duration to a POSIX timespec.
     * @param period Duration to convert.
     * @return Equivalent timespec value.
     */
    static timespec ToTimespec(std::chrono::microseconds period) {
        const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(period);
        const auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(period - seconds);
        return timespec{seconds.count(), nanoseconds.count()};
    }

    /**
     * @brief Close the timer file descriptor.
     */
    void Close() {
        if (fd_ != -1) {
            close(fd_);
            fd_ = -1;
        }
    }

    int fd_ = -1;
};

/**
 * @brief Get elapsed time since startup in milliseconds.
 * @return Elapsed time in milliseconds.
 */
inline double GetTimestampMs() {
    using clock = std::chrono::steady_clock;
    static const auto startup_timestamp = clock::now();
    const auto now_timestamp = clock::now();
    return std::chrono::duration<double, std::milli>(now_timestamp - startup_timestamp).count();
}

}  // namespace deep_robotics::common
