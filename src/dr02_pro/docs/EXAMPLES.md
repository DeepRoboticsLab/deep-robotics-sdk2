# DR02 Pro Topic Examples

[Back to the DR02 Pro SDK Guide](../README.md)

All examples are standalone executables. After loading the workspace environment, run an example with:

```bash
source install/setup.bash
ros2 run dr02_pro <example_name> [args]
```

Before controlling the real robot, complete [Real-Robot Deployment and Control](REAL_ROBOT.md) and enter the Developer Mode listed below. Before simulation validation, check the Simulation Support column and see [Simulation Environment and Operation](SIMULATION.md).

> [!WARNING]
>
> Examples that publish `/JOINTS_CMD` directly must not run with `state_machine` or another `/JOINTS_CMD` publisher.
>
> Upper-body joint-control examples must be run in Upper-Body Joint Control Mode. If they are run in Whole-Body Joint Control Mode, the leg joints may lose support because they do not receive valid control commands, causing the robot to fall.

## Low-Level Examples

| Example | Topic / Interface | Real-Robot Developer Mode | Simulation Support | Command | Description |
| --- | --- | --- | --- | --- | --- |
| `joints_example` | `/JOINTS_DATA` | Whole-Body Joint Control Mode | Supported | `ros2 run dr02_pro joints_example` | Prints one complete DR02 Pro joint-state frame and exits. |
| `joints_example` | `/JOINTS_CMD` | Whole-Body Joint Control Mode | Supported | `ros2 run dr02_pro joints_example --confirm` | Moves all 31 joints smoothly to the zero position and holds the command. |
| `arm_joint_example` | `/JOINTS_DATA`, `/JOINTS_CMD` | Upper-Body Joint Control Mode | Supported | `ros2 run dr02_pro arm_joint_example --confirm` | Prints `/JOINTS_DATA` receive latency and moves the waist and both arms from their current positions to zero. |
| `arm_action_example` | `/JOINTS_CMD` | Upper-Body Joint Control Mode | Supported | `ros2 run dr02_pro arm_action_example <action_id> --confirm` | Executes a preset waist and upper-body action. |

Preset upper-body action IDs:

| ID | Name |
| --- | --- |
| `0` | `greeting` |
| `1` | `kiss` |
| `2` | `handshake` |
| `3` | `salute` |
| `4` | `salute2` |

## High-Level Examples

| Example | Topic / Interface | Real-Robot Developer Mode | Simulation Support | Command | Description |
| --- | --- | --- | --- | --- | --- |
| `motion_info_example` | `/MOTION_INFO` | High-Level Motion Control Mode | Not supported | `ros2 run dr02_pro motion_info_example [--once]` | Prints the current motion state and gait. |
| `motion_state_example` | `/MOTION_STATE` | High-Level Motion Control Mode | Not supported | `ros2 run dr02_pro motion_state_example <motion_state_value>` | Publishes a motion-state transition command. The initial state is `Idle`, and the normal transition sequence is `Idle -> SuspendedStand -> RLControl`; switch to `JointDamping` in a dangerous condition. |
| `gait_example` | `/GAIT` | High-Level Motion Control Mode | Not supported | `ros2 run dr02_pro gait_example <gait_value>` | Use only after the robot enters `RLControl`; publishes a gait transition command. |
| `steer_example` | `/STEER` | High-Level Motion Control Mode | Not supported | `ros2 run dr02_pro steer_example` | Use only after the robot enters `RLControl`. Keyboard velocity control uses a fixed conservative normalized value of `0.6`; `W/S` moves forward/backward, `A/D` moves left/right, and `Q/E` turns. Releasing a key stops the command; Ctrl+C exits. |
| `steer_example` | `/REAL_STEER` | High-Level Motion Control Mode | Not supported | `ros2 run dr02_pro steer_example --real` | Use only after the robot enters `RLControl`. Publishes a rotation command with `yaw=0.4` for 3 seconds by default; modify `x`, `y`, and `yaw` in the example source as needed. |

> [!WARNING]
>
> - `/STEER` values `x`, `y`, and `yaw` are normalized control ratios in `[-1.0, 1.0]`, not physical velocities in m/s or rad/s. `steer_example` publishes a fixed value of `0.6`; actual motion speed depends on the robot state, gait, and control policy.
> - Hold a key to generate a continuous command. The corresponding direction returns to zero within approximately 320 ms after release. The robot may adjust its posture or lean before stepping, and some gait, surface, or load conditions may still prevent continuous movement.
> - For real-robot testing, confirm that the robot is in `RLControl` and test only one direction key at a time in a clear area. This example demonstrates Topic usage and is not intended for performance or maximum-speed testing. Do not increase the fixed value without validation.

Supported motion-state values:

| Value | Name | Description |
| --- | --- | --- |
| `0x0` | `Idle` | Zero torque; idle state. |
| `0x2` | `JointDamping` | Joint-damping soft emergency-stop state. |
| `0x11` | `RLControl` | RL control state. |
| `0x20006` | `SuspendedStand` | Suspended stand-up completed; the robot can enter RL control mode. |

Supported gait values:

| Value | Name | Description |
| --- | --- | --- |
| `0x21001` | `HumanWALKAMP` | Humanoid walking. |
| `0x21006` | `HumanWALKTERRAIN` | Complex-terrain walking. |

## Peripheral Examples

| Example | Topic / Interface | Real-Robot Developer Mode | Simulation Support | Command | Description |
| --- | --- | --- | --- | --- | --- |
| `battery_state_example` | `/BATTERY_DATA` | Any | Not supported | `ros2 run dr02_pro battery_state_example` | Prints battery level, voltage, current, and protection state. |
| `imu_example` | `/IMU_DATA_HEAD`, `/IMU_DATA_BASE` | Any | Not supported | `ros2 run dr02_pro imu_example` | Subscribes to `sensor_msgs/msg/Imu` and prints head and base IMU quaternions, angular velocity, and linear acceleration. |
| `gamepad_key_example` | `/GAMEPAD_KEY` | Any | Not supported | `ros2 run dr02_pro gamepad_key_example` | Prints gamepad key events. |

## Audio Examples

Audio examples play WAV files that already exist in the robot-side audio directory. Copy a file to `/var/opt/robot/data/audio/`, then publish the file name through `/AUDIO/PLAY_FILE` to start playback.

```bash
scp test.wav user@10.21.33.103:/var/opt/robot/data/audio/
ros2 run dr02_pro audio_play_example test.wav
```

| Example | Topic / Interface | Real-Robot Developer Mode | Simulation Support | Command | Description |
| --- | --- | --- | --- | --- | --- |
| `audio_play_example` | `/AUDIO/PLAY_FILE` | Any | Not supported | `ros2 run dr02_pro audio_play_example test.wav` | Publishes a remote file name to start playback. |
| `audio_volume_example` | `/AUDIO/VOLUME_CMD` | Any | Not supported | `ros2 run dr02_pro audio_volume_example <0-100>` | Publishes the target volume percentage. |
| `audio_record_example` | `/AUDIO/RECORD_CMD`, `/AUDIO/RECORD_STATUS` | Any | Not supported | `ros2 run dr02_pro audio_record_example` | Interactive recording control. Press `1` to start, `0` to stop, and `q` to stop and exit. Recordings are saved in `/var/opt/robot/data/audio/`; `audio_node` enforces a default maximum recording duration of 300 seconds. |
