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
src/common/       公共 SDK 工具和依赖
src/<product>/    产品专属 ROS 2 软件包、部署说明和使用文档
third_party/      第三方源码和集成文件
```

## 相关仓库

- [deep-robotics-msg](https://github.com/DeepRoboticsLab/deep-robotics-msg)：SDK 使用的 ROS 2 消息接口库，ROS 2 包名为 `drdds`。
- [deep-robotics-simulation](https://github.com/DeepRoboticsLab/deep-robotics-simulation)：DEEPRobotics 产品的 MuJoCo 仿真环境。
