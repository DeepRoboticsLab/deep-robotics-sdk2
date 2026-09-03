# DR02 Pro SDK 使用指南

本文档是 DR02 Pro 产品软件包的中文总览。仓库级介绍和产品导航请参阅 [DEEPRobotics SDK 根 README](../../README_CN.md)。

该 ROS 2 软件包提供 DR02 Pro 运动控制状态机和独立 Topic 示例，可在开发主机或机器人端设备运行，并支持部分程序配合 MuJoCo 仿真验证。

## 功能概览

- DR02 Pro RL 状态机控制
- 低层关节状态、关节命令和上肢控制示例
- 高层运动状态、步态、速度控制和开发者模式切换示例
- 电池状态和当前故障快照监测示例
- IMU 和手柄按键外设 Topic 示例
- RealSense 深度相机部署和使用说明
- 前向 RGB 相机的 RTSP 访问方式
- WAV 文件播放、音量控制和录音示例

## 目录结构

- `state_machine/`：DR02 Pro 主状态机程序
- `low_level/`：低层关节和上肢控制 Topic 示例
- `high_level/`：高层运动状态、步态、速度控制和开发者模式切换示例
- `monitoring/`：电池状态和当前故障快照监测示例
- `peripherals/`：IMU 和手柄按键外设示例
- `audio/`：WAV 播放、音量和录音示例
- `docs/`：实机部署、开发者模式、仿真运行、状态机、示例和相机说明

## 文档导航

| 文档 | 内容 |
| --- | --- |
| [实机部署与控制](docs/REAL_ROBOT_CN.md) | 开发主机、AOS 主机和 NOS 主机的实机部署与编译方式 |
| [开发者模式](docs/DEVELOPER_MODE_CN.md) | 开发者模式介绍、手柄切换和 SDK 示例切换流程 |
| [仿真环境与运行](docs/SIMULATION_CN.md) | SDK 仿真编译、运行方式和支持范围 |
| [状态机](docs/STATE_MACHINE_CN.md) | 状态流转、运行命令、键盘控制、手柄控制和安全要求 |
| [Topic 示例](docs/EXAMPLES_CN.md) | 全部示例的 Topic、实机开发者模式、仿真支持、运行命令和注意事项 |
| [RealSense 相机](docs/CAMERA_CN.md) | NOS 主机上的相机驱动安装、ROS 2 驱动、librealsense C/C++ 接口、pyrealsense2 Python 接口、三相机启动和 Topic 验证 |
| [前向 RGB 相机](docs/FORWARD_CAMERA_CN.md) | AOS 主机上的前向 RGB 相机：RTSP 地址、必需的传输设置、如何区分已解码与未解码图像，以及如何避免查看相机占用机器人 CPU |
| [关节控制接口](docs/JOINT_CONTROL_CN.md) | 关节消息字段、数组顺序、可控范围、单位和零位定义 |

## 快速开始

首次使用时，根据控制目标选择环境准备文档，再按下一步链接阅读对应的模式和示例说明。

| 控制目标 | 环境准备 | 下一步 |
| --- | --- | --- |
| 实机 | [实机部署与控制](docs/REAL_ROBOT_CN.md) | 阅读[开发者模式](docs/DEVELOPER_MODE_CN.md)，再选择[状态机](docs/STATE_MACHINE_CN.md)或[Topic 示例](docs/EXAMPLES_CN.md) |
| 仿真 | [仿真环境与运行](docs/SIMULATION_CN.md) | 根据仿真支持范围选择[状态机](docs/STATE_MACHINE_CN.md)或[Topic 示例](docs/EXAMPLES_CN.md) |

> [!IMPORTANT]
>
> SDK 的运行位置与控制目标是两个独立概念。只要 ROS/DDS 网络互通，在开发主机、AOS 主机（`10.21.33.103`）或 NOS 主机（`10.21.33.106`）上运行的 SDK 均可能直接控制实机。

## 重要安全说明

> [!WARNING]
>
> - 实机程序可能直接驱动机器人运动，运行前应确认机器人状态和运动空间安全。
> - 首次实机运行具有运动控制功能的程序时，应在具备可靠吊绳保护的条件下进行测试。
> - 控制实机前，应确保机器人已进入与目标程序匹配的开发者模式。
> - `/JOINTS_CMD` 同一时间只允许一个发布者。
> - 发生异常时，应优先使用手柄红色停止按钮退出控制。

## 编译选项

| 选项 | 默认值 | 说明 |
| --- | --- | --- |
| `BUILD_DR02_PRO_STATE_MACHINE` | `ON` | 编译状态机 |
| `BUILD_DR02_PRO_LOW_LEVEL` | `ON` | 编译低层关节和上肢控制示例 |
| `BUILD_DR02_PRO_HIGH_LEVEL` | `ON` | 编译高层运动、步态、速度和开发者模式切换示例 |
| `BUILD_DR02_PRO_MONITORING` | `ON` | 编译状态监测示例 |
| `BUILD_DR02_PRO_PERIPHERALS` | `ON` | 编译外设示例 |
| `BUILD_DR02_PRO_AUDIO` | `ON` | 编译音频示例 |
| `BUILD_SIM` | `OFF` | 启用仿真支持；实机控制不应开启 |
