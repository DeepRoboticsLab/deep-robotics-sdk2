# DR02 Pro Developer Modes

[Back to the DR02 Pro SDK Guide](../README.md)

This document describes the DR02 Pro Developer Modes, their control scopes, and the procedures for switching modes with the gamepad or the SDK example. Developer Modes are for real-robot control only.

<a id="developer-mode"></a>

## Mode Overview

Developer Mode grants the SDK access to the robot's motion-control capabilities. It provides High-Level Motion Control Mode, Whole-Body Joint Control Mode, and Upper-Body Joint Control Mode. Confirm the mode required by the target program before running it.

| Developer Mode | Control Scope | Applicable Examples |
| --- | --- | --- |
| High-Level Motion Control Mode | The SDK controls motion through high-level commands such as `/MOTION_STATE`, `/GAIT`, and `/STEER`; the robot's internal control policy controls the joints | [High-level examples](EXAMPLES.md#high-level-examples) |
| Whole-Body Joint Control Mode | The SDK directly controls all joints through `/JOINTS_CMD` | [State Machine](STATE_MACHINE.md), [low-level examples](EXAMPLES.md#low-level-examples) |
| Upper-Body Joint Control Mode | The SDK controls the waist and both arms through `/JOINTS_CMD`; the robot's internal policy controls the leg joints | [Low-level examples](EXAMPLES.md#low-level-examples) |

### Usage Notes

- Before entering Developer Mode, confirm that the robot is in the idle state.
- After entering Developer Mode, perception, localization, and obstacle-avoidance functions are disabled. Do not rely on these functions to ensure motion safety.
- The three Developer Modes cannot be switched directly. Before selecting another mode, stop the SDK program and fully exit the current mode.
- Only one `/JOINTS_CMD` publisher may run at a time.

See the [State Machine](STATE_MACHINE.md) document for its required mode and [Topic Examples](EXAMPLES.md) for the mode required by each example.

## Switching Methods

1. **Switch with the gamepad:** Configure and enter Developer Mode through the gamepad for the normal real-robot operation flow.
2. **Switch with the SDK example:** Run `developer_mode_example` to switch modes through the `/DEVELOPER_MODE` service and related state Topics.

These methods are independent. Do not operate both at the same time.

<a id="handle-switching"></a>

## Switch with the Gamepad

On the gamepad, open Settings - Auxiliary Functions - Developer Mode Settings, enable Developer Mode, and select the target control mode. The selection is retained; when the target mode does not change, it does not need to be configured again. After configuring the mode, follow the corresponding procedure below from the gamepad main screen to enter it.

### High-Level Motion Control Mode

- Enter:
  1. Confirm that the robot is in the idle state.
  2. Return to the gamepad main screen and use the entry in the upper-left corner to switch to Developer Mode.
  3. After the mode transition completes, select High-Level Control Mode and wait for it to be enabled.
- Exit:
  1. Exit the SDK program in its terminal and confirm that the program and related control Topic publishers have stopped.
  2. Switch the robot back to the idle state.
  3. Select Exit Development on the gamepad main screen.
  4. After confirming that the robot is in the idle state, use the entry in the upper-left corner of the main screen to exit Developer Mode.

### Whole-Body Joint Control Mode

- Enter:
  1. Confirm that the robot is in the idle state.
  2. Return to the gamepad main screen and use the entry in the upper-left corner to switch to Developer Mode.
  3. After the mode transition completes, select Whole-Body Control Mode and wait for it to be enabled.
- Exit:
  1. Exit the SDK program in its terminal and confirm that the program and related control Topic publishers have stopped.
  2. Select Exit Development on the gamepad main screen.
  3. Switch the robot back to the idle state, then use the entry in the upper-left corner of the main screen to exit Developer Mode.

### Upper-Body Joint Control Mode

- Enter:
  1. Confirm that the robot is in the idle state.
  2. Return to the gamepad main screen and use the entry in the upper-left corner to switch to Developer Mode.
  3. Use the gamepad interface to bring the robot to the suspended-standing position.
  4. Select Start Motion.
  5. Select Upper-Body Control Mode and wait for the mode transition to complete.
- Exit:
  1. Exit the SDK program in its terminal and confirm that the program and related control Topic publishers have stopped.
  2. Select Exit Development on the gamepad main screen.
  3. Switch the robot back to the idle state, then use the entry in the upper-left corner of the main screen to exit Developer Mode.

<a id="sdk-example-switching"></a>

## Switch with the SDK Example

`developer_mode_example` enters or exits real-robot Developer Mode and asks for confirmation before potentially dangerous operations.

When entering Upper-Body Joint Control Mode, the example also uses `/MOTION_STATE` to change the motion state and `/MOTION_INFO` to confirm the transition.

```bash
ros2 run dr02_pro developer_mode_example high_level
ros2 run dr02_pro developer_mode_example whole_body
ros2 run dr02_pro developer_mode_example upper_body

ros2 run dr02_pro developer_mode_example exit
ros2 run dr02_pro developer_mode_example damping_exit
```

Arguments:

- `high_level`: enter High-Level Motion Control Mode for high-level motion-control examples.
- `whole_body`: enter Whole-Body Joint Control Mode for the state machine and whole-body joint-control examples.
- `upper_body`: enter Upper-Body Joint Control Mode for waist and arm joint-control examples. Before entering `RLControl`, lower the robot and confirm that both feet are firmly in contact with the ground.
- `exit`: perform a normal exit for the current control mode. In Upper-Body Joint Control Mode, return to `RLControl` before continuing the exit; if the robot is already in `Idle`, skip the damping transition.
- `damping_exit`: quickly exit the current control. It can be used during normal operation or an abnormal condition. When the robot is not in `Idle`, it enters the damping state first and may lose active support; when already in `Idle`, the damping transition is skipped.

### Service Interface

`/DEVELOPER_MODE` switches Developer Mode.

| Item | Description |
| --- | --- |
| Service | `/DEVELOPER_MODE` |
| Type | `drdds/srv/StdSrvString` |
| Request field | `command`, a JSON string |
| Response field | `result`; `success` indicates success, while other values indicate that the operation is incomplete or rejected |

The `command` field uses the following JSON format:

| Operation | JSON Example | Purpose |
| --- | --- | --- |
| `enable` | `{ "Action": "enable" }` | Enable Developer Mode access and wait for `Enabled=true`, `Mode=0` |
| `set_mode` | `{ "Action": "set_mode", "Mode": 1, "Frequency": 500 }` | Select a Developer Mode and set the joint-control data frequency; the frequency range is `0` to `500` |
| `disable` | `{ "Action": "disable" }` | Exit Developer Mode access |

`Mode` values for `set_mode`:

| `Mode` | SDK Name | Control Mode |
| --- | --- | --- |
| `0` | `None` | Clear the current control mode |
| `1` | `UpperBody` | Upper-Body Joint Control Mode |
| `2` | `WholeBody` | Whole-Body Joint Control Mode |
| `3` | `HighLevel` | High-Level Motion Control Mode |

<br>

`/DEVELOPER_MODE_STATUS` reports the current status. Its type is `std_msgs/msg/String`, and the `data` field contains a JSON string such as:

```json
{"Enabled":true,"Mode":2,"Frequency":500}
```

`Enabled` indicates whether Developer Mode access is enabled, `Mode` indicates the current control mode, and `Frequency` indicates the current joint-control data frequency. High-Level Motion Control Mode does not include `Frequency`. The example uses this status to confirm whether service operations took effect.

## Emergency Stop

If an abnormal condition occurs or control must be stopped immediately, press the red stop button on the gamepad first. After the robot returns to a stable state, confirm that the SDK program and its control Topic publishers have stopped, then exit Developer Mode.
