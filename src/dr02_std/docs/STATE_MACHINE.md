# DR02 Std State Machine

[Back to the DR02 Std SDK Guide](../README.md)

The DR02 Std state machine subscribes to `/JOINTS_DATA`, publishes `/JOINTS_CMD`, and receives `/STEER` and `/REAL_STEER` while in the RL control state.

## 1. Operation

### 1.1 Real-Robot Control

Before running on the real robot, complete [Real-Robot Deployment](REAL_ROBOT.md) and follow the [Developer Modes](DEVELOPER_MODE.md#handle-switching) procedure to enter Whole-Body Joint Control Mode.

> [!IMPORTANT]
>
> - The current state-machine code supports Whole-Body Joint Control Mode by default.
> - To run in Upper-Body Joint Control Mode, modify the state-machine code according to the required control behavior.
> - Validate the modified code in the simulation environment before deploying it to the real robot.

> [!WARNING]
>
> Do not run another `/JOINTS_CMD` publisher while the state machine is running.

### 1.2 Simulation

The state machine supports operation with the MuJoCo simulation. See [Simulation Environment and Operation](SIMULATION.md) for SDK-side build and runtime instructions.

## 2. Start the State Machine

```bash
source install/setup.bash
ros2 run dr02_std state_machine
```

## 3. State Transitions

```text
Idle -> ZeroPos -> RLControl
```

- `Idle`: holds the initial command and waits for a user request.
- `ZeroPos`: moves the robot to the zero position.
- `RLControl`: runs the RL policy and receives `/STEER` and `/REAL_STEER`.
- `JointDamping`: damping state used by RL control and safety handling.

## 4. Keyboard Control

- `z`: enter `ZeroPos`
- `c`: enter `RLControl` from `ZeroPos`
- `r`: enter `JointDamping`
- `w/s`: move forward / backward in RL control
- `a/d`: move left / right in RL control
- `q/e`: turn left / right in RL control

## 5. Gamepad Control

- L1: enter `ZeroPos`
- L2: enter `RLControl` from `ZeroPos`
- R2: enter `JointDamping`
- Left joystick: forward / left / backward / right command
- Right joystick: turning command

## 6. Safety Requirements

> [!WARNING]
>
> - Before running the state machine on the real robot, confirm that the robot has entered the correct Developer Mode.
> - Before entering `ZeroPos` or `RLControl`, confirm that the robot state and surrounding motion area are safe.
> - Before switching a real robot to `RLControl`, lower it and confirm that both feet are firmly in contact with the ground. The safety suspension may remain attached, but it must not hold the robot off the ground. Switching while suspended may cause sudden motion or loss of stability, resulting in injury or equipment damage.
> - Only one `/JOINTS_CMD` publisher may run at a time.
> - If an abnormal condition occurs, use the red stop button on the gamepad to stop control immediately.
