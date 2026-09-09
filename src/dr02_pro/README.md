# DR02 Pro SDK Guide

This document is the English overview of the DR02 Pro product package. For repository-level information and product navigation, see the [DEEPRobotics SDK root README](../../README.md).

This ROS 2 package provides the DR02 Pro motion-control state machine and standalone Topic examples. Programs can run on a development host or robot-side device, and selected programs support validation with the MuJoCo simulation.

The AOS (`10.21.33.103`) runs Ubuntu 24.04 / ROS 2 Jazzy on ARM64 without internet access. The NOS (`10.21.33.106`) runs Ubuntu 22.04 / ROS 2 Humble, also on ARM64 without internet access. See [Real-Robot Deployment](docs/REAL_ROBOT.md) for offline message installation and source transfer.

## 1. Features

- DR02 Pro RL state-machine control
- Low-level joint state, joint command, and upper-body control examples
- High-level motion state, gait, velocity control, and Developer Mode switching examples
- Monitoring examples for battery state and current-fault snapshots
- Peripheral Topic examples for IMU and gamepad keys
- RealSense depth-camera deployment and usage documentation
- Audio examples for WAV playback, volume control, and recording

## 2. Directory Layout

- `state_machine/`: main DR02 Pro state-machine program
- `low_level/`: low-level joint and upper-body control Topic examples
- `high_level/`: high-level motion state, gait, velocity control, and Developer Mode switching examples
- `monitoring/`: battery-state and current-fault snapshot monitoring examples
- `peripherals/`: IMU and gamepad key peripheral examples
- `audio/`: WAV playback, volume control, and recording examples
- `docs/`: real-robot deployment, Developer Mode, simulation, state-machine, example, and camera documentation

## 3. Documentation

| Document | Contents |
| --- | --- |
| [Real-Robot Deployment and Control](docs/REAL_ROBOT.md) | Deployment and build on a development host, the AOS host, or the NOS host |
| [Developer Modes](docs/DEVELOPER_MODE.md) | Developer Mode overview, gamepad switching, and SDK example switching procedures |
| [Simulation Environment and Operation](docs/SIMULATION.md) | SDK simulation build, runtime procedure, and supported programs |
| [State Machine](docs/STATE_MACHINE.md) | State transitions, runtime commands, keyboard control, gamepad control, and safety requirements |
| [Topic Examples](docs/EXAMPLES.md) | Topics, real-robot Developer Modes, simulation support, runtime commands, and notes for all examples |
| [RealSense Cameras](docs/CAMERA.md) | Camera driver installation on the NOS host, ROS 2 driver usage, the librealsense C/C++ interface, the pyrealsense2 Python interface, three-camera startup, and Topic verification |
| [Joint Control Interface](docs/JOINT_CONTROL.md) | Joint message fields, array order, controllable ranges, units, and zero-position definition |

## 4. Quick Start

For the first run, select the environment setup document according to the control target, then follow the links in the next-step column for the required mode and examples.

| Control target | Environment setup | Next step |
| --- | --- | --- |
| Real robot | [Real-Robot Deployment and Control](docs/REAL_ROBOT.md) | Read [Developer Mode](docs/DEVELOPER_MODE.md), then choose [State Machine](docs/STATE_MACHINE.md) or [Topic Examples](docs/EXAMPLES.md) |
| Simulation | [Simulation Environment and Operation](docs/SIMULATION.md) | Choose [State Machine](docs/STATE_MACHINE.md) or [Topic Examples](docs/EXAMPLES.md) according to simulation support |

> [!IMPORTANT]
>
> The SDK runtime location and control target are independent concepts. If ROS/DDS network communication is available, an SDK process running on a development host, the AOS host (`10.21.33.103`), or the NOS host (`10.21.33.106`) may directly control the real robot.

## 5. Workflows by Task

### 5.1 How the Repositories Fit Together

The policy workflow spans the following repositories:

