/**
 * @file main.cpp
 * @brief State-machine executable entry point.
 * @author DEEPRobotics
 * @date 2026-06-27
 *
 * @copyright Copyright (c) 2026 DEEPRobotics
 *
 */
#include <iostream>
#include <string>

#include "humanoid_state_machine.hpp"

using namespace deep_robotics::dr02_std;

int main() {
  std::cout << "State Machine Start Running" << std::endl;
  rclcpp::init(0, 0);

  std::shared_ptr<HumanoidStateMachine> fsm =
      std::make_shared<HumanoidStateMachine>(RobotName::DR2STD);
  fsm->Start();
  fsm->Run();
  fsm->Stop();

  rclcpp::shutdown();
  return 0;
}
