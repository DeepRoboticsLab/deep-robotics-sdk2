# DR02 Std SDK 使用指南

本文档是 DR02 Std 产品软件包的中文总览。仓库级介绍和产品导航请参阅 [DEEPRobotics SDK 根 README](../../README_CN.md)。

该 ROS 2 软件包提供 DR02 Std 运动控制状态机和独立 Topic 示例，可在开发主机或机器人端设备运行，并支持部分程序配合 MuJoCo 仿真验证。

AOS（`10.21.33.103`）使用 Ubuntu 24.04 / ROS 2 Jazzy，架构为 ARM64，无互联网连接。 离线消息安装和代码传输步骤见[实机部署文档](docs/REAL_ROBOT_CN.md)。

## 1. 功能概览

- DR02 Std RL 状态机控制
- 低层关节状态、关节命令和上肢控制示例
- 高层运动状态、步态、速度控制和开发者模式切换示例
- 电池状态和当前故障快照监测示例
- IMU 和手柄按键外设 Topic 示例
- WAV 文件播放、音量控制和录音示例

## 2. 目录结构

- `state_machine/`：DR02 Std 主状态机程序
- `low_level/`：低层关节和上肢控制 Topic 示例
- `high_level/`：高层运动状态、步态、速度控制和开发者模式切换示例
- `monitoring/`：电池状态和当前故障快照监测示例
- `peripherals/`：IMU 和手柄按键外设示例
- `audio/`：WAV 播放、音量和录音示例
- `docs/`：实机部署、开发者模式、仿真运行、状态机和示例说明

## 3. 文档导航

| 文档 | 内容 |
| --- | --- |
| [实机部署与控制](docs/REAL_ROBOT_CN.md) | 开发主机和 AOS 主机的实机部署与编译方式 |
| [开发者模式](docs/DEVELOPER_MODE_CN.md) | 开发者模式介绍、手柄切换和 SDK 示例切换流程 |
| [仿真环境与运行](docs/SIMULATION_CN.md) | SDK 仿真编译、运行方式和支持范围 |
| [状态机](docs/STATE_MACHINE_CN.md) | 状态流转、运行命令、键盘控制、手柄控制和安全要求 |
| [Topic 示例](docs/EXAMPLES_CN.md) | 全部示例的 Topic、实机开发者模式、仿真支持、运行命令和注意事项 |
| [关节控制接口](docs/JOINT_CONTROL_CN.md) | 关节消息字段、数组顺序、可控范围、单位和零位定义 |

## 4. 快速开始

首次使用时，根据控制目标选择环境准备文档，再按下一步链接阅读对应的模式和示例说明。

| 控制目标 | 环境准备 | 下一步 |
| --- | --- | --- |
| 实机 | [实机部署与控制](docs/REAL_ROBOT_CN.md) | 阅读[开发者模式](docs/DEVELOPER_MODE_CN.md)，再选择[状态机](docs/STATE_MACHINE_CN.md)或[Topic 示例](docs/EXAMPLES_CN.md) |
| 仿真 | [仿真环境与运行](docs/SIMULATION_CN.md) | 根据仿真支持范围选择[状态机](docs/STATE_MACHINE_CN.md)或[Topic 示例](docs/EXAMPLES_CN.md) |

> [!IMPORTANT]
>
> SDK 的运行位置与控制目标是两个独立概念。只要 ROS/DDS 网络互通，在开发主机或 AOS 主机（`10.21.33.103`）上运行的 SDK 均可能直接控制实机。

## 5. 按任务阅读与操作流程

### 5.1 相关仓库如何配合

策略工作流涉及以下仓库：

