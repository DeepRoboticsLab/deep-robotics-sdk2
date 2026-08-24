# DR02 Std Topic Examples

[Back to the DR02 Std SDK Guide](../README.md)

All examples are standalone executables. After loading the workspace environment, run an example with:

```bash
source install/setup.bash
ros2 run dr02_std <example_name> [args]
```

Before controlling the real robot, complete [Real-Robot Deployment and Control](REAL_ROBOT.md), then see [Developer Modes](DEVELOPER_MODE.md) to choose the gamepad or SDK example switching method and enter the required mode. Before simulation validation, check the Simulation Support column and see [Simulation Environment and Operation](SIMULATION.md).

> [!WARNING]
>
> Examples that publish `/JOINTS_CMD` directly must not run with `state_machine` or another `/JOINTS_CMD` publisher.
>
> Upper-body joint-control examples must be run in Upper-Body Joint Control Mode. If they are run in Whole-Body Joint Control Mode, the leg joints may lose support because they do not receive valid control commands, causing the robot to fall.

## Low-Level Examples

| Example | Topic / Interface | Real-Robot Developer Mode | Simulation Support | Command | Description |
| --- | --- | --- | --- | --- | --- |
| `joints_example` | `/JOINTS_DATA` | Whole-Body Joint Control Mode | Supported | `ros2 run dr02_std joints_example` | Prints one complete DR02 Std joint-state frame and exits. |
| `joints_example` | `/JOINTS_CMD` | Whole-Body Joint Control Mode | Supported | `ros2 run dr02_std joints_example --confirm` | Moves all 21 joints smoothly to the zero position and holds the command. |
| `arm_joint_example` | `/JOINTS_DATA`, `/JOINTS_CMD` | Upper-Body Joint Control Mode | Supported | `ros2 run dr02_std arm_joint_example --confirm` | Prints `/JOINTS_DATA` receive latency and moves the waist and both arms from their current positions to zero. |
| `arm_action_example` | `/JOINTS_CMD` | Upper-Body Joint Control Mode | Supported | `ros2 run dr02_std arm_action_example <action_id> --confirm` | Executes a preset waist and upper-body action. |

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
| `motion_info_example` | `/MOTION_INFO` | High-Level Motion Control Mode | Not supported | `ros2 run dr02_std motion_info_example [--once]` | Prints the current motion state and gait. |
| `motion_state_example` | `/MOTION_STATE` | High-Level Motion Control Mode | Not supported | `ros2 run dr02_std motion_state_example <motion_state_value>` | Publishes a motion-state transition command. The initial state is `Idle`, and the normal transition sequence is `Idle -> SuspendedStand -> RLControl`; switch to `JointDamping` in a dangerous condition. |
| `gait_example` | `/GAIT` | High-Level Motion Control Mode | Not supported | `ros2 run dr02_std gait_example <gait_value>` | Use only after the robot enters `RLControl`; publishes a gait transition command. |
| `steer_example` | `/STEER` | High-Level Motion Control Mode / Upper-Body Joint Control Mode | Not supported | `ros2 run dr02_std steer_example` | Use only after the robot enters `RLControl`. Keyboard velocity control uses a fixed conservative normalized value of `0.6`; `W/S` moves forward/backward, `A/D` moves left/right, and `Q/E` turns. Releasing a key stops the command; Ctrl+C exits. |
| `steer_example` | `/REAL_STEER` | High-Level Motion Control Mode / Upper-Body Joint Control Mode | Not supported | `ros2 run dr02_std steer_example --real` | Use only after the robot enters `RLControl`. Publishes a rotation command with `yaw=0.4` for 3 seconds by default; modify `x`, `y`, and `yaw` in the example source as needed. |

> [!WARNING]
>
> - Before switching a real robot to `RLControl`, lower it and confirm that both feet are firmly in contact with the ground. The safety suspension may remain attached, but it must not hold the robot off the ground. Switching while suspended may cause sudden motion or loss of stability, resulting in injury or equipment damage.
> - `/STEER` values `x`, `y`, and `yaw` are normalized control ratios in `[-1.0, 1.0]`, not physical velocities in m/s or rad/s. `steer_example` publishes a fixed value of `0.6`; actual motion speed depends on the robot state, gait, and control policy.
> - Hold a key to generate a continuous command. The corresponding direction returns to zero within approximately 320 ms after release. The robot may adjust its posture or lean before stepping, and some gait, surface, or load conditions may still prevent continuous movement.
> - For real-robot testing, confirm that the robot is in `RLControl` and test only one direction key at a time in a clear area. This example demonstrates Topic usage and is not intended for performance or maximum-speed testing. Do not increase the fixed value without validation.

