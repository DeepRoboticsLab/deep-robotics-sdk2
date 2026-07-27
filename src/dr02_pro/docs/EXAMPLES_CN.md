# DR02 Pro Topic 示例

[返回 DR02 Pro SDK 使用指南](../README_CN.md)

所有示例都是独立可执行程序。加载工作空间环境后，按以下格式运行：

```bash
source install/setup.bash
ros2 run dr02_pro <example_name> [args]
```

控制实机前，请先完成[实机部署与控制](REAL_ROBOT_CN.md)，并根据下表进入对应的开发者模式。使用仿真验证前，请根据“仿真支持”列确认示例是否支持，并参阅[仿真环境与运行](SIMULATION_CN.md)。

> [!WARNING]
>
> 直接发布 `/JOINTS_CMD` 的示例不得与 `state_machine` 或其他 `/JOINTS_CMD` 发布者同时运行。
>
> 上半身关节控制示例必须在上半身关节控制模式下运行。若在全身关节控制模式下运行，腿部关节可能因未获得有效控制指令而失去支撑，导致机器人跌倒。

## 低层示例

| 示例 | Topic / 接口 | 实机开发者模式 | 仿真支持 | 运行命令 | 说明 |
| --- | --- | --- | --- | --- | --- |
| `joints_example` | `/JOINTS_DATA` | 全身关节控制模式 | 支持 | `ros2 run dr02_pro joints_example` | 打印一帧完整 DR02 Pro 关节状态后退出。 |
| `joints_example` | `/JOINTS_CMD` | 全身关节控制模式 | 支持 | `ros2 run dr02_pro joints_example --confirm` | 平滑控制 31 个关节回零并保持。 |
| `arm_joint_example` | `/JOINTS_DATA`, `/JOINTS_CMD` | 上半身关节控制模式 | 支持 | `ros2 run dr02_pro arm_joint_example --confirm` | 打印 `/JOINTS_DATA` 接收延迟，并将腰部和双臂从当前位置移动到 0 位。 |
| `arm_action_example` | `/JOINTS_CMD` | 上半身关节控制模式 | 支持 | `ros2 run dr02_pro arm_action_example <action_id> --confirm` | 执行预设腰部和上肢动作。 |

预设上肢动作 ID：

| ID | 名称 |
| --- | --- |
| `0` | `greeting` |
| `1` | `kiss` |
| `2` | `handshake` |
| `3` | `salute` |
| `4` | `salute2` |

## 高层示例

| 示例 | Topic / 接口 | 实机开发者模式 | 仿真支持 | 运行命令 | 说明 |
| --- | --- | --- | --- | --- | --- |
| `motion_info_example` | `/MOTION_INFO` | 高层运动控制模式 | 不支持 | `ros2 run dr02_pro motion_info_example [--once]` | 打印当前运动状态和步态。 |
| `motion_state_example` | `/MOTION_STATE` | 高层运动控制模式 | 不支持 | `ros2 run dr02_pro motion_state_example <motion_state_value>` | 发布运动状态切换命令。初始状态为 `Idle`，正常切换顺序为 `Idle -> SuspendedStand -> RLControl`；危险情况下可切换至 `JointDamping`。 |
| `gait_example` | `/GAIT` | 高层运动控制模式 | 不支持 | `ros2 run dr02_pro gait_example <gait_value>` | 仅在机器人进入 `RLControl` 状态后使用，用于发布步态切换命令。 |
| `steer_example` | `/STEER` | 高层运动控制模式 | 不支持 | `ros2 run dr02_pro steer_example` | 仅在机器人进入 `RLControl` 状态后使用。键盘速度控制使用固定的保守归一化速度 `0.6`；`W/S` 前后，`A/D` 左右，`Q/E` 转向；松开按键自动停，Ctrl+C 退出。 |
| `steer_example` | `/REAL_STEER` | 高层运动控制模式 | 不支持 | `ros2 run dr02_pro steer_example --real` | 仅在机器人进入 `RLControl` 状态后使用。默认发布 3 秒 `yaw=0.4` 的旋转指令；可根据需要在示例代码中修改 `x`、`y` 和 `yaw`。 |

