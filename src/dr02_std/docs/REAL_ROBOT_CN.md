# DR02 Std 实机部署与控制

[返回 DR02 Std SDK 使用指南](../README_CN.md)

本文档说明控制 DR02 Std 实机时的 SDK 环境、运行位置和部署与编译流程。SDK 可以在开发主机或 AOS 主机（`10.21.33.103`）上运行；AOS 主机是机器人端计算设备。只要 ROS/DDS 网络互通，这些设备均可能直接控制实机。

## 使用流程

1. 选择 SDK 运行位置，完成网络、环境、代码部署和编译。
2. 根据目标程序阅读[开发者模式文档](DEVELOPER_MODE_CN.md)，选择手柄或 SDK 示例切换方式并进入对应模式。
3. 在运行设备上启动程序。
4. 正常结束时先停止程序，再退出开发者模式；发生异常时使用手柄红色停止按钮。

## 环境与网络准备

SDK 依赖 ROS 2、[deep-robotics-msg](https://github.com/DeepRoboticsLab/deep-robotics-msg.git) 消息接口库。[deep-robotics-msg](https://github.com/DeepRoboticsLab/deep-robotics-msg.git) 的 ROS 2 包名为 `drdds`。

消息接口库支持 deb 安装和源码编译两种方式，Ubuntu 22.04/24.04（amd64/arm64）可按[安装消息接口](../../../README_CN.md#安装消息接口)自动安装 deb 包。手动安装或源码编译步骤请参阅 [deep-robotics-msg](https://github.com/DeepRoboticsLab/deep-robotics-msg.git) 文档。通过 deb 安装时，加载 ROS 2 环境即可；通过源码编译时，还应加载消息接口库工作空间的 `install/setup.bash`。本文后续命令仅展示 ROS 2 环境加载。

| 运行位置 | IP 地址 | 环境状态 | 准备方式 |
| --- | --- | --- | --- |
| 开发主机 | 根据用户网络配置 | 建议使用 Ubuntu 22.04 或 Ubuntu 24.04 | 安装对应版本的 ROS 2，并安装或源码编译 [deep-robotics-msg](https://github.com/DeepRoboticsLab/deep-robotics-msg.git) 消息接口库 |
| AOS 主机 | `10.21.33.103` | 需安装消息接口库 | 安装 [deep-robotics-msg](https://github.com/DeepRoboticsLab/deep-robotics-msg.git) 消息接口库，并加载 ROS 2 和消息接口库环境 |

连接 AOS 主机时，可以使用机器人 WiFi，也可以将网线插入机器人背部网口。网络连通后，通过 SSH 登录 AOS 主机。

机器人 WiFi 密码与 SSH 登录密码不同。WiFi 名称、WiFi 密码和 SSH 密码均以交付资料或技术支持提供的信息为准。

## 部署与编译

### 在开发主机部署与编译

SDK 直接在开发主机上运行并控制实机时，开发主机必须通过网线连接机器人背部网口。

1. 将开发主机的有线网口 IP 和网关配置到 `10.21.33.*` 网段。网口 IP 可以使用未被占用的地址，例如 `10.21.33.100`。
2. 检查开发主机能否访问机器人设备：

   ```bash
   ping 10.21.33.103
   ```

3. 加载 ROS 2 和消息接口库环境，检查能否发现机器人 Topic：

   ```bash
   source /opt/ros/<ros-distro>/setup.bash
   ros2 topic list
   ```

4. 在开发主机上编译 SDK：

   ```bash
   colcon build --packages-up-to dr02_std --cmake-args -DBUILD_PLATFORM=x86
   ```

> [!IMPORTANT]
>
> `ping` 成功仅表示 IP 网络连通。运行 SDK 前，还应确认 `ros2 topic list` 能够发现机器人 Topic。

### 在 AOS 主机（10.21.33.103）部署与编译

通过机器人 WiFi 或连接机器人背部网口访问 AOS 主机。在包含 `deep-robotics-sdk2` 目录的路径下执行以下命令将代码传输至 AOS 主机：

```bash
scp -r deep-robotics-sdk2 user@10.21.33.103:~/
```

AOS 主机需要安装 [deep-robotics-msg](https://github.com/DeepRoboticsLab/deep-robotics-msg.git) 消息接口库。安装后提供的 ROS 2 包名为 `drdds`。登录后，加载 ROS 2 和消息接口库环境并编译 SDK：

```bash
ssh user@10.21.33.103
source /opt/ros/<ros-distro>/setup.bash
cd ~/deep-robotics-sdk2
colcon build --packages-up-to dr02_std --cmake-args -DBUILD_PLATFORM=arm
```

### 部署与编译说明

> [!NOTE]
>
> - `--packages-up-to dr02_std` 会编译 `dr02_std` 及当前工作空间内的依赖包。
> - 相关依赖已经完成编译后，日常增量开发可使用 `--packages-select dr02_std`。

> [!WARNING]
>
> 实机编译和控制不得启用 `BUILD_SIM=ON`。

## 开发者模式

开发者模式的用途、三种模式的控制范围，以及通过手柄或 SDK 示例切换模式的方法，统一参阅[开发者模式文档](DEVELOPER_MODE_CN.md)。