Supported motion-state values:

| Value | Name | Description |
| --- | --- | --- |
| `0x0` | `Idle` | Zero torque; idle state. |
| `0x2` | `JointDamping` | Joint-damping soft emergency-stop state. |
| `0x11` | `RLControl` | RL control state. |
| `0x20006` | `SuspendedStand` | Suspended stand-up is complete; lower the robot and confirm that both feet contact the ground before entering `RLControl`. |

Supported gait values:

| Value | Name | Description |
| --- | --- | --- |
| `0x21001` | `HumanWALKAMP` | Humanoid walking. |
| `0x21006` | `HumanWALKTERRAIN` | Complex-terrain walking. |

## Monitoring Examples

| Example | Topic / Interface | Real-Robot Developer Mode | Simulation Support | Command | Description |
| --- | --- | --- | --- | --- | --- |
| `battery_state_example` | `/BATTERY_DATA` | Any | Not supported | `ros2 run dr02_std battery_state_example` | Prints battery level, voltage, current, and protection state. |
| `fault_snapshot_example` | `/fault_aggregator` | Any | Not supported | `ros2 run dr02_std fault_snapshot_example` | Subscribes to `drdds/msg/FaultEventArray` and prints the current-fault snapshot. |

### Current Fault Snapshot

`/fault_aggregator` uses fault-state-change triggers and periodic reporting. It immediately publishes the current active-fault snapshot when a fault is detected, also publishes when other fault states change, and continues publishing snapshots periodically. Repeated snapshots with the same content do not mean that the faults occurred again; an empty `active_faults` array means that no faults are currently active.

The example displays only the information needed to identify and assess a fault:

| Output | Description |
| --- | --- |
| Time | Local time when the fault occurred or was last updated; omitted when invalid. |
| Severity | Highest severity reported for the fault. |
| Fault code | Product-defined fault identifier; applications should use this value for matching. |
| Fault name | Human-readable fault description. |

Severity increases from `DEBUG(0)`, `INFO(1)`, `NOTICE(2)`, `WARN(3)`, `ERROR(4)`, `CRITICAL(5)`, and `ALERT(6)` to `EMERG(7)`. Product-specific protection and recovery behavior depends on the fault. The example skips unchanged periodic snapshots to avoid duplicate output; other message fields are reserved for internal diagnostics and are not displayed.

## Peripheral Examples

| Example | Topic / Interface | Real-Robot Developer Mode | Simulation Support | Command | Description |
| --- | --- | --- | --- | --- | --- |
| `imu_example` | `/IMU_DATA_BASE` | Any | Not supported | `ros2 run dr02_std imu_example` | Subscribes to `sensor_msgs/msg/Imu` and prints the base IMU quaternion, angular velocity, and linear acceleration. |
| `gamepad_key_example` | `/GAMEPAD_KEY` | Any | Not supported | `ros2 run dr02_std gamepad_key_example` | Prints gamepad key events. |

## Audio Examples

Audio examples play WAV files that already exist in the robot-side audio directory. Copy a file to `/var/opt/robot/data/audio/`, then publish the file name through `/AUDIO/PLAY_FILE` to start playback.

```bash
scp test.wav user@10.21.33.103:/var/opt/robot/data/audio/
ros2 run dr02_std audio_play_example test.wav
```

| Example | Topic / Interface | Real-Robot Developer Mode | Simulation Support | Command | Description |
| --- | --- | --- | --- | --- | --- |
| `audio_play_example` | `/AUDIO/PLAY_FILE` | Any | Not supported | `ros2 run dr02_std audio_play_example test.wav` | Publishes a remote file name to start playback. |
| `audio_volume_example` | `/AUDIO/VOLUME_CMD` | Any | Not supported | `ros2 run dr02_std audio_volume_example <0-100>` | Publishes the target volume percentage. |
| `audio_record_example` | `/AUDIO/RECORD_CMD`, `/AUDIO/RECORD_STATUS` | Any | Not supported | `ros2 run dr02_std audio_record_example` | Interactive recording control. Press `1` to start, `0` to stop, and `q` to stop and exit. Recordings are saved in `/var/opt/robot/data/audio/`; `audio_node` enforces a default maximum recording duration of 300 seconds. |
