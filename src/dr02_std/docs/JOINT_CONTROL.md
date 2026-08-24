# DR02 Std Joint Control Interface

[Back to the DR02 Std SDK Guide](../README.md)

This document describes the message fields, units, array order, and controllable ranges used by the DR02 Std low-level joint examples.

## Message Structure

`/JOINTS_DATA` publishes joint feedback with `Joints`, and `/JOINTS_CMD` publishes joint commands with `JointsCmd`. In both messages, `data[i]` identifies a joint by its array index `i`; `data_id` is not the joint index.

In the message header, `header.stamp` is the message generation time and `header.frame_id` is the publisher frame sequence number.

## Feedback Message: `/JOINTS_DATA`

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

## Command Message: `/JOINTS_CMD`

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

## Joint Array Order

`/JOINTS_DATA` and `/JOINTS_CMD` use the following 21-joint order:

| Index range | Group | Array order |
| --- | --- | --- |
| `0` | Waist | `0 waist_z_joint` |
| `1-4` | Left arm | `1 left_shoulder_y_joint`, `2 left_shoulder_x_joint`, `3 left_shoulder_z_joint`, `4 left_elbow_joint` |
| `5-8` | Right arm | `5 right_shoulder_y_joint`, `6 right_shoulder_x_joint`, `7 right_shoulder_z_joint`, `8 right_elbow_joint` |
| `9-14` | Left leg | `9 left_hip_y_joint`, `10 left_hip_x_joint`, `11 left_hip_z_joint`, `12 left_knee_joint`, `13 left_ankle_y_joint`, `14 left_ankle_x_joint` |
| `15-20` | Right leg | `15 right_hip_y_joint`, `16 right_hip_x_joint`, `17 right_hip_z_joint`, `18 right_knee_joint`, `19 right_ankle_y_joint`, `20 right_ankle_x_joint` |

## Control Ranges and Zero Position

- Whole-Body Joint Control Mode: controllable indices are `0-20`.
- Upper-Body Joint Control Mode: controllable indices are `0-8`, covering the waist and both arms.
- DR02 Std has no neck joints.
- The `0` position in the examples is the product-defined and calibrated joint zero, not the position at program startup. The examples first read the current position and then interpolate smoothly to `0`.
