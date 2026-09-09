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

## 机器人主机

| 主机 | 地址 | Ubuntu | ROS 2 | 架构 | 互联网 |
| --- | --- | --- | --- | --- | --- |
| AOS | `10.21.33.103` | 24.04 | Jazzy | ARM64（`arm64`） | 无 |
| NOS（DR02 Pro） | `10.21.33.106` | 22.04 | Humble | ARM64（`arm64`） | 无 |

请在可联网的开发电脑上下载源码，通过机器人局域网传输，再在目标主机上针对其
ROS 版本原生编译。具体传输和 SDK 编译命令见对应产品的实机部署文档。

## 安装消息接口

脚本检查依赖，下载 [deep-robotics-msg](https://github.com/DeepRoboticsLab/deep-robotics-msg.git)，
然后编译并安装其 deb 包。缺少依赖时仅报错，不会安装依赖。目标主机需已具备 ROS 2、
编译器、CMake/CPack、colcon 和所需 ROS 消息生成包。

在可联网的开发主机上，从 SDK 根目录执行：

```bash
./scripts/install_deep_robotics_msg.sh
```

在 AOS 上，直接使用已从开发电脑传入的源码：

```bash
cd ~/deep-robotics-sdk2
./scripts/install_deep_robotics_msg.sh --source-dir ~/deep-robotics-msg
source /opt/ros/jazzy/setup.bash
```

`--source-dir` 跳过下载，无需联网。脚本不执行 apt，只创建源码临时副本进行编译，
保留原始源码并在结束时清理临时文件。仅安装生成的 deb 包时使用 sudo。

Ubuntu 24.04 自动选择 Jazzy，Ubuntu 22.04 自动选择 Humble，支持 amd64 和 arm64。
NOS 已预装 `drdds`，加载 `/opt/ros/humble/setup.bash` 即可。安装后，每个新终端
只需加载对应 ROS 环境，无需额外加载消息工作空间。

可选参数：`--ref <标签或提交>` 指定下载版本（默认 `main`），`--jobs N` 设置编译
并行数（默认 2）。`--ref` 不可与 `--source-dir` 同时使用，完整用法见 `--help`。

## 相关仓库

- [deep-robotics-msg](https://github.com/DeepRoboticsLab/deep-robotics-msg.git)：SDK 使用的 ROS 2 消息接口库，ROS 2 包名为 `drdds`。
- [deep-robotics-simulation](https://github.com/DeepRoboticsLab/deep-robotics-simulation)：DEEPRobotics 产品的 MuJoCo 仿真环境。
