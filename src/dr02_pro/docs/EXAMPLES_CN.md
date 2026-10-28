# DR02 Pro Topic 示例

[返回 DR02 Pro SDK 使用指南](../README_CN.md)

所有示例都是独立可执行程序。加载工作空间环境后，按以下格式运行：

```bash
source install/setup.bash
ros2 run dr02_pro <example_name> [args]
```

控制实机前，请先完成[实机部署与控制](REAL_ROBOT_CN.md)，再参阅[开发者模式文档](DEVELOPER_MODE_CN.md)选择手柄或 SDK 示例切换方式，并根据下表选择对应的开发模式。使用仿真验证前，请根据“仿真支持”列确认示例是否支持，并参阅[仿真环境与运行](SIMULATION_CN.md)。

> [!WARNING]
>
> 直接发布 `/JOINTS_CMD` 的示例不得与 `state_machine` 或其他 `/JOINTS_CMD` 发布者同时运行。
>
> 上半身关节控制示例必须在上肢开发模式下运行。若在全身开发模式下运行，腿部关节可能因未获得有效控制指令而失去支撑，导致机器人跌倒。

## 1. 低层示例

| 示例 | Topic / 接口 | 实机开发模式 | 仿真支持 | 运行命令 | 说明 |
| --- | --- | --- | --- | --- | --- |
| `joints_example` | `/JOINTS_DATA` | 全身开发模式 | 支持 | `ros2 run dr02_pro joints_example` | 打印一帧完整 DR02 Pro 关节状态后退出。 |
| `joints_example` | `/JOINTS_CMD` | 全身开发模式 | 支持 | `ros2 run dr02_pro joints_example --confirm` | 平滑控制 31 个关节回零并保持。 |
| `arm_joint_example` | `/JOINTS_DATA`, `/JOINTS_CMD` | 上肢开发模式 | 支持 | `ros2 run dr02_pro arm_joint_example --confirm` | 打印 `/JOINTS_DATA` 接收延迟，并将腰部和双臂从当前位置移动到 0 位。 |
| `arm_action_example` | `/JOINTS_CMD` | 上肢开发模式 | 支持 | `ros2 run dr02_pro arm_action_example <action_id> --confirm` | 直接发布 `/JOINTS_CMD` 控制腰部和上肢关节；可修改示例中的关节轨迹，自定义动作。 |

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

| 示例 | Topic / 接口 | 实机开发模式 | 仿真支持 | 运行命令 | 说明 |
| --- | --- | --- | --- | --- | --- |
| `motion_info_example` | `/MOTION_INFO` | 高层开发模式 | 不支持 | `ros2 run dr02_pro motion_info_example [--once]` | 查看 `/MOTION_STATE` 切换后的运动状态及当前步态。 |
| `motion_state_example` | `/MOTION_STATE` | 高层开发模式 | 不支持 | `ros2 run dr02_pro motion_state_example <motion_state_value>` | 发布运动状态切换命令。初始状态为 `Idle`，正常切换顺序为 `Idle -> SuspendedStand -> RLControl`；危险情况下可切换至 `JointDamping`。 |
| `gait_example` | `/GAIT` | 高层开发模式 | 不支持 | `ros2 run dr02_pro gait_example <gait_value>` | 仅在机器人进入 `RLControl` 状态后使用，用于发布步态切换命令。 |
| `action_example` | `/ACTION_COMMAND` | 高层开发模式 | 不支持 | `ros2 run dr02_pro action_example <action_id> --confirm` | 执行机器人内置动作。仅在机器人进入 `RLControl` 状态后使用。 |
| `action_status_example` | `/ACTION_STATUS` | 高层开发模式 | 不支持 | `ros2 run dr02_pro action_status_example` | 查看当前正在执行的动作 ID、执行阶段和错误码。 |
| `steer_example` | `/STEER` | 高层开发模式 / 上肢开发模式 | 不支持 | `ros2 run dr02_pro steer_example` | 仅在机器人进入 `RLControl` 状态或者上肢开发模式使用。键盘速度控制使用固定的保守归一化速度 `0.6`；`W/S` 前后，`A/D` 左右，`Q/E` 转向；松开按键自动停，Ctrl+C 退出。 |
| `steer_example` | `/REAL_STEER` | 高层开发模式 / 上肢开发模式 | 不支持 | `ros2 run dr02_pro steer_example --real` | 仅在机器人进入 `RLControl` 状态或者上肢开发模式使用。默认发布 3 秒 `yaw=0.4` 的旋转指令；可根据需要在示例代码中修改 `x`、`y` 和 `yaw`。 |

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
| `30` | `arm_wave_small` | 小幅招手。 |
| `31` | `arm_heart` | 比心。 |
| `32` | `arm_clap` | 鼓掌。 |
| `33` | `arm_salute` | 敬礼。 |
| `34` | `waist_patrol` | 腰部转动巡视。 |
| `35` | `neck_patrol` | 头部左右环视。 |
| `36` | `neck_shake` | 摇头。 |
| `37` | `neck_nod` | 点头。 |

