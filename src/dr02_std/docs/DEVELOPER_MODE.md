# DR02 Std Developer Mode

[Back to the DR02 Std SDK Guide](../README.md)

This document describes the purpose of DR02 Std Developer Mode, its three development modes, and how to switch modes with the gamepad or the SDK example. Developer Mode is for real-robot control only.

<a id="developer-mode"></a>

## 1. Mode Overview

Developer Mode grants the SDK access to the robot's motion-control capabilities. It includes High-Level, Whole-Body, and Upper-Body Development Modes. The three modes provide different control scopes; confirm which mode the target program requires before running it.

| Mode | Name | Control Layer | Control Method | Applicable Examples |
| --- | --- | --- | --- | --- |
| 0 | High-Level Development | High-Level | Use `/MOTION_STATE`, `/GAIT`, and `/STEER` for high-level interfaces; the joints are controlled by the internal policy | [High-Level Examples](EXAMPLES.md#2-high-level-examples) |
| 1 | Upper-Body Development | Low-Level Joint | The SDK controls the waist and both arms through `/JOINTS_CMD`; the legs can receive movement commands from the gamepad or `/STEER` | [Low-Level Examples](EXAMPLES.md#1-low-level-examples) |
| 2 | Whole-Body Development | Low-Level Joint | The SDK directly controls all joints through `/JOINTS_CMD` | [State Machine](STATE_MACHINE.md), [Low-Level Examples](EXAMPLES.md#1-low-level-examples) |

### 1.1 Mode Switching Requirements

| Mode | Entry Condition | Standard Exit | Quick Damping Exit |
| --- | --- | --- | --- |
| High-Level Development | The default mode after entering Developer Mode | If not in `Idle`, switch to `JointDamping`, wait until it returns to `Idle`, then exit Developer Mode; if already in `Idle`, exit directly | If not in `Idle`, switch to `JointDamping`, wait until it returns to `Idle`, then exit Developer Mode |
| Upper-Body Development | In High-Level Development Mode, the motion state is `RLControl` | Return to High-Level Development Mode with the motion state in `RLControl`, then follow the High-Level Development Mode exit procedure | If not in `Idle`, switch directly to `JointDamping`, wait until it returns to `Idle`, then exit Developer Mode |
| Whole-Body Development | In High-Level Development Mode, the motion state is `Idle` | Return to High-Level Development Mode, transition through damping to `Idle`, then exit Developer Mode | If not in `Idle`, switch directly to `JointDamping`, wait until it returns to `Idle`, then exit Developer Mode |

> [!WARNING]
>
> Entering damping may cause the robot to lose active support. Provide fall protection beforehand.

> [!NOTE]
>
> - Both the gamepad and the SDK example support **standard exit** and **quick damping exit**.
> - Upper-Body Development Mode and Whole-Body Development Mode cannot be switched directly. Stop the SDK program and return to High-Level Development Mode before switching.
> - Only one publisher may publish `/JOINTS_CMD` at a time.

## 2. Switching Methods

1. **Switch with the gamepad:** Configure and enter Developer Mode with the gamepad, which is suitable for the on-site real-robot operation flow.
2. **Switch with the SDK example:** Run `developer_mode_example` to switch modes through the `/DEVELOPER_MODE` service and related state Topics, which is suitable for completing the workflow from a terminal.

These methods are independent; do not operate both at the same time.

<a id="handle-switching"></a>

## 3. Switching with the Gamepad

Enable Developer Mode in the gamepad settings under Auxiliary Functions - Developer Mode Settings. Then follow the procedures below to enter the required development mode.

### 3.1 High-Level Development Mode

- Enter:
  1. Confirm that the robot is in the idle state.
  2. Use the mode switch button in the upper-left corner of the gamepad main screen to enter Developer Mode.
  3. High-Level Development Mode is selected by default.
- Standard exit:
  1. Exit the SDK program in its terminal and confirm that the program and related control Topic publishers have stopped.
  2. Press the damping control and wait for the robot to return to the idle state.
  3. Use the mode switch button in the upper-left corner of the main screen to exit Developer Mode.

### 3.2 Whole-Body Development Mode

- Enter:
  1. Confirm that the robot is in the idle state.
  2. Use the mode switch button in the upper-left corner of the gamepad main screen to enter Developer Mode.
  3. Select Whole-Body Development; the mode transition is complete.
- Standard exit:
  1. Exit the SDK program in its terminal and confirm that the program and related control Topic publishers have stopped.
  2. Select Exit Development on the gamepad main screen to return to High-Level Development Mode and wait for the robot to return to the idle state.
  3. Use the mode switch button in the upper-left corner of the main screen to exit Developer Mode.

### 3.3 Upper-Body Development Mode

- Enter:
  1. Confirm that the robot is in the idle state.
  2. Use the mode switch button in the upper-left corner of the gamepad main screen to enter Developer Mode.
  3. **Lift the robot and confirm that both feet are off the ground**, then select Suspended Stand.
  4. **Lower the robot and confirm that both feet are fully in contact with the ground**, then select Start Motion.
  5. Select Upper-Body Development; the mode transition is complete.
- Standard exit:
  1. Exit the SDK program in its terminal and confirm that the program and related control Topic publishers have stopped.
  2. Select Exit Development on the gamepad main screen to return to High-Level Development Mode.
  3. Press the damping control and wait for the robot to return to the idle state.
  4. Use the mode switch button in the upper-left corner of the main screen to exit Developer Mode.

<a id="sdk-example-switching"></a>

## 4. Switching with the SDK

`developer_mode_example` enters or exits real-robot Developer Mode. It is a complete target-workflow example that changes states and asks for confirmation during dangerous stages.

> ⚠️ **Safety confirmation when running `upper_body`**
>
> Upper-Body Development Mode requires the `RLControl` motion state. When the example switches from `Idle` to `RLControl`, follow its prompts:
>
> 1. Lift the robot and confirm that both feet are off the ground.
> 2. Switch to `SuspendedStand`.
> 3. Lower the robot and confirm that both feet are fully in contact with the ground.
> 4. Confirm that the posture is stable, then switch to `RLControl`.

```bash
ros2 run dr02_std developer_mode_example high_level
ros2 run dr02_std developer_mode_example whole_body
ros2 run dr02_std developer_mode_example upper_body

ros2 run dr02_std developer_mode_example exit
ros2 run dr02_std developer_mode_example damping_exit
```

Arguments:

- `high_level`: enter High-Level Development Mode following the procedure.
- `whole_body`: enter Whole-Body Development Mode following the procedure.
- `upper_body`: enter Upper-Body Development Mode following the procedure.
- `exit`: follow the standard procedure to exit Developer Mode.
- `damping_exit`: exit Developer Mode through quick damping.

### 4.1 Service Interface

`/DEVELOPER_MODE` enters or exits Developer Mode and selects a development mode.

| Item | Content |
| --- | --- |
| Service Name | `/DEVELOPER_MODE` |
| Service Type | `drdds/srv/StdSrvString` |
| Request Field | `command`, a JSON string |
| Response Field | `result`; `success` indicates success, while other values indicate that the operation is incomplete or rejected |

The `command` field uses the following JSON format:

| Action | Request Example | Purpose |
| --- | --- | --- |
| `enable` | `{"Action":"enable"}` | Enter Developer Mode; the successful state is `Enabled=true, Mode=0` |
| `set_mode` | `{ "Action": "set_mode", "Mode": 1, "Frequency": 500 }` | Select a development mode; for Modes 1 and 2, `Frequency` ranges from `1` to `500`; Mode 0 ignores this field |
| `disable` | `{"Action":"disable"}` | Exit Developer Mode when in Mode 0 and `Idle` |

The `Mode` values for `set_mode` are:

| `Mode` | SDK Name | Development Mode |
| --- | --- | --- |
| `0` | `HighLevel` | High-Level Development Mode |
| `1` | `UpperBody` | Upper-Body Development Mode |
| `2` | `WholeBody` | Whole-Body Development Mode |

<br>

`/DEVELOPER_MODE_STATUS` confirms the current state. Its type is `std_msgs/msg/String`, and its `data` field is a JSON string. For example:

```json
{"Enabled":true,"Mode":2,"Frequency":500}
```

`Enabled` indicates whether Developer Mode is enabled, `Mode` indicates the current development mode, and `Frequency` indicates the current joint-control data frequency. High-Level Development Mode does not include the `Frequency` field. The example uses this status to determine whether a service operation took effect.

## 5. Emergency Stop

If an abnormal condition occurs or an immediate stop is required, press the red stop button on the gamepad first. After the robot's state has recovered, confirm that the SDK program and control Topic publishers have stopped before switching development modes again.
