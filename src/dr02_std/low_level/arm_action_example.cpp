/**
 * @file arm_action_example.cpp
 * @brief Preset upper-body action command example.
 * @author DEEPRobotics
 * @date 2026-06-27
 *
 * @copyright Copyright (c) 2026 DEEPRobotics
 *
 */
#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "drdds/msg/joints.hpp"
#include "drdds/msg/joints_cmd.hpp"
#include "rclcpp/rclcpp.hpp"
#include "utils.hpp"

namespace {

constexpr int kDofNum = 21;
constexpr int kUpperBodyDofNum = 9;
constexpr auto kPublishPeriod = std::chrono::milliseconds(5);

using JointArray = std::array<float, kDofNum>;

constexpr JointArray kBaseKp{
    600.0f,                                  // waist
    200.0f, 200.0f, 200.0f, 200.0f,          // left arm
    200.0f, 200.0f, 200.0f, 200.0f,          // right arm
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,     // left leg
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,     // right leg
};

constexpr JointArray kBaseKd{
    6.0f,                                  // waist
    5.0f, 5.0f, 5.0f, 5.0f,               // left arm
    5.0f, 5.0f, 5.0f, 5.0f,               // right arm
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,   // left leg
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,   // right leg
};

struct ActionInfo {
    int id;
    const char* name;
};

struct ActionPlan {
    int id = -1;
    const char* name = "";
    float kp_scale = 1.0f;
    float elbow_kp_scale = 1.0f;
    std::vector<float> swing_time;
    std::vector<JointArray> goal_joint_pos;
};

constexpr std::array<ActionInfo, 5> kActionInfos{{
    {0, "greeting"},
    {1, "kiss"},
    {2, "handshake"},
    {3, "salute"},
    {4, "salute2"},
}};

/**
 * @brief Parse a preset action ID.
 * @param value Input command-line string.
 * @param action_id Parsed action ID.
 * @return true if parsing succeeds; false otherwise.
 */
bool ParseActionId(const char* value, int& action_id) {
    errno = 0;
    char* end = nullptr;
    const long parsed = std::strtol(value, &end, 10);
    if (errno != 0 || end == value || *end != '\0') {
        return false;
    }

    for (const auto& action : kActionInfos) {
        if (parsed == action.id) {
            action_id = static_cast<int>(parsed);
            return true;
        }
    }
    return false;
}

/**
 * @brief Initialize action plan stage containers.
 * @param plan Action plan to initialize.
 * @param n Number of action stages.
 */
void InitPlan(ActionPlan& plan, int n) {
    plan.swing_time.assign(n, 0.0f);
    plan.goal_joint_pos.assign(n, JointArray{});
}

/**
 * @brief Fill standing arm baseline targets for all action stages.
 * @param plan Action plan to update.
 */
void SetStandingArmBaseline(ActionPlan& plan) {
    for (std::size_t s = 0; s < plan.goal_joint_pos.size(); ++s) {
        plan.goal_joint_pos[s][2] = 0.13f;    // left shoulder_x
        plan.goal_joint_pos[s][4] = 1.2f;     // left elbow_y
        plan.goal_joint_pos[s][6] = -0.13f;  // right shoulder_x
        plan.goal_joint_pos[s][8] = 1.2f;    // right elbow_y
    }
}

/**
 * @brief Build the greeting action plan.
 * @return Greeting action plan.
 */
ActionPlan GreetingPlan() {
    ActionPlan plan;
    plan.id = 0;
    plan.name = "greeting";
    plan.kp_scale = 1.0f;
    plan.elbow_kp_scale = 1.0f;
    InitPlan(plan, 6);

    plan.swing_time = {2.0f, 0.8f, 0.8f, 0.8f, 0.8f, 2.0f};
    SetStandingArmBaseline(plan);

    // Stage 0
    plan.goal_joint_pos[0][5] = -0.731f;
    plan.goal_joint_pos[0][6] = -0.374f;
    plan.goal_joint_pos[0][7] = -0.351f;
    plan.goal_joint_pos[0][8] = -0.340f;
    plan.goal_joint_pos[0][1] = -0.731f;
    plan.goal_joint_pos[0][2] = 0.374f;
    plan.goal_joint_pos[0][3] = 0.351f;
    plan.goal_joint_pos[0][4] = -0.340f;
    // Stage 1
    plan.goal_joint_pos[1][5] = -2.339f;
    plan.goal_joint_pos[1][6] = -0.239f;
    plan.goal_joint_pos[1][7] = 0.182f;
    plan.goal_joint_pos[1][8] = 1.028f;
    plan.goal_joint_pos[1][1] = -2.339f;
    plan.goal_joint_pos[1][2] = 0.239f;
    plan.goal_joint_pos[1][3] = -0.182f;
    plan.goal_joint_pos[1][4] = 1.028f;
    // Stage 2
    plan.goal_joint_pos[2][5] = -0.731f;
    plan.goal_joint_pos[2][6] = -0.374f;
    plan.goal_joint_pos[2][7] = -0.351f;
    plan.goal_joint_pos[2][8] = -0.340f;
    plan.goal_joint_pos[2][1] = -0.731f;
    plan.goal_joint_pos[2][2] = 0.374f;
    plan.goal_joint_pos[2][3] = 0.351f;
    plan.goal_joint_pos[2][4] = -0.340f;
    // Stage 3
    plan.goal_joint_pos[3][5] = -2.339f;
    plan.goal_joint_pos[3][6] = -0.239f;
    plan.goal_joint_pos[3][7] = 0.182f;
    plan.goal_joint_pos[3][8] = 1.028f;
    plan.goal_joint_pos[3][1] = -2.339f;
    plan.goal_joint_pos[3][2] = 0.239f;
    plan.goal_joint_pos[3][3] = -0.182f;
    plan.goal_joint_pos[3][4] = 1.028f;
    // Stage 4
    plan.goal_joint_pos[4][5] = -0.731f;
    plan.goal_joint_pos[4][6] = -0.374f;
    plan.goal_joint_pos[4][7] = -0.351f;
    plan.goal_joint_pos[4][8] = -0.340f;
    plan.goal_joint_pos[4][1] = -0.731f;
    plan.goal_joint_pos[4][2] = 0.374f;
    plan.goal_joint_pos[4][3] = 0.351f;
    plan.goal_joint_pos[4][4] = -0.340f;
    // Stage 5
    return plan;
}

/**
 * @brief Build the kiss action plan.
 * @return Kiss action plan.
 */
ActionPlan KissPlan() {
    ActionPlan plan;
    plan.id = 1;
    plan.name = "kiss";
    plan.kp_scale = 1.0f;
    plan.elbow_kp_scale = 1.0f;
    InitPlan(plan, 4);

    plan.swing_time = {1.0f, 2.0f, 2.0f, 2.0f};
    SetStandingArmBaseline(plan);

    // Stage 0
    plan.goal_joint_pos[0][2] = 0.15f;
    plan.goal_joint_pos[0][6] = -0.15f;
    plan.goal_joint_pos[0][4] = 1.1f;
    plan.goal_joint_pos[0][8] = 1.1f;
    // Stage 1
    plan.goal_joint_pos[1][5] = -0.935f;
    plan.goal_joint_pos[1][6] = -0.154f;
    plan.goal_joint_pos[1][7] = 0.908f;
    plan.goal_joint_pos[1][8] = -0.7f;
    // Stage 2
    plan.goal_joint_pos[2][5] = -0.803f;
    plan.goal_joint_pos[2][6] = -0.541f;
    plan.goal_joint_pos[2][7] = -0.479f;
    plan.goal_joint_pos[2][8] = 0.202f;
    // Stage 3
    plan.goal_joint_pos[3][2] = 0.15f;
    plan.goal_joint_pos[3][6] = -0.15f;
    plan.goal_joint_pos[3][4] = 1.1f;
    plan.goal_joint_pos[3][8] = 1.1f;
    return plan;
}

/**
 * @brief Build the handshake action plan.
 * @return Handshake action plan.
 */
ActionPlan HandshakePlan() {
    ActionPlan plan;
    plan.id = 2;
    plan.name = "handshake";
    plan.kp_scale = 0.25f;
    plan.elbow_kp_scale = 0.1f;
    InitPlan(plan, 3);

    plan.swing_time = {2.0f, 6.0f, 2.0f};
    SetStandingArmBaseline(plan);

    // Stage 0-1
    for (int s = 0; s <= 1; ++s) {
        plan.goal_joint_pos[s][5] = -0.25f;
        plan.goal_joint_pos[s][6] = 0.08f;
        plan.goal_joint_pos[s][7] = 0.47f;
        plan.goal_joint_pos[s][8] = 0.37f;
    }
    // Stage 2
    return plan;
}

/**
 * @brief Build the salute action plan.
 * @return Salute action plan.
 */
ActionPlan SalutePlan() {
    ActionPlan plan;
    plan.id = 3;
    plan.name = "salute";

    plan.kp_scale = 1.0f;
    plan.elbow_kp_scale = 1.0f;
    InitPlan(plan, 3);

    plan.swing_time = {4.0f, 4.0f, 4.0f};
    SetStandingArmBaseline(plan);

    // Stage 0-1
    for (int s = 0; s <= 1; ++s) {
        plan.goal_joint_pos[s][5] = -0.141f;  // shoulder_y
        plan.goal_joint_pos[s][6] = -1.775f;  // shoulder_x
        plan.goal_joint_pos[s][7] = -1.441f;  // shoulder_z
        plan.goal_joint_pos[s][8] = -0.675f;  // elbow_y
    }
    // Stage 2
    return plan;
}

/**
 * @brief Build the second salute action plan.
 * @return Second salute action plan.
 */
ActionPlan Salute2Plan() {
    ActionPlan plan;
    plan.id = 4;
    plan.name = "salute2";

    plan.kp_scale = 1.0f;
    plan.elbow_kp_scale = 1.0f;
    InitPlan(plan, 3);

    plan.swing_time = {4.0f, 4.0f, 4.0f};
    SetStandingArmBaseline(plan);

    // Stage 0-1
    for (int s = 0; s <= 1; ++s) {
        plan.goal_joint_pos[s][5] = -2.180f;  // shoulder_y
        plan.goal_joint_pos[s][6] = 0.088f;   // shoulder_x
        plan.goal_joint_pos[s][7] = 0.304f;   // shoulder_z
        plan.goal_joint_pos[s][8] = 0.357f;   // elbow_y
    }
    // Stage 2
    return plan;
}

/**
 * @brief Build an action plan for the selected action ID.
 * @param action_id Preset action ID.
 * @return Action plan for the ID.
 */
ActionPlan MakeActionPlan(int action_id) {
    switch (action_id) {
        case 0:
            return GreetingPlan();
        case 1:
            return KissPlan();
        case 2:
            return HandshakePlan();
        case 3:
            return SalutePlan();
        case 4:
            return Salute2Plan();
        default:
            return HandshakePlan();
    }
}

/**
 * @brief Build upper-body proportional gains for an action plan.
 * @param plan Action plan containing gain scaling values.
 * @return Joint gain array.
 */
JointArray MakeActionKp(const ActionPlan& plan) {
    JointArray kp = kBaseKp;
    for (int i = 0; i < kUpperBodyDofNum; ++i) {
        kp[i] = kBaseKp[i] * plan.kp_scale;
    }

    if (plan.elbow_kp_scale < 1.0f) {
        kp[4] = kBaseKp[4] * plan.elbow_kp_scale;    // left elbow_y
        kp[8] = kBaseKp[8] * plan.elbow_kp_scale;  // right elbow_y
    }

    return kp;
}

}  // namespace

