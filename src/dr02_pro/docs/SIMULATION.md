# DR02 Pro Simulation Environment and Operation

[Back to the DR02 Pro SDK Guide](../README.md)

This document describes how to build and run the SDK side when using the DR02 Pro SDK with a simulation environment. For simulation environment setup and startup instructions, see the [deep-robotics-simulation](https://github.com/DeepRoboticsLab/deep-robotics-simulation) repository.

## Environment Preparation

Prepare the following on the development host:

- A ROS 2 environment with `ament_cmake`.
- The [deep-robotics-msg](https://github.com/DeepRoboticsLab/deep-robotics-msg) message interface package installed or loaded. Its ROS 2 package name is `drdds`.

The message interface package supports deb installation and source builds. See the `deep-robotics-msg` documentation for detailed instructions. With a deb installation, loading the ROS 2 environment is sufficient. With a source build, the message interface workspace `install/setup.bash` must also be loaded. The commands below show only the ROS 2 environment setup.

```bash
source /opt/ros/<ros-distro>/setup.bash
```

## Build the SDK

```bash
colcon build --packages-up-to dr02_pro --cmake-args -DBUILD_PLATFORM=x86 -DBUILD_SIM=ON
```

> [!WARNING]
>
> `BUILD_SIM=ON` is only for the simulation environment and must not be used for real-robot control.

## Supported Programs

The following programs currently support simulation through `/JOINTS_DATA` and `/JOINTS_CMD`:

- `state_machine`
- `joints_example`
- `arm_joint_example`
- `arm_action_example`

High-level, peripheral, and audio examples do not currently support simulation validation.

## Run the SDK

The SDK and simulation are independent processes and must run in separate terminals. Start the simulation according to the repository documentation above. Ensure that both terminals load the same ROS 2 and message interface environments and use the same `ROS_DOMAIN_ID`.

The following SDK terminal command runs the state machine as an example:

```bash
export ROS_DOMAIN_ID=<domain-id>
source /opt/ros/<ros-distro>/setup.bash
source install/setup.bash
ros2 run dr02_pro state_machine
```

See [Topic Examples](EXAMPLES.md) for low-level example commands.
