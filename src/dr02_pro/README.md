# DR02 Pro SDK Guide

This document is the English overview of the DR02 Pro product package. For repository-level information and product navigation, see the [DEEPRobotics SDK root README](../../README.md).

This ROS 2 package provides the DR02 Pro motion-control state machine and standalone Topic examples. Programs can run on a development host or robot-side device, and selected programs support validation with the MuJoCo simulation.

## Features

- DR02 Pro RL state-machine control
- Low-level joint state, joint command, and upper-body control examples
- High-level motion state, gait, and velocity control examples
- Peripheral Topic examples for battery, IMU, and gamepad keys
- RealSense depth-camera deployment and usage documentation
- Audio examples for WAV playback, volume control, and recording

## Directory Layout

- `state_machine/`: main DR02 Pro state-machine program
- `low_level/`: low-level joint and upper-body control Topic examples
- `high_level/`: high-level motion state, gait, and velocity control examples
- `peripherals/`: battery, IMU, and gamepad key examples
- `audio/`: WAV playback, volume control, and recording examples
- `docs/`: real-robot control, simulation, state-machine, example, and camera documentation

## Build Options

| Option | Default | Description |
| --- | --- | --- |
| `BUILD_DR02_PRO_STATE_MACHINE` | `ON` | Build the state machine |
| `BUILD_DR02_PRO_LOW_LEVEL` | `ON` | Build low-level joint and upper-body control examples |
| `BUILD_DR02_PRO_HIGH_LEVEL` | `ON` | Build high-level motion, gait, and velocity examples |
| `BUILD_DR02_PRO_PERIPHERALS` | `ON` | Build peripheral examples |
| `BUILD_DR02_PRO_AUDIO` | `ON` | Build audio examples |
| `BUILD_SIM` | `OFF` | Enable simulation support; do not enable for real-robot control |

## Workflow

1. Select [Real-Robot Deployment and Control](docs/REAL_ROBOT.md) or [Simulation Environment and Operation](docs/SIMULATION.md) according to the control target.
2. Prepare the environment and build the SDK according to the selected document.
3. Run the [State Machine](docs/STATE_MACHINE.md) or the required [Topic Example](docs/EXAMPLES.md).

> [!IMPORTANT]
>
> The SDK runtime location and control target are independent concepts. If ROS/DDS network communication is available, an SDK process running on a development host, the AOS host (`10.21.33.103`), or the NOS host (`10.21.33.106`) may directly control the real robot.

## Documentation

| Document | Contents |
| --- | --- |
| [Real-Robot Deployment and Control](docs/REAL_ROBOT.md) | Deployment on a development host, the AOS host, or the NOS host, Developer Modes, and real-robot safety requirements |
| [Simulation Environment and Operation](docs/SIMULATION.md) | SDK simulation build, runtime procedure, and supported programs |
| [State Machine](docs/STATE_MACHINE.md) | State transitions, runtime commands, keyboard control, gamepad control, and safety requirements |
| [Topic Examples](docs/EXAMPLES.md) | Topics, real-robot Developer Modes, simulation support, runtime commands, and notes for all examples |
| [RealSense Cameras](docs/CAMERA.md) | Camera driver installation on the NOS host, ROS 2 driver usage, the librealsense C/C++ interface, the pyrealsense2 Python interface, three-camera startup, and Topic verification |

## Important Safety Information

> [!WARNING]
>
> - Real-robot programs may directly drive robot motion. Confirm that the robot state and surrounding motion area are safe before running them.
> - Before controlling the real robot, ensure that it has entered the Developer Mode required by the target program.
> - Only one `/JOINTS_CMD` publisher may run at a time.
> - If an abnormal condition occurs, use the red stop button on the gamepad to stop control immediately.
