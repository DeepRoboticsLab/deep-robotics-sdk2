# DEEPRobotics SDK

[English](README.md) | **简体中文**

DEEPRobotics SDK 提供面向已支持 DEEPRobotics 产品的 ROS 2 软件包和 Topic 示例。产品专属的部署、运行模式、状态机和示例说明由对应产品目录维护。

## 产品列表

| 产品 | ROS 2 软件包 | 产品文档 |
| --- | --- | --- |
| DR02 Pro | `dr02_pro` | [English](src/dr02_pro/README.md) / [中文](src/dr02_pro/README_CN.md) |
| DR02 Std | `dr02_std` | [English](src/dr02_std/README.md) / [中文](src/dr02_std/README_CN.md) |

## 仓库结构

```text
scripts/          依赖安装脚本
src/common/       公共 SDK 工具和依赖
src/<product>/    产品专属 ROS 2 软件包、部署说明和使用文档
third_party/      第三方源码和集成文件
```

## 安装消息接口

请先安装 ROS 2 并配置其 apt 软件源，然后在 SDK 根目录执行：

```bash
./scripts/install_deep_robotics_msg.sh
```

脚本在 Ubuntu 22.04 上选择 Humble，在 Ubuntu 24.04 上选择 Jazzy，支持
x86-64（`amd64`）和 64 位 ARM（`arm64`）原生编译。脚本通过 apt 安装编译依赖，
下载 [deep-robotics-msg](https://github.com/DeepRoboticsLab/deep-robotics-msg.git)，
构建 deb 包并安装到 `/opt/ros/<distro>`，最后验证 `drdds` 接口。系统安装使用 sudo。
下载和编译文件均存放在临时目录，成功或失败退出时自动清理，不修改 shell 启动文件。

```bash
source /opt/ros/humble/setup.bash  # Ubuntu 22.04；Ubuntu 24.04 请使用 jazzy
```

每个终端在执行产品文档中的编译或运行命令前，都应加载上述环境，无需额外加载消息
工作空间。已预装 `drdds` 的主机（如 NOS）可跳过此步骤。再次运行脚本会重新编译并
安装所选的上游版本。

默认使用上游 `main` 分支。可通过 `--ref <标签或完整提交SHA>` 固定版本，
通过 `--jobs N` 调整编译并行数（默认为 2，以限制 ARM 主机的内存占用）。
完整选项见 `--help`。脚本不负责安装 ROS 2 本身。

## 相关仓库

- [deep-robotics-msg](https://github.com/DeepRoboticsLab/deep-robotics-msg.git)：SDK 使用的 ROS 2 消息接口库，ROS 2 包名为 `drdds`。
- [deep-robotics-simulation](https://github.com/DeepRoboticsLab/deep-robotics-simulation)：DEEPRobotics 产品的 MuJoCo 仿真环境。
