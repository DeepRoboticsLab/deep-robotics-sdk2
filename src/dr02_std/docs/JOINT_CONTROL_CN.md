# DR02 Std 关节控制接口

[返回 DR02 Std SDK 使用指南](../README_CN.md)

本文说明 DR02 Std 低层关节示例使用的消息字段、单位、数组顺序和控制范围。

## 消息结构

`/JOINTS_DATA` 使用 `Joints` 发布关节反馈，`/JOINTS_CMD` 使用 `JointsCmd` 发布关节命令。两条消息中的 `data[i]` 由数组索引 `i` 确定对应关节，`data_id` 不是关节编号。

消息头中的 `header.stamp` 表示消息生成时间，`header.frame_id` 表示发布端的帧序号。

## 反馈消息 `/JOINTS_DATA`

| 字段 | 含义 | 单位或说明 |
| --- | --- | --- |
| `name` | 关节名称 | 保留字段，SDK 不应依赖 |
| `data_id` | 机器人侧数据标识 | 保留字段，不是数组索引 |
| `status_word` | 驱动器诊断状态 | 保留字段，没有公开枚举，SDK 不应依赖 |
| `position` | 关节位置 | `rad` |
| `torque` | 关节反馈力矩 | `N·m` |
| `velocity` | 关节角速度 | `rad/s` |
| `motion_temp` | 电机温度 | `°C` |
| `driver_temp` | 驱动器温度 | `°C` |

## 控制消息 `/JOINTS_CMD`

| 字段 | 含义 | 单位或说明 |
| --- | --- | --- |
| `name` | 关节名称 | 保留字段，SDK 不应依赖 |
| `data_id` | 机器人侧命令标识 | 保留字段，不是数组索引 |
| `control_word` | 关节控制命令类型 | SDK 关节控制使用 `4` |
| `position` | 目标关节位置 | `rad` |
| `torque` | 前馈力矩 | `N·m` |
| `velocity` | 目标关节角速度 | `rad/s` |
| `kp` | 位置增益 | `N·m/rad` |
| `kd` | 速度增益 | `N·m·s/rad` |

## 关节数组顺序

`/JOINTS_DATA` 和 `/JOINTS_CMD` 使用以下 21 个关节顺序：

| 索引范围 | 分组 | 数组顺序 |
| --- | --- | --- |
| `0` | 腰部 | `0 waist_z_joint` |
| `1-4` | 左臂 | `1 left_shoulder_y_joint`、`2 left_shoulder_x_joint`、`3 left_shoulder_z_joint`、`4 left_elbow_joint` |
| `5-8` | 右臂 | `5 right_shoulder_y_joint`、`6 right_shoulder_x_joint`、`7 right_shoulder_z_joint`、`8 right_elbow_joint` |
| `9-14` | 左腿 | `9 left_hip_y_joint`、`10 left_hip_x_joint`、`11 left_hip_z_joint`、`12 left_knee_joint`、`13 left_ankle_y_joint`、`14 left_ankle_x_joint` |
| `15-20` | 右腿 | `15 right_hip_y_joint`、`16 right_hip_x_joint`、`17 right_hip_z_joint`、`18 right_knee_joint`、`19 right_ankle_y_joint`、`20 right_ankle_x_joint` |

## 控制范围与零位

- 全身关节控制模式：可控索引为 `0-20`。
- 上半身关节控制模式：可控索引为 `0-8`，包括腰部和双臂。
- DR02 Std 不包含颈部关节。
- 示例中的 `0` 位是产品关节配置和标定定义的零位，不是程序启动时的当前位置。示例会先读取当前位置，再平滑插值到 `0` 位。

