# DR02 Std 仿真环境与运行

[返回 DR02 Std SDK 使用指南](../README_CN.md)

本文档说明 DR02 Std SDK 与仿真环境配合时，SDK 侧的编译和运行方式。仿真程序的环境配置及启动方式请参阅 [deep-robotics-simulation](https://github.com/DeepRoboticsLab/deep-robotics-simulation) 仓库。

## 环境准备

开发主机应准备：

- 带 `ament_cmake` 的 ROS 2 环境。
- 已安装或已加载 [deep-robotics-msg](https://github.com/DeepRoboticsLab/deep-robotics-msg.git) 消息接口库，其 ROS 2 包名为 `drdds`。

消息接口库支持 deb 安装和源码编译两种方式，Ubuntu 22.04/24.04（amd64/arm64）可按[安装消息接口](../../../README_CN.md#安装消息接口)自动安装 deb 包。手动安装或源码编译步骤请参阅 [deep-robotics-msg](https://github.com/DeepRoboticsLab/deep-robotics-msg.git) 文档。通过 deb 安装时，加载 ROS 2 环境即可；通过源码编译时，还应加载消息接口库工作空间的 `install/setup.bash`。本文后续命令仅展示 ROS 2 环境加载。

```bash
source /opt/ros/<ros-distro>/setup.bash
```

## 编译 SDK

```bash
colcon build --packages-up-to dr02_std --cmake-args -DBUILD_PLATFORM=x86 -DBUILD_SIM=ON
```

> [!WARNING]
>
> `BUILD_SIM=ON` 仅用于仿真环境，不得用于实机控制。

## 支持范围

当前支持以下程序通过 `/JOINTS_DATA` 和 `/JOINTS_CMD` 与仿真配合运行：

- `state_machine`
- `joints_example`
- `arm_joint_example`
- `arm_action_example`

高层、外设和音频示例当前不支持仿真验证。

## 运行 SDK

SDK 与仿真程序是两个独立进程，应在两个终端中运行。请先按照上述仿真仓库文档启动仿真程序，并确保两个终端加载相同的 ROS 2 和消息接口库环境、使用相同的 `ROS_DOMAIN_ID`。

SDK 终端以运行状态机为例：

```bash
export ROS_DOMAIN_ID=<domain-id>
source /opt/ros/<ros-distro>/setup.bash
source install/setup.bash
ros2 run dr02_std state_machine
```

低层示例的运行命令请参阅[Topic 示例](EXAMPLES_CN.md)。