| 仓库 | 职责 |
| --- | --- |
| [rl_training](https://github.com/DeepRoboticsLab/rl_training) | 训练 AMP 策略并导出 `policy.onnx`。 |
| [deep-robotics-retarget](https://github.com/DeepRoboticsLab/deep-robotics-retarget) | 将源动作重定向到机器人，用于 Mimic 动作跟踪。 |
| [deep-robotics-mimic](https://github.com/DeepRoboticsLab/deep-robotics-mimic) | 准备参考动作，训练跟踪该动作的 Mimic 策略并导出 ONNX。 |
| 本 SDK | 加载导出策略、执行推理并发送关节命令。 |
| [deep-robotics-simulation](https://github.com/DeepRoboticsLab/deep-robotics-simulation) | 运行 MuJoCo 机器人与场景，接收 `/JOINTS_CMD`，返回关节和 IMU 反馈，用于 SDK 验证。 |
| [deep-robotics-msg](https://github.com/DeepRoboticsLab/deep-robotics-msg.git) | 提供共用的 `drdds` 消息接口。 |

AMP：**训练 → 导出 ONNX → 替换 SDK 模型 → 仿真验证 → 实机部署**。
Mimic：**动作重定向 → 准备跟踪动作并训练 → 导出 ONNX → SDK 加载动作与模型 → 仿真验证 → 实机部署**。

本 SDK 的 Mimic 部署由 [Pro 软件包](../dr02_pro/README_CN.md#53-部署-mimic-策略)提供，Std 状态机当前提供 AMP 控制。

仿真时，需要同时阅读 [SDK 仿真文档](docs/SIMULATION_CN.md)和
[仿真产品文档](https://github.com/DeepRoboticsLab/deep-robotics-simulation/blob/main/dr02_std/README_CN.md)。两个进程在不同终端运行，使用相同的 ROS
发行版、消息版本和 `ROS_DOMAIN_ID`。状态机或全身关节示例使用仿真的 `control.mode: whole`，
手臂示例使用 `upper`。仿真 `upper` 模式约束基座和腿部；实机上半身关节控制模式则由
机器人内部控制器控制腿部。仿真配置不会切换实机的开发者模式。

### 5.2 部署 AMP 策略

实机部署前，先按顺序阅读 **[实机部署](docs/REAL_ROBOT_CN.md) →
[开发者模式](docs/DEVELOPER_MODE_CN.md) → [状态机](docs/STATE_MACHINE_CN.md)**。
三篇文档依次说明代码在哪台主机部署和编译、实机应进入哪种控制模式，以及策略如何启动、
切换和停止。更换模型是策略准备步骤，之后仍需完成仿真验证和上述实机操作流程。

部署前，先完成[消息安装](../../README_CN.md#4-安装消息接口)，
并阅读[关节控制接口](docs/JOINT_CONTROL_CN.md)了解机器人的命令接口。

1. **训练并导出 AMP。** 按 [rl_training](https://github.com/DeepRoboticsLab/rl_training)
   中的 DR02 AMP 训练和 ONNX 导出说明操作。已有导出的 `policy.onnx` 时，从下一步开始。
2. **替换模型文件。** 将本产品的
   [state_machine/policy/policy.onnx](state_machine/policy/policy.onnx)
   替换为新导出的 `policy.onnx`，保持文件名不变。该训练流程的观测、动作及其他部署设置
   已与 SDK 匹配，无需修改 runner 或观测代码。SDK 已编译时，仅替换 ONNX 后重启
   `state_machine` 即可，无需重新编译 C++。
3. **准备仿真器。** 按仿真仓库的
   [环境配置](https://github.com/DeepRoboticsLab/deep-robotics-simulation/blob/main/README_CN.md#3-环境配置)
   和[产品配置](https://github.com/DeepRoboticsLab/deep-robotics-simulation/blob/main/dr02_std/README_CN.md)准备环境，选择 `control.mode: whole` 及带碰撞地面的场景。
   在仿真仓库运行 `python3 run_sim.py dr02_std`。
4. **编译并运行 SDK 仿真。** 按[仿真文档](docs/SIMULATION_CN.md)使用 `BUILD_SIM=ON` 编译，
   再按[状态机文档](docs/STATE_MACHINE_CN.md)运行。当前 AMP 路径为
   `Idle -> ZeroPos -> RLControl`：按 `z` 请求零位，等待姿态稳定后按 `c` 请求 AMP。
   上机前先验证站立、方向控制和阻尼退出；同时只能有一个 `/JOINTS_CMD` 发布者。
5. **传输并在目标主机重新编译。** 按[实机部署](docs/REAL_ROBOT_CN.md)选择运行主机。
   离线 AOS 需传入消息源码，用安装脚本的 `--source-dir` 安装。传输 SDK 源码和策略资源，
   在目标 ARM 主机进行全新原生编译，设置 `BUILD_PLATFORM=arm`、`BUILD_SIM=OFF`。
   策略路径相对于编译时记录的源码位置解析，需保留目标主机上的源码和资源，不能只复制可执行文件。
6. **切换实机模式并运行。** 按[开发者模式](docs/DEVELOPER_MODE_CN.md)进入**全身关节控制模式**，
   再按[状态机](docs/STATE_MACHINE_CN.md)及其安全要求操作。进入策略控制前确认双脚接地，
   按文档执行停止流程。修改策略或控制器后，先重新仿真验证，再重复上机流程。

### 5.3 其他任务的阅读顺序

下表中的实机任务先完成[实机部署](docs/REAL_ROBOT_CN.md)，再按
[开发者模式](docs/DEVELOPER_MODE_CN.md)进入对应示例要求的模式。

| 目标 | 阅读顺序与操作 |
| --- | --- |
| 测试或编写全身关节命令 | [关节控制接口](docs/JOINT_CONTROL_CN.md) → [低层示例](docs/EXAMPLES_CN.md#1-低层示例)。先用 `joints_example` 查看反馈，再在 `whole` 仿真中验证命令，最后进入实机全身关节控制模式。 |
| 编写手臂、腰部轨迹 | [关节控制接口](docs/JOINT_CONTROL_CN.md) → [低层示例](docs/EXAMPLES_CN.md#1-低层示例) → `arm_joint_example` / `arm_action_example`。修改示例轨迹，先在 `upper` 仿真中验证，再使用实机上半身关节控制模式。 |
| 使用内置行走、步态或动作 | [开发者模式](docs/DEVELOPER_MODE_CN.md) → [高层示例](docs/EXAMPLES_CN.md#2-高层示例)。进入高层运动控制模式，按 `Idle -> SuspendedStand -> RLControl` 操作，再使用运动状态、步态、动作、方向控制及反馈示例。选择 `HumanWALKAMP` 调用机器人内置步态，不会加载自定义 ONNX。 |
| 在应用中切换开发者模式 | [SDK 切换步骤](docs/DEVELOPER_MODE_CN.md#sdk-example-switching) → [developer_mode_example.cpp](high_level/developer_mode_example.cpp)。接入前理解服务请求、确认、状态检查与退出流程。 |
| 查看电池、故障、IMU 或手柄事件 | [状态监测示例](docs/EXAMPLES_CN.md#3-状态监测示例)及[外设示例](docs/EXAMPLES_CN.md#4-外设示例)。运行相应订阅程序，各开发者模式均可使用，无需启动状态机。 |
| 播放音频、调节音量或录音 | [音频示例](docs/EXAMPLES_CN.md#5-音频示例)。先把播放文件传到机器人音频目录，再使用播放、音量或录音示例。 |
| 修改地形、场景或仿真时序 | [仿真产品文档](https://github.com/DeepRoboticsLab/deep-robotics-simulation/blob/main/dr02_std/README_CN.md) → 其中的 `config.yaml` 和 `scene.xml`。在仿真仓库配置公共 world，重新进行 SDK 仿真验证；SDK 的 `BUILD_SIM` 不用于选择地形。 |

文档支持的仿真流程包括 `state_machine` 和三个低层关节示例。高层机器人命令、模式切换服务、
状态监测、外设示例与音频流程需要对应的机器人端服务，启动 MuJoCo 不会替代这些服务。

## 6. 重要安全说明

> [!WARNING]
>
> - 实机程序可能直接驱动机器人运动，运行前应确认机器人状态和运动空间安全。
> - 首次实机运行具有运动控制功能的程序时，应在具备可靠吊绳保护的条件下进行测试。
> - 控制实机前，应确保机器人已进入与目标程序匹配的开发者模式。
> - `/JOINTS_CMD` 同一时间只允许一个发布者。
> - 发生异常时，应优先使用手柄红色停止按钮退出控制。

## 7. 编译选项

| 选项 | 默认值 | 说明 |
| --- | --- | --- |
| `BUILD_DR02_STD_STATE_MACHINE` | `ON` | 编译状态机 |
| `BUILD_DR02_STD_LOW_LEVEL` | `ON` | 编译低层关节和上肢控制示例 |
| `BUILD_DR02_STD_HIGH_LEVEL` | `ON` | 编译高层运动、步态、速度和开发者模式切换示例 |
| `BUILD_DR02_STD_MONITORING` | `ON` | 编译状态监测示例 |
| `BUILD_DR02_STD_PERIPHERALS` | `ON` | 编译外设示例 |
| `BUILD_DR02_STD_AUDIO` | `ON` | 编译音频示例 |
| `BUILD_SIM` | `OFF` | 启用仿真支持；实机控制不应开启 |