class ArmActionExample : public rclcpp::Node {
public:
    explicit ArmActionExample(ActionPlan plan)
        : Node("arm_action_example"),
          plan_(std::move(plan)),
          action_kp_(MakeActionKp(plan_)),
          action_kd_(kBaseKd) {
        joint_cmd_pub_ = create_publisher<drdds::msg::JointsCmd>("/JOINTS_CMD", 10);
        joint_data_sub_ = create_subscription<drdds::msg::Joints>(
            "/JOINTS_DATA", 10, std::bind(&ArmActionExample::HandleJointData, this, std::placeholders::_1));
        publish_timer_ = create_wall_timer(kPublishPeriod, std::bind(&ArmActionExample::PublishJointCommand, this));

        RCLCPP_WARN(get_logger(), "Publishing /JOINTS_CMD directly. Do not run the state machine at the same time.");
        RCLCPP_INFO(get_logger(), "Selected action: %d %s", plan_.id, plan_.name);
        RCLCPP_INFO(get_logger(), "Waiting for /JOINTS_DATA before starting preset action.");
    }

private:
    /**
     * @brief Handle joint-state messages from /JOINTS_DATA.
     * @param msg Received joint-state message.
     */
    void HandleJointData(const drdds::msg::Joints::SharedPtr msg) {
        if (initial_state_ready_) {
            return;
        }

        if (msg->data.size() != kDofNum) {
            RCLCPP_WARN_THROTTLE(
                get_logger(), *get_clock(), 1000, "Expected %d joints, received %zu", kDofNum, msg->data.size());
            return;
        }

        for (int i = 0; i < kDofNum; ++i) {
            initial_positions_[i] = msg->data[i].position;
            initial_velocities_[i] = msg->data[i].velocity;
        }

        start_time_ = now();
        initial_state_ready_ = true;
        RCLCPP_INFO(get_logger(), "Initial joint state received. Starting action %d %s.", plan_.id, plan_.name);
    }

