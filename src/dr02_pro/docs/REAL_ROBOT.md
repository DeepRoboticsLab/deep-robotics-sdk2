# DR02 Pro Real-Robot Deployment and Control

[Back to the DR02 Pro SDK Guide](../README.md)

This document describes the SDK environment, runtime locations, and deployment and build process used to control a DR02 Pro robot. The SDK can run on a development host, the AOS host (`10.21.33.103`), or the NOS host (`10.21.33.106`). The AOS and NOS hosts are robot-side computers. Any of these devices may directly control the robot when ROS/DDS network communication is available.

## 1. Workflow

1. Select the SDK runtime location, then complete network setup, environment preparation, code deployment, and compilation.
2. Read the [Developer Modes](DEVELOPER_MODE.md) document and use either the gamepad or SDK example to enter the required mode.
3. Start the program on the selected runtime device.
4. For a normal shutdown, stop the program before exiting Developer Mode. If an abnormal condition occurs, use the red stop button on the gamepad.

## 2. Environment and Network Preparation

The SDK depends on ROS 2 and the [deep-robotics-msg](https://github.com/DeepRoboticsLab/deep-robotics-msg.git) message interface package. The ROS 2 package name provided by [deep-robotics-msg](https://github.com/DeepRoboticsLab/deep-robotics-msg.git) is `drdds`.

The message interface package supports deb installation and source builds. For automatic deb installation on Ubuntu 22.04/24.04 (amd64/arm64), follow [Install the Message Interfaces](../../../README.md#4-install-the-message-interfaces). See the [deep-robotics-msg](https://github.com/DeepRoboticsLab/deep-robotics-msg.git) documentation for manual installation or source builds. With a deb installation, loading the ROS 2 environment is sufficient. With a source build, the message interface workspace `install/setup.bash` must also be loaded. The commands below show only the ROS 2 environment setup.

| Runtime Location | IP Address | Environment Status | Preparation |
| --- | --- | --- | --- |
| Development host | Depends on the user's network configuration | Ubuntu 22.04 or Ubuntu 24.04 is recommended | Install the corresponding ROS 2 distribution and install or build the [deep-robotics-msg](https://github.com/DeepRoboticsLab/deep-robotics-msg.git) message interface package from source |
| AOS host | `10.21.33.103` | Ubuntu 24.04 / Jazzy; ARM64; no internet | Install the [deep-robotics-msg](https://github.com/DeepRoboticsLab/deep-robotics-msg.git) message interface package and load the ROS 2 and message interface environments |
| NOS host | `10.21.33.106` | Ubuntu 22.04 / Humble; ARM64; no internet; messages preinstalled | No separate message interface package installation is required |

To connect to the AOS host or NOS host, use either the robot WiFi network or an Ethernet cable connected to the network port on the rear of the robot. After network connectivity is established, log in to the target device through SSH.

The robot WiFi password and SSH login password are different. Refer to the delivery materials or information provided by technical support for the WiFi SSID, WiFi password, and SSH password.

## 3. Deployment and Build

### 3.1 Development Host

When the SDK runs on a development host to control the real robot, the host must connect to the network port on the rear of the robot through Ethernet.

1. Configure the host Ethernet interface IP address and gateway on the `10.21.33.*` subnet. Use an available address such as `10.21.33.100`.
2. Verify that the host can reach the robot device:

   ```bash
   ping 10.21.33.103
   ```

3. Load the ROS 2 and message interface environments, then verify that robot Topics can be discovered:

   ```bash
   source /opt/ros/<ros-distro>/setup.bash
   ros2 topic list
   ```

4. Build the SDK on the development host:

   ```bash
   colcon build --packages-up-to dr02_pro --cmake-args -DBUILD_PLATFORM=x86
   ```

> [!IMPORTANT]
>
> A successful `ping` confirms only IP network connectivity. Before running the SDK, also confirm that `ros2 topic list` can discover robot Topics.

### 3.2 AOS Host (10.21.33.103)

The AOS runs Ubuntu 24.04 and ROS 2 Jazzy on ARM64 and has no internet access.

Access the AOS host through the robot WiFi network or the network port on the rear
of the robot. On an internet-connected development host, from the directory containing
`deep-robotics-sdk2`, download the [deep-robotics-msg](https://github.com/DeepRoboticsLab/deep-robotics-msg.git)
source and transfer both repositories to the AOS host. Skip `git clone` if you
already have the required message source revision:

```bash
git clone --depth 1 https://github.com/DeepRoboticsLab/deep-robotics-msg.git
scp -r deep-robotics-sdk2 deep-robotics-msg user@10.21.33.103:~/
```

The message interfaces (ROS 2 package `drdds`) must be installed on the AOS host.
Log in and use the SDK installer to build the transferred message source natively
on the AOS and install its Debian package, then build the SDK:

```bash
ssh user@10.21.33.103
cd ~/deep-robotics-sdk2
./scripts/install_deep_robotics_msg.sh --source-dir ~/deep-robotics-msg
source /opt/ros/jazzy/setup.bash
colcon build --packages-up-to dr02_pro --cmake-args -DBUILD_PLATFORM=arm
```

`--source-dir` uses the transferred source without downloading it again. The
script checks dependencies and reports any missing ones; it never installs
dependencies or runs apt. The AOS already has the required build tools and ROS 2
Jazzy environment, so it can compile and install directly. Sudo is used only to
install the built Debian package. Supplied sources are preserved and temporary
build files are cleaned up. No separate message workspace needs to be sourced.
Use matching message interface versions on the SDK and AOS hosts.

### 3.3 NOS Host (10.21.33.106)

The NOS runs Ubuntu 22.04 and ROS 2 Humble on ARM64 and has no internet access.

Access the NOS host through the robot WiFi network or the network port on the rear of the robot. From the directory containing `deep-robotics-sdk2`, run the following command to transfer the source code to the NOS host:

```bash
scp -r deep-robotics-sdk2 user@10.21.33.106:~/
```

The message interface package is preinstalled on the NOS host; [deep-robotics-msg](https://github.com/DeepRoboticsLab/deep-robotics-msg.git) does not need to be installed or built separately. After logging in, load the existing ROS 2 environment and build the SDK:

```bash
ssh user@10.21.33.106
source /opt/ros/humble/setup.bash
cd ~/deep-robotics-sdk2
colcon build --packages-up-to dr02_pro --cmake-args -DBUILD_PLATFORM=arm
```

### 3.4 Deployment and Build Notes

> [!NOTE]
>
> - `--packages-up-to dr02_pro` builds `dr02_pro` and its dependencies in the current workspace.
> - After the dependencies have been built, use `--packages-select dr02_pro` for routine incremental development.

> [!WARNING]
>
> Do not enable `BUILD_SIM=ON` when building or controlling the real robot.

## 4. Developer Modes

The purpose, control scopes, and gamepad or SDK example switching procedures for Developer Mode are described in [Developer Modes](DEVELOPER_MODE.md).
