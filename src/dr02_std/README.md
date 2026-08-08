# DR02 Std SDK Guide

This document is the English overview of the DR02 Std product package. For repository-level information and product navigation, see the [DEEPRobotics SDK root README](../../README.md).

This ROS 2 package provides the DR02 Std motion-control state machine and standalone Topic examples. Programs can run on a development host or robot-side device, and selected programs support validation with the MuJoCo simulation.

## Features

- DR02 Std RL state-machine control
- Low-level joint state, joint command, and upper-body control examples
- High-level motion state, gait, and velocity control examples
- Monitoring examples for battery state and current-fault snapshots
- Peripheral Topic examples for IMU and gamepad keys
- Audio examples for WAV playback, volume control, and recording

## Directory Layout

- `state_machine/`: main DR02 Std state-machine program
- `low_level/`: low-level joint and upper-body control Topic examples
- `high_level/`: high-level motion state, gait, and velocity control examples
- `monitoring/`: battery-state and current-fault snapshot monitoring examples
- `peripherals/`: IMU and gamepad key peripheral examples
- `audio/`: WAV playback, volume control, and recording examples
- `docs/`: real-robot control, simulation, state-machine, and example documentation

## Build Options

| Option | Default | Description |
| --- | --- | --- |
| `BUILD_DR02_STD_STATE_MACHINE` | `ON` | Build the state machine |
| `BUILD_DR02_STD_LOW_LEVEL` | `ON` | Build low-level joint and upper-body control examples |
| `BUILD_DR02_STD_HIGH_LEVEL` | `ON` | Build high-level motion, gait, and velocity examples |
| `BUILD_DR02_STD_MONITORING` | `ON` | Build monitoring examples |
| `BUILD_DR02_STD_PERIPHERALS` | `ON` | Build peripheral examples |
| `BUILD_DR02_STD_AUDIO` | `ON` | Build audio examples |
| `BUILD_SIM` | `OFF` | Enable simulation support; do not enable for real-robot control |

## Workflow

1. Select [Real-Robot Deployment and Control](docs/REAL_ROBOT.md) or [Simulation Environment and Operation](docs/SIMULATION.md) according to the control target.
2. Prepare the environment and build the SDK according to the selected document.
3. Run the [State Machine](docs/STATE_MACHINE.md) or the required [Topic Example](docs/EXAMPLES.md).

> [!IMPORTANT]
>
> The SDK runtime location and control target are independent concepts. If ROS/DDS network communication is available, an SDK process running on a development host or the AOS host (`10.21.33.103`) may directly control the real robot.

## Documentation

| Document | Contents |
| --- | --- |
| [Real-Robot Deployment and Control](docs/REAL_ROBOT.md) | Deployment on a development host or the AOS host, Developer Modes, and real-robot safety requirements |
| [Simulation Environment and Operation](docs/SIMULATION.md) | SDK simulation build, runtime procedure, and supported programs |
| [State Machine](docs/STATE_MACHINE.md) | State transitions, runtime commands, keyboard control, gamepad control, and safety requirements |
| [Topic Examples](docs/EXAMPLES.md) | Topics, real-robot Developer Modes, simulation support, runtime commands, and notes for all examples |

## Important Safety Information

> [!WARNING]
>
> - Real-robot programs may directly drive robot motion. Confirm that the robot state and surrounding motion area are safe before running them.
> - Before controlling the real robot, ensure that it has entered the Developer Mode required by the target program.
> - Only one `/JOINTS_CMD` publisher may run at a time.
> - If an abnormal condition occurs, use the red stop button on the gamepad to stop control immediately.
