# DR02 Pro 实机部署与控制

[返回 DR02 Pro SDK 使用指南](../README_CN.md)

本文档说明控制 DR02 Pro 实机时的 SDK 环境、运行位置和部署与编译流程。SDK 可以在开发主机、AOS 主机（`10.21.33.103`）或 NOS 主机（`10.21.33.106`）上运行；AOS 主机和 NOS 主机均为机器人端计算设备。只要 ROS/DDS 网络互通，这些设备均可能直接控制实机。

## 1. 使用流程

1. 选择 SDK 运行位置，完成网络、环境、代码部署和编译。
2. 根据目标程序阅读[开发者模式文档](DEVELOPER_MODE_CN.md)，选择手柄或 SDK 示例切换方式并进入对应模式。
3. 在运行设备上启动程序。
4. 正常结束时先停止程序，再退出开发者模式；发生异常时使用手柄红色停止按钮。

## 2. 环境与网络准备

SDK 依赖 ROS 2、[deep-robotics-msg](https://github.com/DeepRoboticsLab/deep-robotics-msg.git) 消息接口库。[deep-robotics-msg](https://github.com/DeepRoboticsLab/deep-robotics-msg.git) 的 ROS 2 包名为 `drdds`。

消息接口库支持 deb 安装和源码编译两种方式，Ubuntu 22.04/24.04（amd64/arm64）可按[安装消息接口](../../../README_CN.md#4-安装消息接口)自动安装 deb 包。手动安装或源码编译步骤请参阅 [deep-robotics-msg](https://github.com/DeepRoboticsLab/deep-robotics-msg.git) 文档。通过 deb 安装时，加载 ROS 2 环境即可；通过源码编译时，还应加载消息接口库工作空间的 `install/setup.bash`。本文后续命令仅展示 ROS 2 环境加载。

| 运行位置 | IP 地址 | 环境状态 | 准备方式 |
| --- | --- | --- | --- |
| 开发主机 | 根据用户网络配置 | 建议使用 Ubuntu 22.04 或 Ubuntu 24.04 | 安装对应版本的 ROS 2，并安装或源码编译 [deep-robotics-msg](https://github.com/DeepRoboticsLab/deep-robotics-msg.git) 消息接口库 |
| AOS 主机 | `10.21.33.103` | Ubuntu 24.04 / Jazzy；ARM64；无互联网 | 安装 [deep-robotics-msg](https://github.com/DeepRoboticsLab/deep-robotics-msg.git) 消息接口库，并加载 ROS 2 和消息接口库环境 |
| NOS 主机 | `10.21.33.106` | Ubuntu 22.04 / Humble；ARM64；无互联网；已预装消息接口库 | 无需单独安装消息接口库 |

连接 AOS 主机或 NOS 主机时，可以使用机器人 WiFi，也可以将网线插入机器人背部网口。网络连通后，通过 SSH 登录对应设备。

机器人 WiFi 密码与 SSH 登录密码不同。WiFi 名称、WiFi 密码和 SSH 密码均以交付资料或技术支持提供的信息为准。

## 3. 部署与编译

### 3.1 在开发主机部署与编译

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
   colcon build --packages-up-to dr02_pro --cmake-args -DBUILD_PLATFORM=x86
   ```

> [!IMPORTANT]
>
> `ping` 成功仅表示 IP 网络连通。运行 SDK 前，还应确认 `ros2 topic list` 能够发现机器人 Topic。

### 3.2 在 AOS 主机（10.21.33.103）部署与编译

AOS 使用 Ubuntu 24.04、ROS 2 Jazzy 和 ARM64 架构，无法访问互联网。

通过机器人 WiFi 或连接机器人背部网口访问 AOS 主机。在可联网的开发主机上，进入包含
`deep-robotics-sdk2` 的目录，下载 [deep-robotics-msg](https://github.com/DeepRoboticsLab/deep-robotics-msg.git)
源码，再将两个仓库一起传输至 AOS。若已有所需版本的消息源码，可跳过 `git clone`：

```bash
git clone --depth 1 https://github.com/DeepRoboticsLab/deep-robotics-msg.git
scp -r deep-robotics-sdk2 deep-robotics-msg user@10.21.33.103:~/
```

AOS 主机必须安装消息接口库（ROS 2 包名为 `drdds`）。登录后，使用 SDK 中的安装
脚本在 AOS 上原生编译传入的消息源码并安装 deb 包，然后编译 SDK：

```bash
ssh user@10.21.33.103
cd ~/deep-robotics-sdk2
./scripts/install_deep_robotics_msg.sh --source-dir ~/deep-robotics-msg
source /opt/ros/jazzy/setup.bash
colcon build --packages-up-to dr02_pro --cmake-args -DBUILD_PLATFORM=arm
```

`--source-dir` 使用已传入的源码，不再下载。脚本只检查依赖，缺少时报告错误，
不安装依赖，也不执行 apt。AOS 已具备所需编译工具和 ROS 2 Jazzy 环境，直接编译
安装即可。仅安装生成的 deb 包时使用 sudo，原始源码保持不变，临时编译文件自动清理。
安装后无需额外加载消息工作空间。SDK 与 AOS 应使用匹配的消息接口版本。

### 3.3 在 NOS 主机（10.21.33.106）部署与编译

NOS 使用 Ubuntu 22.04、ROS 2 Humble 和 ARM64 架构，无法访问互联网。

通过机器人 WiFi 或连接机器人背部网口访问 NOS 主机。在包含 `deep-robotics-sdk2` 目录的路径下执行以下命令将代码传输至 NOS 主机：

```bash
scp -r deep-robotics-sdk2 user@10.21.33.106:~/
```

NOS 主机已预装消息接口库，无需单独安装或源码编译 [deep-robotics-msg](https://github.com/DeepRoboticsLab/deep-robotics-msg.git)。登录后，加载设备现有的 ROS 2 环境并编译 SDK：

```bash
ssh user@10.21.33.106
source /opt/ros/humble/setup.bash
cd ~/deep-robotics-sdk2
colcon build --packages-up-to dr02_pro --cmake-args -DBUILD_PLATFORM=arm
```

### 3.4 部署与编译说明

> [!NOTE]
>
> - `--packages-up-to dr02_pro` 会编译 `dr02_pro` 及当前工作空间内的依赖包。
> - 相关依赖已经完成编译后，日常增量开发可使用 `--packages-select dr02_pro`。

> [!WARNING]
>
> 实机编译和控制不得启用 `BUILD_SIM=ON`。

## 4. 开发者模式

开发者模式的用途、三种模式的控制范围，以及通过手柄或 SDK 示例切换模式的方法，统一参阅[开发者模式文档](DEVELOPER_MODE_CN.md)。