| Repository | Role |
| --- | --- |
| [rl_training](https://github.com/DeepRoboticsLab/rl_training) | Train an AMP policy and export `policy.onnx`. |
| [deep-robotics-retarget](https://github.com/DeepRoboticsLab/deep-robotics-retarget) | Retarget a motion to the robot for Mimic tracking. |
| [deep-robotics-mimic](https://github.com/DeepRoboticsLab/deep-robotics-mimic) | Prepare the reference motion, train a Mimic policy to track it, and export ONNX. |
| This SDK | Load the exported policy, run inference, and send joint commands. |
| [deep-robotics-simulation](https://github.com/DeepRoboticsLab/deep-robotics-simulation) | Run the MuJoCo robot and scene, receive `/JOINTS_CMD`, and return joint/IMU feedback for SDK validation. |
| [deep-robotics-msg](https://github.com/DeepRoboticsLab/deep-robotics-msg.git) | Provide the shared `drdds` message interfaces. |

AMP: **train → export ONNX → replace the SDK model → validate in simulation → deploy to the robot**.
Mimic: **retarget motion → prepare the tracking motion and train → export ONNX → load the motion/model pair in the SDK → validate → deploy**.

For simulation, read both the [SDK simulation guide](docs/SIMULATION.md) and the
[simulator's product guide](https://github.com/DeepRoboticsLab/deep-robotics-simulation/blob/main/dr02_pro/README.md). Run them in separate terminals with
the same ROS distribution, message version, and `ROS_DOMAIN_ID`. Set the simulator's
`control.mode` to `whole` for the state machine or whole-body joint examples, and
`upper` for arm examples. The simulator's `upper` mode constrains the base and legs;
on the real robot, Upper-Body Joint Control Mode leaves the legs under the robot's
internal controller. Simulation mode selection does not switch a real robot's
Developer Mode.

### 5.2 Deploy an AMP Policy

Before hardware deployment, read **[Real-Robot Deployment](docs/REAL_ROBOT.md) →
[Developer Modes](docs/DEVELOPER_MODE.md) → [State Machine](docs/STATE_MACHINE.md)**
in that order. They explain where to transfer/build the SDK, which robot control
mode to enter, and how to start, switch, and stop policy control. Replacing the
model prepares the policy; simulation validation and these hardware operating
steps still follow.

Before deployment, complete [message installation](../../README.md#4-install-the-message-interfaces)
and read [Joint Control](docs/JOINT_CONTROL.md) for the robot's command interface.

1. **Train and export AMP.** Follow the DR02 AMP training and ONNX export instructions
   in [rl_training](https://github.com/DeepRoboticsLab/rl_training). If you already
   have the exported `policy.onnx`, start at the next step.
2. **Replace the model file.** Replace this product's
   [state_machine/policy/policy.onnx](state_machine/policy/policy.onnx) with the
   newly exported `policy.onnx`, keeping the same filename. The observations,
   actions, and other deployment settings for this training workflow already
   match the SDK, so no runner or observation changes are needed. If the SDK is
   already built, replacing only this ONNX file requires restarting
   `state_machine`, not recompiling C++.
3. **Prepare the simulator.** Follow its
   [environment setup](https://github.com/DeepRoboticsLab/deep-robotics-simulation/blob/main/README.md#3-environment-setup)
   and [product configuration](https://github.com/DeepRoboticsLab/deep-robotics-simulation/blob/main/dr02_pro/README.md). Use `control.mode: whole`
   and a scene with collidable ground. Start `python3 run_sim.py dr02_pro`
   from the simulation repository.
4. **Build and run the SDK for simulation.** Follow
   [Simulation](docs/SIMULATION.md) with `BUILD_SIM=ON`, then
   [State Machine](docs/STATE_MACHINE.md). For AMP, the implemented path is
   `Idle -> ZeroPos -> StandUp -> RLControlAMP`: request zero position with `z`, wait for it to settle,
   then request AMP with `c`. Validate standing, steering, and damping before
   moving to hardware; only one `/JOINTS_CMD` publisher may run.
5. **Transfer and rebuild on the target.** Follow
   [Real-Robot Deployment](docs/REAL_ROBOT.md) for the selected host. On the
   offline AOS, transfer the message source and use the installer's `--source-dir`
   option. Transfer the SDK source and policy assets, then make a fresh native
   ARM build with `BUILD_PLATFORM=arm` and `BUILD_SIM=OFF`. The policy path is
   resolved relative to the source location embedded at compilation, so retain
   the source/assets on the target; copying only the executable is insufficient.
6. **Enter the real-robot control mode and operate.** Read
   [Developer Modes](docs/DEVELOPER_MODE.md) to enter **Whole-Body Joint Control
   Mode**, then follow [State Machine](docs/STATE_MACHINE.md) and its safety
   requirements. Confirm foot contact before entering policy control and use
   the documented stop procedure. Revalidate any policy or controller changes
   in simulation before repeating this step.

The Pro state machine initializes both AMP and Mimic runners at startup, so keep
the selected Mimic model and reference motion available even when testing AMP only.

### 5.3 Deploy a Mimic Policy

For hardware deployment, first read **[Real-Robot Deployment](docs/REAL_ROBOT.md) →
[Developer Modes](docs/DEVELOPER_MODE.md) → [State Machine](docs/STATE_MACHINE.md)**
for the target host, Whole-Body Joint Control Mode, and AMP/Mimic switching and stopping.

A Mimic policy needs the reference motion it was trained to track. Deploy the
ONNX and that motion together; `fist_routine` is the bundled example.

1. **Prepare the motion.** Follow
   [deep-robotics-retarget](https://github.com/DeepRoboticsLab/deep-robotics-retarget)
   to retarget the source motion. Then follow the
   [Mimic data pipeline](https://github.com/DeepRoboticsLab/deep-robotics-mimic#2-data-pipeline)
   to prepare the tracking NPZ. Skip this preparation if the motion is already available.
2. **Train and export.** Follow the training and export instructions in
   [deep-robotics-mimic](https://github.com/DeepRoboticsLab/deep-robotics-mimic)
   using that motion, and retain both the reference `.npz` and exported `.onnx`.
3. **Add the pair to the SDK.** Put the new ONNX in
   [state_machine/policy/](state_machine/policy/) and the related motion NPZ in
   [state_machine/motion_data/](state_machine/motion_data/).
4. **Select the pair and rebuild.** In
   [mimic_policy_runner.hpp](state_machine/run_policy/mimic_policy_runner.hpp),
   change `policy_path` and `motion_path` from `fist_routine.onnx` and
   `fist_routine.npz` to the new filenames. The motion and policy from this
   workflow already match the runner; no observation or motion-format changes
   are needed in the SDK. Rebuild `state_machine` because these paths are in C++.
5. **Validate and deploy.** Follow the simulation and hardware steps in
   [the AMP workflow](#52-deploy-an-amp-policy), transferring both new files and
   the updated runner. Use [State Machine](docs/STATE_MACHINE.md) for Mimic
   entry with `v`, return to AMP with `c`, and automatic return when the motion
   finishes. Keep the AMP model available for that return.

### 5.4 Other Tasks and Reading Order

For physical-robot tasks below, start with [Real-Robot Deployment](docs/REAL_ROBOT.md).
Use the mode required by each example in [Developer Modes](docs/DEVELOPER_MODE.md).

| Goal | Reading order and procedure |
| --- | --- |
| Test or implement whole-body joint commands | [Joint Control](docs/JOINT_CONTROL.md) → [Low-Level Examples](docs/EXAMPLES.md#1-low-level-examples). Inspect feedback with `joints_example`, then test commands in `whole` simulation before Whole-Body Joint Control Mode on hardware. |
| Implement arm/waist trajectories | [Joint Control](docs/JOINT_CONTROL.md) → [Low-Level Examples](docs/EXAMPLES.md#1-low-level-examples) → `arm_joint_example` / `arm_action_example`. Modify the example trajectories, test in `upper` simulation, then use Upper-Body Joint Control Mode on hardware. |
| Use built-in walking, gait selection, or actions | [Developer Modes](docs/DEVELOPER_MODE.md) → [High-Level Examples](docs/EXAMPLES.md#2-high-level-examples). Enter High-Level Motion Control Mode, follow `Idle -> SuspendedStand -> RLControl`, then use motion/gait/action/steering examples and their feedback. Selecting `HumanWALKAMP` invokes the robot's built-in gait; it does not load your ONNX policy. |
| Switch Developer Mode through an application | [Developer Modes](docs/DEVELOPER_MODE.md#sdk-example-switching) → [developer_mode_example.cpp](high_level/developer_mode_example.cpp). Read the service request, confirmation, status checks, and exit procedure before integrating them. |
| Inspect battery, faults, IMU, or gamepad events | [Monitoring Examples](docs/EXAMPLES.md#3-monitoring-examples) and [Peripheral Examples](docs/EXAMPLES.md#4-peripheral-examples). Run the relevant subscriber; these examples work in any Developer Mode and do not require the state machine. |
| Play audio, set volume, or record | [Audio Examples](docs/EXAMPLES.md#5-audio-examples). Transfer playback files to the robot's audio directory, then use the playback, volume, or recording examples. |
| Acquire RealSense color/depth data | [Camera Guide](docs/CAMERA.md): prepare the drivers on the NOS, identify cameras, choose ROS Topics or direct C++/Python access, then verify streams. The NOS is offline, so provision driver dependencies separately if absent. Camera access does not require running a motion controller. |
| Change terrain, scene, or simulator timing | [Simulator product guide](https://github.com/DeepRoboticsLab/deep-robotics-simulation/blob/main/dr02_pro/README.md) → its `config.yaml` and `scene.xml`. Configure shared worlds in the simulation repo and repeat the SDK simulation validation; the SDK's `BUILD_SIM` flag does not select terrain. |

The documented simulation workflow supports `state_machine` and the three low-level
joint examples. High-level robot commands, mode-switching services, monitoring,
peripheral examples, and audio require their robot-side services for the documented
workflows; they are not replaced by starting MuJoCo.

## 6. Important Safety Information

> [!WARNING]
>
> - Real-robot programs may directly drive robot motion. Confirm that the robot state and surrounding motion area are safe before running them.
> - When running a motion-control program on the real robot for the first time, test it with a reliable safety suspension in place.
> - Before controlling the real robot, ensure that it has entered the Developer Mode required by the target program.
> - Only one `/JOINTS_CMD` publisher may run at a time.
> - If an abnormal condition occurs, use the red stop button on the gamepad to stop control immediately.

## 7. Build Options

| Option | Default | Description |
| --- | --- | --- |
| `BUILD_DR02_PRO_STATE_MACHINE` | `ON` | Build the state machine |
| `BUILD_DR02_PRO_LOW_LEVEL` | `ON` | Build low-level joint and upper-body control examples |
| `BUILD_DR02_PRO_HIGH_LEVEL` | `ON` | Build high-level motion, gait, velocity, and Developer Mode switching examples |
| `BUILD_DR02_PRO_MONITORING` | `ON` | Build monitoring examples |
| `BUILD_DR02_PRO_PERIPHERALS` | `ON` | Build peripheral examples |
| `BUILD_DR02_PRO_AUDIO` | `ON` | Build audio examples |
| `BUILD_SIM` | `OFF` | Enable simulation support; do not enable for real-robot control |