> [!WARNING]
>
> - `/STEER` 的 `x`、`y` 和 `yaw` 是范围为 `[-1.0, 1.0]` 的归一化控制比例，不是以 m/s 或 rad/s 表示的实际速度。`steer_example` 固定发布 `0.6`，实际运动速度由机器人状态、步态和控制策略决定。
> - 应持续按住按键以形成连续指令；松开后对应方向将在约 320 ms 内归零。机器人可能先调整姿态或产生倾斜，部分步态、地面或负载条件下仍可能无法形成连续移动。
> - 实机测试时，应确认机器人处于 `RLControl` 状态，并在空旷环境中一次只测试一个方向键。本示例仅用于展示 Topic 用法，不用于性能或最大速度测试，请勿自行增大固定值。

支持的运动状态值：

| 数值 | 名称 | 说明 |
| --- | --- | --- |
| `0x0` | `Idle` | 零力矩，空闲状态。 |
| `0x2` | `JointDamping` | 关节阻尼软急停状态。 |
| `0x11` | `RLControl` | RL 控制状态。 |
| `0x20006` | `SuspendedStand` | 悬吊起立完成，可进入 RL 控制模式。 |

支持的步态值：

| 数值 | 名称 | 说明 |
| --- | --- | --- |
| `0x21001` | `HumanWALKAMP` | 仿人走路。 |
| `0x21006` | `HumanWALKTERRAIN` | 复杂地形走路。 |

## 外设示例

| 示例 | Topic / 接口 | 实机开发者模式 | 仿真支持 | 运行命令 | 说明 |
| --- | --- | --- | --- | --- | --- |
| `battery_state_example` | `/BATTERY_DATA` | 任意 | 不支持 | `ros2 run dr02_pro battery_state_example` | 打印电量、电压、电流和保护状态。 |
| `imu_example` | `/IMU_DATA_HEAD`, `/IMU_DATA_BASE` | 任意 | 不支持 | `ros2 run dr02_pro imu_example` | 订阅 `sensor_msgs/msg/Imu`，打印头部和基座 IMU 的四元数、角速度和线加速度。 |
| `gamepad_key_example` | `/GAMEPAD_KEY` | 任意 | 不支持 | `ros2 run dr02_pro gamepad_key_example` | 打印手柄按键事件。 |

## 音频示例

音频示例播放机器人端音频目录中已经存在的 WAV 文件。先将文件拷贝到 `/var/opt/robot/data/audio/`，再通过 `/AUDIO/PLAY_FILE` 发布文件名触发播放。

```bash
scp test.wav user@10.21.33.103:/var/opt/robot/data/audio/
ros2 run dr02_pro audio_play_example test.wav
```

| 示例 | Topic / 接口 | 实机开发者模式 | 仿真支持 | 运行命令 | 说明 |
| --- | --- | --- | --- | --- | --- |
| `audio_play_example` | `/AUDIO/PLAY_FILE` | 任意 | 不支持 | `ros2 run dr02_pro audio_play_example test.wav` | 发布远程文件名触发播放。 |
| `audio_volume_example` | `/AUDIO/VOLUME_CMD` | 任意 | 不支持 | `ros2 run dr02_pro audio_volume_example <0-100>` | 发布目标音量百分比。 |
| `audio_record_example` | `/AUDIO/RECORD_CMD`, `/AUDIO/RECORD_STATUS` | 任意 | 不支持 | `ros2 run dr02_pro audio_record_example` | 交互式录音控制。按 `1` 开始，按 `0` 停止，按 `q` 停止并退出。录音文件保存在 `/var/opt/robot/data/audio/`；audio_node 默认强制最大录音时长 300 秒。 |
