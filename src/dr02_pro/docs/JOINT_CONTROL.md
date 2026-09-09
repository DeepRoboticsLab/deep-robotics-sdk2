# DR02 Pro Joint Control Interface

[Back to the DR02 Pro SDK Guide](../README.md)

This document describes the message fields, units, array order, and controllable ranges used by the DR02 Pro low-level joint examples.

## 1. Message Structure

`/JOINTS_DATA` publishes joint feedback with `Joints`, and `/JOINTS_CMD` publishes joint commands with `JointsCmd`. In both messages, `data[i]` identifies a joint by its array index `i`; `data_id` is not the joint index.

In the message header, `header.stamp` is the message generation time and `header.frame_id` is the publisher frame sequence number.

## 2. Feedback Message: `/JOINTS_DATA`

| Field | Meaning | Unit or description |
| --- | --- | --- |
| `name` | Joint name | Reserved field; SDK applications should not rely on it |
| `data_id` | Robot-side data identifier | Reserved field; not the array index |
| `status_word` | Driver diagnostic status | Reserved field; no public enumeration is defined |
| `position` | Joint position | `rad` |
| `torque` | Joint feedback torque | `N·m` |
| `velocity` | Joint angular velocity | `rad/s` |
| `motion_temp` | Motor temperature | `°C` |
| `driver_temp` | Driver temperature | `°C` |

## 3. Command Message: `/JOINTS_CMD`

| Field | Meaning | Unit or description |
| --- | --- | --- |
| `name` | Joint name | Reserved field; SDK applications should not rely on it |
| `data_id` | Robot-side command identifier | Reserved field; not the array index |
| `control_word` | Joint control command type | SDK joint control uses `4` |
| `position` | Target joint position | `rad` |
| `torque` | Feedforward torque | `N·m` |
| `velocity` | Target joint angular velocity | `rad/s` |
| `kp` | Position gain | `N·m/rad` |
| `kd` | Velocity gain | `N·m·s/rad` |

## 4. Joint Array Order

`/JOINTS_DATA` and `/JOINTS_CMD` use the following 31-joint order:

| Index range | Group | Array order |
| --- | --- | --- |
| `0-2` | Waist | `0 waist_z_joint`, `1 waist_x_joint`, `2 waist_y_joint` |
| `3-9` | Left arm | `3 left_shoulder_y_joint`, `4 left_shoulder_x_joint`, `5 left_shoulder_z_joint`, `6 left_elbow_joint`, `7 left_wrist_z_joint`, `8 left_wrist_y_joint`, `9 left_wrist_x_joint` |
| `10-16` | Right arm | `10 right_shoulder_y_joint`, `11 right_shoulder_x_joint`, `12 right_shoulder_z_joint`, `13 right_elbow_joint`, `14 right_wrist_z_joint`, `15 right_wrist_y_joint`, `16 right_wrist_x_joint` |
| `17-22` | Left leg | `17 left_hip_y_joint`, `18 left_hip_x_joint`, `19 left_hip_z_joint`, `20 left_knee_joint`, `21 left_ankle_y_joint`, `22 left_ankle_x_joint` |
| `23-28` | Right leg | `23 right_hip_y_joint`, `24 right_hip_x_joint`, `25 right_hip_z_joint`, `26 right_knee_joint`, `27 right_ankle_y_joint`, `28 right_ankle_x_joint` |
| `29-30` | Neck | `29 neck_z_joint`, `30 neck_y_joint` |

## 5. Control Ranges and Zero Position

- Whole-Body Joint Control Mode: controllable indices are `0-30`.
- Upper-Body Joint Control Mode: controllable indices are `0-16`, covering the waist and both arms.
- Neck joints `29-30` are currently locked and cannot be controlled.
- The `0` position in the examples is the product-defined and calibrated joint zero, not the position at program startup. The examples first read the current position and then interpolate smoothly to `0`.
