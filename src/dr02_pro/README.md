# DR02 Pro SDK Guide

This document is the English overview of the DR02 Pro product package. For repository-level information and product navigation, see the [DEEPRobotics SDK root README](../../README.md).

This ROS 2 package provides the DR02 Pro motion-control state machine and standalone Topic examples. Programs can run on a development host or robot-side device, and selected programs support validation with the MuJoCo simulation.

## Features

- DR02 Pro RL state-machine control
- Low-level joint state, joint command, and upper-body control examples
- High-level motion state, gait, velocity control, and Developer Mode switching examples
- Monitoring examples for battery state and current-fault snapshots
- Peripheral Topic examples for IMU and gamepad keys
- RealSense depth-camera deployment and usage documentation
- Forward-facing RGB camera access over RTSP
- Audio examples for WAV playback, volume control, and recording

## Directory Layout

- `state_machine/`: main DR02 Pro state-machine program
- `low_level/`: low-level joint and upper-body control Topic examples
- `high_level/`: high-level motion state, gait, velocity control, and Developer Mode switching examples
- `monitoring/`: battery-state and current-fault snapshot monitoring examples
- `peripherals/`: IMU and gamepad key peripheral examples
- `audio/`: WAV playback, volume control, and recording examples
- `docs/`: real-robot deployment, Developer Mode, simulation, state-machine, example, and camera documentation

## Documentation

| Document | Contents |
| --- | --- |
| [Real-Robot Deployment and Control](docs/REAL_ROBOT.md) | Deployment and build on a development host, the AOS host, or the NOS host |
| [Developer Modes](docs/DEVELOPER_MODE.md) | Developer Mode overview, gamepad switching, and SDK example switching procedures |
| [Simulation Environment and Operation](docs/SIMULATION.md) | SDK simulation build, runtime procedure, and supported programs |
| [State Machine](docs/STATE_MACHINE.md) | State transitions, runtime commands, keyboard control, gamepad control, and safety requirements |
| [Topic Examples](docs/EXAMPLES.md) | Topics, real-robot Developer Modes, simulation support, runtime commands, and notes for all examples |
| [RealSense Cameras](docs/CAMERA.md) | Camera driver installation on the NOS host, ROS 2 driver usage, the librealsense C/C++ interface, the pyrealsense2 Python interface, three-camera startup, and Topic verification |
| [Forward RGB Camera](docs/FORWARD_CAMERA.md) | The forward-facing RGB camera on the AOS host: RTSP stream address, required transport settings, how to tell a decoded frame from an undecoded one, and how to keep camera viewing off the robot's CPU |
| [Joint Control Interface](docs/JOINT_CONTROL.md) | Joint message fields, array order, controllable ranges, units, and zero-position definition |

## Quick Start

For the first run, select the environment setup document according to the control target, then follow the links in the next-step column for the required mode and examples.

| Control target | Environment setup | Next step |
| --- | --- | --- |
| Real robot | [Real-Robot Deployment and Control](docs/REAL_ROBOT.md) | Read [Developer Mode](docs/DEVELOPER_MODE.md), then choose [State Machine](docs/STATE_MACHINE.md) or [Topic Examples](docs/EXAMPLES.md) |
| Simulation | [Simulation Environment and Operation](docs/SIMULATION.md) | Choose [State Machine](docs/STATE_MACHINE.md) or [Topic Examples](docs/EXAMPLES.md) according to simulation support |

> [!IMPORTANT]
>
> The SDK runtime location and control target are independent concepts. If ROS/DDS network communication is available, an SDK process running on a development host, the AOS host (`10.21.33.103`), or the NOS host (`10.21.33.106`) may directly control the real robot.

## Important Safety Information

> [!WARNING]
>
> - Real-robot programs may directly drive robot motion. Confirm that the robot state and surrounding motion area are safe before running them.
> - When running a motion-control program on the real robot for the first time, test it with a reliable safety suspension in place.
> - Before controlling the real robot, ensure that it has entered the Developer Mode required by the target program.
> - Only one `/JOINTS_CMD` publisher may run at a time.
> - If an abnormal condition occurs, use the red stop button on the gamepad to stop control immediately.

## Build Options

| Option | Default | Description |
| --- | --- | --- |
| `BUILD_DR02_PRO_STATE_MACHINE` | `ON` | Build the state machine |
| `BUILD_DR02_PRO_LOW_LEVEL` | `ON` | Build low-level joint and upper-body control examples |
| `BUILD_DR02_PRO_HIGH_LEVEL` | `ON` | Build high-level motion, gait, velocity, and Developer Mode switching examples |
| `BUILD_DR02_PRO_MONITORING` | `ON` | Build monitoring examples |
| `BUILD_DR02_PRO_PERIPHERALS` | `ON` | Build peripheral examples |
| `BUILD_DR02_PRO_AUDIO` | `ON` | Build audio examples |
| `BUILD_SIM` | `OFF` | Enable simulation support; do not enable for real-robot control |