### 2.4 `action_status_example` 输出

| 执行阶段 | 含义 |
| --- | --- |
| `IDLE(0)` | 空闲。 |
| `PLAYING(1)` | 执行中。 |
| `ERROR(2)` | 执行失败。 |
| `DONE(3)` | 执行完成，不代表已回到默认步态。 |

| 错误码 | 含义 |
| --- | --- |
| `OK(0)` | 正常。 |
| `BUSY(1)` | 已有动作正在执行。 |
| `INVALID_COMMAND(2)` | 非法或不支持的命令。 |
| `OUTPUT_FAILURE(3)` | 执行、通信或反馈异常，或执行超时。 |
| `UNSUPPORTED_MODE(4)` | 当前运动状态或步态不支持。 |
| `IMU_NOT_READY(5)` | IMU 数据未就绪或超时。 |
| `FEEDBACK_NOT_READY(6)` | 动作所需反馈未就绪或超时。 |

## 3. 状态监测示例

| 示例 | Topic / 接口 | 实机开发模式 | 仿真支持 | 运行命令 | 说明 |
| --- | --- | --- | --- | --- | --- |
| `battery_state_example` | `/BATTERY_DATA` | 任意 | 不支持 | `ros2 run dr02_pro battery_state_example` | 打印电量、电压、电流和保护状态。 |
| `fault_snapshot_example` | `/fault_aggregator` | 任意 | 不支持 | `ros2 run dr02_pro fault_snapshot_example` | 订阅 `drdds/msg/FaultEventArray` 并打印当前故障快照。 |

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

| 示例 | Topic / 接口 | 实机开发模式 | 仿真支持 | 运行命令 | 说明 |
| --- | --- | --- | --- | --- | --- |
| `imu_example` | `/IMU_DATA_HEAD`, `/IMU_DATA_BASE` | 任意 | 不支持 | `ros2 run dr02_pro imu_example` | 订阅 `sensor_msgs/msg/Imu`，打印头部和基座 IMU 的四元数、角速度和线加速度。 |
| `gamepad_key_example` | `/GAMEPAD_KEY` | 任意 | 不支持 | `ros2 run dr02_pro gamepad_key_example` | 打印手柄按键事件。 |

## 5. 音频示例

音频示例播放机器人端音频目录中已经存在的 WAV 文件。先将文件拷贝到 `/var/opt/robot/data/audio/`，再通过 `/AUDIO/PLAY_FILE` 发布文件名触发播放。

```bash
scp test.wav user@10.21.33.103:/var/opt/robot/data/audio/
ros2 run dr02_pro audio_play_example test.wav
```

| 示例 | Topic / 接口 | 实机开发模式 | 仿真支持 | 运行命令 | 说明 |
| --- | --- | --- | --- | --- | --- |
| `audio_play_example` | `/AUDIO/PLAY_FILE` | 任意 | 不支持 | `ros2 run dr02_pro audio_play_example test.wav` | 发布远程文件名触发播放。 |
| `audio_volume_example` | `/AUDIO/VOLUME_CMD` | 任意 | 不支持 | `ros2 run dr02_pro audio_volume_example <0-100>` | 发布目标音量百分比。 |
