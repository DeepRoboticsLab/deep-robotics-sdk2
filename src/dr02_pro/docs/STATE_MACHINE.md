# DR02 Pro State Machine

[Back to the DR02 Pro SDK Guide](../README.md)

The DR02 Pro state machine subscribes to `/JOINTS_DATA`, publishes `/JOINTS_CMD`, uses `/STEER` and `/REAL_STEER` commands for AMP control, and executes reference motions through Mimic control.

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
ros2 run dr02_pro state_machine
```

## 3. State Transitions

```text
Idle -> ZeroPos -> StandUp -> RLControlAMP
                          -> RLControlMimic
RLControlAMP <-> RLControlMimic
```

- `Idle`: holds the initial command and waits for a user request.
- `ZeroPos`: moves the robot to the zero position.
- `StandUp`: interpolates to the policy start pose for about 3 seconds after a policy request from zero position, then enters the requested policy.
- `RLControlAMP`: runs the AMP policy and handles steering commands.
- `RLControlMimic`: runs the Mimic policy with reference motion and automatically returns to AMP when the motion finishes.
- `JointDamping`: damping state requested by the user or safety handling.

Manual AMP/Mimic switches are direct, without `StandUp`; the code checks steering
commands, body posture, and joint velocity before allowing the switch. If those
checks fail, the current policy remains active. Automatic return after a completed
Mimic motion is a separate transition.
See the [README workflow](../README.md#52-deploy-an-amp-policy) for policy preparation and deployment.

## 4. Keyboard Control

- `z`: enter `ZeroPos` from `Idle`
- `c`: request AMP through `StandUp` from `ZeroPos`, or request AMP from Mimic
- `v`: request Mimic through `StandUp` from `ZeroPos`, or request Mimic from AMP
- `r`: request `JointDamping`
- `w/s`: move forward / backward in AMP control
- `a/d`: move left / right in AMP control
- `q/e`: turn left / right in AMP control

## 5. Gamepad Control

- L1: enter `ZeroPos` from `Idle`
- L2: request AMP through `StandUp` from `ZeroPos`
- R1: request Mimic from AMP
- R2: request `JointDamping`
- Left joystick: forward/backward and lateral commands for AMP
- Right joystick: turning command for AMP

Real-robot Developer Mode switching and SDK state-machine control are separate
operations. Complete [Developer Mode](DEVELOPER_MODE.md) preparation first, then
use the state-machine controls above.

## 6. Safety Requirements

> [!WARNING]
>
> - Before running the state machine on the real robot, confirm that the robot has entered the correct Developer Mode.
> - Before entering `ZeroPos`, `StandUp`, or policy control, confirm that the robot state and surrounding motion area are safe.
> - Before requesting AMP or Mimic on a real robot (including entry to `StandUp`), lower it and confirm that both feet are firmly in contact with the ground. The safety suspension may remain attached, but it must not hold the robot off the ground. Switching while suspended may cause sudden motion or loss of stability, resulting in injury or equipment damage.
> - Only one `/JOINTS_CMD` publisher may run at a time.
> - If an abnormal condition occurs, use the red stop button on the gamepad to stop control immediately.
