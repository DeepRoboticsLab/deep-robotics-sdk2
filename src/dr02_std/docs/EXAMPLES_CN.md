# DR02 Std Topic 示例

[返回 DR02 Std SDK 使用指南](../README_CN.md)

所有示例都是独立可执行程序。加载工作空间环境后，按以下格式运行：

```bash
source install/setup.bash
ros2 run dr02_std <example_name> [args]
```

控制实机前，请先完成[实机部署与控制](REAL_ROBOT_CN.md)，再参阅[开发者模式文档](DEVELOPER_MODE_CN.md)选择手柄或 SDK 示例切换方式，并根据下表进入对应的开发者模式。使用仿真验证前，请根据“仿真支持”列确认示例是否支持，并参阅[仿真环境与运行](SIMULATION_CN.md)。

> [!WARNING]
>
> 直接发布 `/JOINTS_CMD` 的示例不得与 `state_machine` 或其他 `/JOINTS_CMD` 发布者同时运行。
>
> 上半身关节控制示例必须在上半身关节控制模式下运行。若在全身关节控制模式下运行，腿部关节可能因未获得有效控制指令而失去支撑，导致机器人跌倒。

## 1. 低层示例

| 示例 | Topic / 接口 | 实机开发者模式 | 仿真支持 | 运行命令 | 说明 |
| --- | --- | --- | --- | --- | --- |
| `joints_example` | `/JOINTS_DATA` | 全身关节控制模式 | 支持 | `ros2 run dr02_std joints_example` | 打印一帧完整 DR02 Std 关节状态后退出。 |
| `joints_example` | `/JOINTS_CMD` | 全身关节控制模式 | 支持 | `ros2 run dr02_std joints_example --confirm` | 平滑控制 21 个关节回零并保持。 |
| `arm_joint_example` | `/JOINTS_DATA`, `/JOINTS_CMD` | 上半身关节控制模式 | 支持 | `ros2 run dr02_std arm_joint_example --confirm` | 打印 `/JOINTS_DATA` 接收延迟，并将腰部和双臂从当前位置移动到 0 位。 |
| `arm_action_example` | `/JOINTS_CMD` | 上半身关节控制模式 | 支持 | `ros2 run dr02_std arm_action_example <action_id> --confirm` | 直接发布 `/JOINTS_CMD` 控制腰部和上肢关节；可修改示例中的关节轨迹，自定义动作。 |

预设上肢动作 ID：

| ID | 名称 |
| --- | --- |
| `0` | `greeting` |
| `1` | `kiss` |
| `2` | `handshake` |
| `3` | `salute` |
| `4` | `salute2` |

### 1.1 关节接口

关节消息字段、单位、数组顺序、可控范围和零位说明参阅[关节控制接口](JOINT_CONTROL_CN.md)。

## 2. 高层示例

| 示例 | Topic / 接口 | 实机开发者模式 | 仿真支持 | 运行命令 | 说明 |
| --- | --- | --- | --- | --- | --- |
| `motion_info_example` | `/MOTION_INFO` | 高层运动控制模式 | 不支持 | `ros2 run dr02_std motion_info_example [--once]` | 查看 `/MOTION_STATE` 切换后的运动状态及当前步态。 |
| `motion_state_example` | `/MOTION_STATE` | 高层运动控制模式 | 不支持 | `ros2 run dr02_std motion_state_example <motion_state_value>` | 发布运动状态切换命令。初始状态为 `Idle`，正常切换顺序为 `Idle -> SuspendedStand -> RLControl`；危险情况下可切换至 `JointDamping`。 |
| `gait_example` | `/GAIT` | 高层运动控制模式 | 不支持 | `ros2 run dr02_std gait_example <gait_value>` | 仅在机器人进入 `RLControl` 状态后使用，用于发布步态切换命令。 |
| `action_example` | `/ACTION` | 高层运动控制模式 | 不支持 | `ros2 run dr02_std action_example <action_id> --confirm` | 执行机器人内置动作。仅在机器人进入 `RLControl` 状态后使用，机器人会自动进入 `Action` 运动状态；动作完成后仍处于该运动状态，需另行切回 `RLControl`。 |
| `action_info_example` | `/ACTION_INFO` | 高层运动控制模式 | 不支持 | `ros2 run dr02_std action_info_example` | 查看当前正在执行的内置动作；`0` 表示当前没有执行，非零值表示当前动作 ID。 |
| `steer_example` | `/STEER` | 高层运动控制模式 / 上半身关节控制模式 | 不支持 | `ros2 run dr02_std steer_example` | 仅在机器人进入 `RLControl` 状态后使用。键盘速度控制使用固定的保守归一化速度 `0.6`；`W/S` 前后，`A/D` 左右，`Q/E` 转向；松开按键自动停，Ctrl+C 退出。 |
| `steer_example` | `/REAL_STEER` | 高层运动控制模式 / 上半身关节控制模式 | 不支持 | `ros2 run dr02_std steer_example --real` | 仅在机器人进入 `RLControl` 状态后使用。默认发布 3 秒 `yaw=0.4` 的旋转指令；可根据需要在示例代码中修改 `x`、`y` 和 `yaw`。 |

