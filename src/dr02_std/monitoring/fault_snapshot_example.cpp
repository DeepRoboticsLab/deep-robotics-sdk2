/**
 * @file fault_snapshot_example.cpp
 * @brief Current-fault snapshot subscriber example.
 * @author DEEPRobotics
 * @date 2026-08-03
 *
 * @copyright Copyright (c) 2026 DEEPRobotics
 *
 */
#include <algorithm>
#include <cstdint>
#include <ctime>
#include <functional>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "drdds/msg/fault_event.hpp"
#include "drdds/msg/fault_event_array.hpp"
#include "rclcpp/rclcpp.hpp"

namespace {

const char* SeverityName(uint8_t severity) {
    switch (severity) {
        case drdds::msg::FaultEvent::SEVERITY_DEBUG:
            return "DEBUG";
        case drdds::msg::FaultEvent::SEVERITY_INFO:
            return "INFO";
        case drdds::msg::FaultEvent::SEVERITY_NOTICE:
            return "NOTICE";
        case drdds::msg::FaultEvent::SEVERITY_WARN:
            return "WARN";
        case drdds::msg::FaultEvent::SEVERITY_ERROR:
            return "ERROR";
        case drdds::msg::FaultEvent::SEVERITY_CRITICAL:
            return "CRITICAL";
        case drdds::msg::FaultEvent::SEVERITY_ALERT:
            return "ALERT";
        case drdds::msg::FaultEvent::SEVERITY_EMERG:
            return "EMERG";
        default:
            return "UNKNOWN";
    }
}

std::string FormatFaultTime(const builtin_interfaces::msg::Time& timestamp) {
    constexpr uint32_t kNanosecondsPerSecond = 1000000000U;
    constexpr uint32_t kNanosecondsPerMillisecond = 1000000U;
    if (timestamp.sec <= 0 || timestamp.nanosec >= kNanosecondsPerSecond) {
        return {};
    }

    const std::time_t seconds = static_cast<std::time_t>(timestamp.sec);
    std::tm local_time{};
    if (localtime_r(&seconds, &local_time) == nullptr) {
        return {};
    }

    std::ostringstream stream;
    stream << std::put_time(&local_time, "%Y-%m-%d %H:%M:%S") << '.' << std::setfill('0') << std::setw(3)
           << timestamp.nanosec / kNanosecondsPerMillisecond;
    return stream.str();
}

std::string HighestSeverityText(const std::vector<uint8_t>& severities) {
    if (severities.empty()) {
        return "UNKNOWN";
    }

    const uint8_t severity = *std::max_element(severities.begin(), severities.end());
    std::ostringstream stream;
    stream << SeverityName(severity) << '(' << static_cast<unsigned>(severity) << ')';
    return stream.str();
}

}  // namespace

class FaultSnapshotSubscriber : public rclcpp::Node {
public:
    FaultSnapshotSubscriber() : Node("fault_snapshot_example") {
        const auto qos = rclcpp::QoS(rclcpp::KeepLast(1)).reliable();
        fault_snapshot_sub_ = create_subscription<drdds::msg::FaultEventArray>(
            "/fault_aggregator", qos,
            std::bind(&FaultSnapshotSubscriber::HandleFaultSnapshot, this, std::placeholders::_1));
        RCLCPP_INFO(get_logger(), "Subscribed /fault_aggregator. Press Ctrl+C to exit.");
    }

private:
    /**
     * @brief Handle current-fault snapshots from /fault_aggregator.
     * @param msg Received fault snapshot.
     */
    void HandleFaultSnapshot(const drdds::msg::FaultEventArray::SharedPtr msg) {
        // Compare only active faults because the snapshot timestamp changes on every publication.
        if (has_snapshot_ && msg->active_faults == last_active_faults_) {
            return;
        }
        last_active_faults_ = msg->active_faults;
        has_snapshot_ = true;

        if (msg->active_faults.empty()) {
            RCLCPP_INFO(get_logger(), "No active faults.");
            return;
        }

        RCLCPP_INFO(get_logger(), "Active faults: %zu", msg->active_faults.size());

        for (const auto& fault : msg->active_faults) {
            const std::string fault_time = FormatFaultTime(fault.timestamp);
            const std::string severity = HighestSeverityText(fault.severities);
            if (fault_time.empty()) {
                RCLCPP_INFO(get_logger(), "[%s] 0x%04X %s", severity.c_str(),
                            static_cast<unsigned>(fault.code), fault.name.c_str());
            } else {
                RCLCPP_INFO(get_logger(), "[%s] [%s] 0x%04X %s", fault_time.c_str(), severity.c_str(),
                            static_cast<unsigned>(fault.code), fault.name.c_str());
            }
        }
    }

    rclcpp::Subscription<drdds::msg::FaultEventArray>::SharedPtr fault_snapshot_sub_;
    std::vector<drdds::msg::FaultEvent> last_active_faults_;
    bool has_snapshot_ = false;
};

int main(int argc, char* argv[]) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<FaultSnapshotSubscriber>());
    rclcpp::shutdown();
    return 0;
}