    /**
     * @brief Publish the current preset action command to /JOINTS_CMD.
     */
    void PublishJointCommand() {
        if (!initial_state_ready_) {
            RCLCPP_INFO_THROTTLE(get_logger(), *get_clock(), 1000, "Waiting for /JOINTS_DATA...");
            return;
        }

        const rclcpp::Time stamp = now();
        const float run_time = static_cast<float>(std::max(0.0, (stamp - start_time_).seconds()));
        const float swing_time = plan_.swing_time[action_state_];
        JointArray planning_joint_pos{};
        JointArray planning_joint_vel{};

        if (run_time <= swing_time) {
            for (int i = 0; i < kUpperBodyDofNum; ++i) {
                if (action_state_ == 0) {
                    planning_joint_pos[i] = deep_robotics::common::GetCubicSplinePos(
                        initial_positions_[i], initial_velocities_[i], plan_.goal_joint_pos[action_state_][i], 0.0f,
                        run_time, swing_time);
                    planning_joint_vel[i] = deep_robotics::common::GetCubicSplineVel(
                        initial_positions_[i], initial_velocities_[i], plan_.goal_joint_pos[action_state_][i], 0.0f,
                        run_time, swing_time);
                } else {
                    planning_joint_pos[i] = deep_robotics::common::GetCubicSplinePos(
                        plan_.goal_joint_pos[action_state_ - 1][i], 0.0f, plan_.goal_joint_pos[action_state_][i],
                        0.0f,
                        run_time, swing_time);
                    planning_joint_vel[i] = deep_robotics::common::GetCubicSplineVel(
                        plan_.goal_joint_pos[action_state_ - 1][i], 0.0f, plan_.goal_joint_pos[action_state_][i],
                        0.0f, run_time, swing_time);
                }
            }
        } else if (action_state_ < static_cast<int>(plan_.goal_joint_pos.size()) - 1) {
            for (int i = 0; i < kUpperBodyDofNum; ++i) {
                planning_joint_pos[i] = plan_.goal_joint_pos[action_state_][i];
            }
            action_state_++;
            start_time_ = stamp;
            RCLCPP_INFO(get_logger(), "go to next arm action state: %d", action_state_);
        } else {
            for (int i = 0; i < kUpperBodyDofNum; ++i) {
                planning_joint_pos[i] = plan_.goal_joint_pos.back()[i];
            }
            // keep
            if (!completed_) {
                completed_ = true;
                RCLCPP_INFO(get_logger(), "Preset action %d %s completed. Holding final command until Ctrl+C.",
                            plan_.id, plan_.name);
            }
        }

        drdds::msg::JointsCmd msg;
        msg.header.frame_id = frame_id_++;
        msg.header.stamp = stamp;
        msg.data.resize(kDofNum);

        for (int i = 0; i < kDofNum; ++i) {
            msg.data[i].control_word = 4;
            msg.data[i].position = planning_joint_pos[i];
            msg.data[i].velocity = planning_joint_vel[i];
            msg.data[i].torque = 0.0f;
            msg.data[i].kp = action_kp_[i];
            msg.data[i].kd = action_kd_[i];
        }

        joint_cmd_pub_->publish(msg);
    }

