# DR02 Pro Real-Robot Deployment and Control

[Back to the DR02 Pro SDK Guide](../README.md)

This document describes the SDK environment, runtime locations, and deployment and build process used to control a DR02 Pro robot. The SDK can run on a development host, the AOS host (`10.21.33.103`), or the NOS host (`10.21.33.106`). The AOS and NOS hosts are robot-side computers. Any of these devices may directly control the robot when ROS/DDS network communication is available.

## Workflow

1. Select the SDK runtime location, then complete network setup, environment preparation, code deployment, and compilation.
2. Read the [Developer Modes](DEVELOPER_MODE.md) document and use either the gamepad or SDK example to enter the required mode.
3. Start the program on the selected runtime device.
4. For a normal shutdown, stop the program before exiting Developer Mode. If an abnormal condition occurs, use the red stop button on the gamepad.

## Environment and Network Preparation

The SDK depends on ROS 2 and the [deep-robotics-msg](https://github.com/DeepRoboticsLab/deep-robotics-msg) message interface package. The ROS 2 package name provided by `deep-robotics-msg` is `drdds`.

The message interface package supports deb installation and source builds. See the `deep-robotics-msg` documentation for detailed instructions. With a deb installation, loading the ROS 2 environment is sufficient. With a source build, the message interface workspace `install/setup.bash` must also be loaded. The commands below show only the ROS 2 environment setup.

| Runtime Location | IP Address | Environment Status | Preparation |
| --- | --- | --- | --- |
| Development host | Depends on the user's network configuration | Ubuntu 22.04 or Ubuntu 24.04 is recommended | Install the corresponding ROS 2 distribution and install or build the `deep-robotics-msg` message interface package from source |
| AOS host | `10.21.33.103` | The message interface package must be installed | Install the `deep-robotics-msg` message interface package and load the ROS 2 and message interface environments |
| NOS host | `10.21.33.106` | The message interface package is preinstalled | No separate message interface package installation is required |

To connect to the AOS host or NOS host, use either the robot WiFi network or an Ethernet cable connected to the network port on the rear of the robot. After network connectivity is established, log in to the target device through SSH.

The robot WiFi password and SSH login password are different. Refer to the delivery materials or information provided by technical support for the WiFi SSID, WiFi password, and SSH password.

## Deployment and Build

### Development Host

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

### AOS Host (10.21.33.103)

Access the AOS host through the robot WiFi network or the network port on the rear of the robot. From the directory containing `deep-robotics-sdk2`, run the following command to transfer the source code to the AOS host:

```bash
scp -r deep-robotics-sdk2 user@10.21.33.103:~/
```

The `deep-robotics-msg` message interface package must be installed on the AOS host. The installed ROS 2 package name is `drdds`. After logging in, load the ROS 2 and message interface environments and build the SDK:

```bash
ssh user@10.21.33.103
source /opt/ros/<ros-distro>/setup.bash
cd ~/deep-robotics-sdk2
colcon build --packages-up-to dr02_pro --cmake-args -DBUILD_PLATFORM=arm
```

### NOS Host (10.21.33.106)

Access the NOS host through the robot WiFi network or the network port on the rear of the robot. From the directory containing `deep-robotics-sdk2`, run the following command to transfer the source code to the NOS host:

```bash
scp -r deep-robotics-sdk2 user@10.21.33.106:~/
```

The message interface package is preinstalled on the NOS host; `deep-robotics-msg` does not need to be installed or built separately. After logging in, load the existing ROS 2 environment and build the SDK:

```bash
ssh user@10.21.33.106
source /opt/ros/<ros-distro>/setup.bash
cd ~/deep-robotics-sdk2
colcon build --packages-up-to dr02_pro --cmake-args -DBUILD_PLATFORM=arm
```

### Deployment and Build Notes

> [!NOTE]
>
> - `--packages-up-to dr02_pro` builds `dr02_pro` and its dependencies in the current workspace.
> - After the dependencies have been built, use `--packages-select dr02_pro` for routine incremental development.

> [!WARNING]
>
> Do not enable `BUILD_SIM=ON` when building or controlling the real robot.

## Developer Modes

The purpose, control scopes, and gamepad or SDK example switching procedures for Developer Mode are described in [Developer Modes](DEVELOPER_MODE.md).