> [!WARNING]
>
> - 实机切换至 `RLControl` 前，必须先将机器人放下并确认双脚稳定接触地面。吊绳可以保留用于保护，但不得使机器人处于悬空状态。悬空切换可能产生突然动作或失稳，造成人身伤害或设备损坏。
> - `action_example`、`gait_example` 和 `steer_example` 都会产生运动或动作控制，运行前应确认运动空间安全。
> - `/STEER` 的 `x`、`y` 和 `yaw` 是 `[-1.0, 1.0]` 的归一化比例，不是实际速度；示例固定使用 `0.6`，实际速度由机器人状态、步态和控制策略决定。
> - 请持续按住按键；松开后约 `320 ms` 归零。机器人可能调整姿态或产生倾斜，实机测试时应在空旷环境中一次只测试一个方向键。本示例仅用于 Topic 验证，不用于性能或最大速度测试。

### 2.1 `motion_state_example` 参数

| 数值 | 名称 | 说明 |
| --- | --- | --- |
| `0x0` | `Idle` | 零力矩，空闲状态。 |
| `0x2` | `JointDamping` | 关节阻尼软急停状态。 |
| `0x11` | `RLControl` | RL 控制状态。 |
| `0x20006` | `SuspendedStand` | 悬吊起立完成；将机器人放下并确认双脚接触地面后，方可进入 `RLControl`。 |

### 2.2 `gait_example` 参数

| 数值 | 名称 | 说明 |
| --- | --- | --- |
| `0x21001` | `HumanWALKAMP` | 仿人走路。 |
| `0x21006` | `HumanWALKTERRAIN` | 复杂地形走路。 |

### 2.3 `action_example` 参数

| ID | 名称 | 说明 |
| --- | --- | --- |
| `0x3000` | `greeting` | 庆祝动作。 |
| `0x3001` | `kiss` | 飞吻动作。 |
| `0x3002` | `handshake` | 握手动作。 |
| `0x3003` | `salute` | 敬礼动作。 |
| `0x3004` | `salute2` | 第二种敬礼动作。 |
| `0x3008` | `prepare_idle` | 返回准备姿态。 |

## 3. 状态监测示例

| 示例 | Topic / 接口 | 实机开发者模式 | 仿真支持 | 运行命令 | 说明 |
| --- | --- | --- | --- | --- | --- |
| `battery_state_example` | `/BATTERY_DATA` | 任意 | 不支持 | `ros2 run dr02_std battery_state_example` | 打印电量、电压、电流和保护状态。 |
| `fault_snapshot_example` | `/fault_aggregator` | 任意 | 不支持 | `ros2 run dr02_std fault_snapshot_example` | 订阅 `drdds/msg/FaultEventArray` 并打印当前故障快照。 |

### 3.1 当前故障快照

`/fault_aggregator` 采用状态变化触发和周期两种上报方式：故障出现时立即发布一次当前活动故障快照，故障状态发生其他变化时也会触发发布，同时按周期持续发布。重复收到相同内容不表示故障重复发生；`active_faults` 为空表示当前没有活动故障。

示例仅显示便于用户判断故障的关键信息：

| 输出 | 含义 |
| --- | --- |
| 时间 | 故障发生或最近一次更新的本地时间；时间无效时不显示。 |
| 严重级别 | 一类故障中最高的严重级别。 |
| 故障码 | 产品定义的故障标识，程序判断应使用该值。 |
| 故障名称 | 便于用户阅读的故障说明。 |

严重级别从低到高依次为 `DEBUG(0)`、`INFO(1)`、`NOTICE(2)`、`WARN(3)`、`ERROR(4)`、`CRITICAL(5)`、`ALERT(6)` 和 `EMERG(7)`，具体保护和处置方式由产品定义。示例会跳过内容未变化的周期快照，避免重复输出；消息中的其他字段用于内部故障诊断，不在示例中展示。

## 4. 外设示例

| 示例 | Topic / 接口 | 实机开发者模式 | 仿真支持 | 运行命令 | 说明 |
| --- | --- | --- | --- | --- | --- |
| `imu_example` | `/IMU_DATA_BASE` | 任意 | 不支持 | `ros2 run dr02_std imu_example` | 订阅 `sensor_msgs/msg/Imu`，打印基座 IMU 的四元数、角速度和线加速度。 |
| `gamepad_key_example` | `/GAMEPAD_KEY` | 任意 | 不支持 | `ros2 run dr02_std gamepad_key_example` | 打印手柄按键事件。 |

## 5. 音频示例

音频示例播放机器人端音频目录中已经存在的 WAV 文件。先将文件拷贝到 `/var/opt/robot/data/audio/`，再通过 `/AUDIO/PLAY_FILE` 发布文件名触发播放。

```bash
scp test.wav user@10.21.33.103:/var/opt/robot/data/audio/
ros2 run dr02_std audio_play_example test.wav
```

| 示例 | Topic / 接口 | 实机开发者模式 | 仿真支持 | 运行命令 | 说明 |
| --- | --- | --- | --- | --- | --- |
| `audio_play_example` | `/AUDIO/PLAY_FILE` | 任意 | 不支持 | `ros2 run dr02_std audio_play_example test.wav` | 发布远程文件名触发播放。 |
| `audio_volume_example` | `/AUDIO/VOLUME_CMD` | 任意 | 不支持 | `ros2 run dr02_std audio_volume_example <0-100>` | 发布目标音量百分比。 |
| `audio_record_example` | `/AUDIO/RECORD_CMD`, `/AUDIO/RECORD_STATUS` | 任意 | 不支持 | `ros2 run dr02_std audio_record_example` | 交互式录音控制。按 `1` 开始，按 `0` 停止，按 `q` 停止并退出。录音文件保存在 `/var/opt/robot/data/audio/`；audio_node 默认强制最大录音时长 300 秒。 |