    ActionPlan plan_;
    JointArray action_kp_;
    JointArray action_kd_;
    JointArray initial_positions_{};
    JointArray initial_velocities_{};
    bool initial_state_ready_ = false;
    bool completed_ = false;
    int action_state_ = 0;
    uint64_t frame_id_ = 0;
    rclcpp::Time start_time_;
    rclcpp::Publisher<drdds::msg::JointsCmd>::SharedPtr joint_cmd_pub_;
    rclcpp::Subscription<drdds::msg::Joints>::SharedPtr joint_data_sub_;
    rclcpp::TimerBase::SharedPtr publish_timer_;
};

int main(int argc, char* argv[]) {
    if (argc == 1) {
        std::cout << "action_id action_name\n";
        for (const auto& action : kActionInfos) {
            std::cout << action.id << " " << action.name << "\n";
        }
        std::cout << "\nUsage:\n"
                  << "  " << argv[0] << " <action_id> --confirm\n\n"
                  << "WARNING: this example publishes /JOINTS_CMD directly.\n"
                  << "Do not run state_machine or another /JOINTS_CMD publisher at the same time.\n";
        return 0;
    }

    if (argc != 3 || std::string(argv[2]) != "--confirm") {
        std::cerr << "Usage:\n"
                  << "  " << argv[0] << "\n"
                  << "  " << argv[0] << " <action_id> --confirm\n\n"
                  << "WARNING: this example publishes /JOINTS_CMD directly.\n";
        return 1;
    }

    int action_id = -1;
    if (!ParseActionId(argv[1], action_id)) {
        std::cerr << "invalid action_id. Run without arguments to show available actions.\n";
        return 1;
    }

    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<ArmActionExample>(MakeActionPlan(action_id)));
    rclcpp::shutdown();
    return 0;
}
