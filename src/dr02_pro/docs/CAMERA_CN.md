# DR02 Pro RealSense 相机使用指南

[返回 DR02 Pro SDK 使用指南](../README_CN.md)

DR02 Pro 配置三台 Intel RealSense D435 深度相机，可提供彩色图像和深度图像，为环境感知、目标识别和三维视觉等应用提供数据。三台相机均通过 USB 连接至 NOS 主机（`10.21.33.106`），相机驱动及相关程序应在该设备上安装和运行。

> [!NOTE]
>
> 本文中的 RealSense 驱动部署、ROS 2 驱动使用和 librealsense SDK 调用均采用官方提供的方式。软件包、依赖关系、启动参数和 API 可能随版本更新，最新要求以 [RealSense ROS 2 Wrapper](https://github.com/realsenseai/realsense-ros) 和 [librealsense SDK](https://github.com/realsenseai/librealsense) 官方文档为准。DR02 Pro 特有的产品操作将在对应步骤中单独说明。

## 1. 相机访问方式

RealSense 相机支持以下两种使用方式：

| 使用方式 | 说明 | 适用场景 |
| --- | --- | --- |
| ROS 2 驱动 | 通过 `realsense2_camera` 访问相机，并将彩色图像、深度图像等数据发布为 ROS 2 Topic。 | ROS 2 节点间共享数据、使用 RViz2 查看图像或从开发主机订阅数据。 |
| librealsense SDK | C/C++ 或 Python 程序直接调用 `librealsense2` 获取和处理相机数据，不经过 ROS 2 Topic。 | 在 NOS 主机本地进行低延迟图像处理，或只对外发布处理结果。 |

两种方式的数据链路如下：

```text
ROS 2 驱动：相机 -> librealsense -> realsense2_camera -> ROS 2 Topic
直接调用：相机 -> librealsense -> C/C++ 程序或 pyrealsense2 程序
```

两种方式不能同时访问同一台相机。

## 2. 首次部署

### 2.1 登录 NOS 主机

先将开发主机连接至机器人 WiFi 或机器人背部网口，再登录 NOS 主机：

```bash
ssh user@10.21.33.106
```

### 2.2 检查并配置联网 WiFi

登录 NOS 主机后，首先查看网络设备状态：

```bash
# 查看网络设备，重点检查 wlan0 的 STATE 和 CONNECTION
nmcli device status
```

如果 `wlan0` 已显示为 `connected`，且所连接的 WiFi 可以访问互联网，请跳过本步骤的剩余命令。

如果 `wlan0` 未连接，请依次执行：

```bash
# 启用 WiFi
sudo nmcli radio wifi on

# 扫描附近的 WiFi，在 SSID 列中找到需要连接的网络名称
nmcli device wifi list

# 将 <wifi_ssid> 替换为实际的 SSID，并根据提示输入 WiFi 密码
sudo nmcli --ask device wifi connect "<wifi_ssid>"

# 查看当前连接，确认 wlan0 已连接至目标 WiFi
nmcli connection show --active
```

列表中同名的 SSID 表示多个无线接入点，直接使用 SSID 连接即可，无需指定 BSSID。

### 2.3 安装 RealSense ROS 2 驱动

以下命令采用 RealSense 官方 ROS 2 Wrapper 文档提供的 Debian 包部署方式：

```bash
sudo apt update
sudo apt install ros-humble-realsense2-camera
```

`apt` 会自动安装 `ros-humble-librealsense2`、RealSense 消息包及其系统依赖。

### 2.4 检查驱动

```bash
source /opt/ros/humble/setup.bash
ros2 pkg prefix realsense2_camera
```

`ros2 pkg prefix` 应输出 `realsense2_camera` 的安装路径。

## 3. 相机识别与配置

### 3.1 相机序列号

三台相机通过各自的硬件序列号进行区分。执行：

```bash
rs-enumerate-devices -s
```

该命令只能列出当前连接的相机及其硬件序列号，无法直接确定每个序列号对应的物理安装位置。请记录三台相机的序列号，后续启动时使用。

### 3.2 查看支持的分辨率和帧率

启动相机前，执行以下命令查看已连接设备支持的全部视频流 profile：

```bash
rs-enumerate-devices
```

在输出的 `Supported modes` 中分别查看 Depth Module 和 RGB Camera 支持的模式。`resolution` 列表示分辨率，`fps` 列表示帧率。

## 4. 相机运行

### 4.1 运行前准备

启动 `realsense2_camera` 或直接调用 librealsense SDK 的程序前，需要先通过手柄进入开发者模式。这是机器人侧的准备操作，用于停止内部相机驱动并释放相机设备。

三种开发者控制模式中的任意一种均可使用相机，具体切换步骤请参阅[开发者模式文档](DEVELOPER_MODE_CN.md#handle-switching)。使用相机不要求同时启动 SDK 运动控制程序。

### 4.2 使用 ROS 2 驱动

本方式启动 RealSense 官方 ROS 2 Wrapper 提供的 `realsense2_camera` 驱动节点。该节点负责打开指定的物理相机、采集彩色和深度数据，并将数据持续发布为 ROS 2 Topic。其他 ROS 2 节点无需直接访问相机，可通过订阅对应 Topic 获取数据。

#### 4.2.1 启动并检查单台相机

首次运行或修改 profile 后，应先从记录的序列号中任选一个，临时作为 `camera1` 启动。将 `<camera1_serial>` 替换为实际序列号，不保留尖括号：

```bash
source /opt/ros/humble/setup.bash
ros2 launch realsense2_camera rs_launch.py \
  camera_namespace:=camera1 \
  camera_name:=camera1 \
  serial_no:=_<camera1_serial> \
  enable_color:=true \
  enable_depth:=true \
  depth_module.depth_profile:=640x480x30 \
  rgb_camera.color_profile:=640x480x30
```

上述启动参数说明如下：

| 参数 | 说明 |
| --- | --- |
| `camera_namespace` | ROS 2 节点的命名空间。与 `camera_name` 共同决定节点和 Topic 路径。 |
| `camera_name` | 当前相机的逻辑名称。本文档使用 `camera1`、`camera2` 和 `camera3`。 |
| `serial_no` | 将启动进程绑定到指定的物理相机。纯数字序列号前必须添加 `_`，例如 `serial_no:=_123456789`。 |
| `enable_color` | 是否启用彩色图像流。 |
| `enable_depth` | 是否启用深度图像流。 |
| `depth_module.depth_profile` | 深度图像的分辨率和帧率，格式为 `<width>x<height>x<fps>`。 |
| `rgb_camera.color_profile` | 彩色图像的分辨率和帧率，格式为 `<width>x<height>x<fps>`。 |


#### 4.2.2 启动其余相机

`camera1` 工作正常后，将另外两个序列号临时分配给 `camera2` 和 `camera3`，并在另外两个终端中分别启动。

终端 2：

```bash
source /opt/ros/humble/setup.bash
ros2 launch realsense2_camera rs_launch.py \
  camera_namespace:=camera2 \
  camera_name:=camera2 \
  serial_no:=_<camera2_serial> \
  enable_color:=true \
  enable_depth:=true \
  depth_module.depth_profile:=640x480x30 \
  rgb_camera.color_profile:=640x480x30
```

终端 3：

```bash
source /opt/ros/humble/setup.bash
ros2 launch realsense2_camera rs_launch.py \
  camera_namespace:=camera3 \
  camera_name:=camera3 \
  serial_no:=_<camera3_serial> \
  enable_color:=true \
  enable_depth:=true \
  depth_module.depth_profile:=640x480x30 \
  rgb_camera.color_profile:=640x480x30
```

> [!NOTE]
>
> `camera1`、`camera2` 和 `camera3` 是用户定义的逻辑名称，实际打开的物理相机由 `serial_no` 决定。用户可根据接收到的图像确认对应关系，并按应用需要调整序列号。

### 4.3 直接调用 librealsense SDK

`librealsense2` 是 RealSense 官方底层 SDK，可用于发现设备、配置视频流、读取彩色和深度帧、生成点云并访问相机参数。直接调用 librealsense SDK 不经过 ROS 2，也不会自动发布 ROS 2 Topic；如需向其他 ROS 2 节点提供数据，应由用户程序自行发布所需消息。

#### 4.3.1 C/C++ 接口

C/C++ 程序可以直接链接并调用 `librealsense2`。安装 `ros-humble-realsense2-camera` 时，会同时安装 `ros-humble-librealsense2`，完成部署后即可用于 C/C++ 开发。具体接口和示例请参阅 [librealsense C/C++ 示例](https://github.com/realsenseai/librealsense/tree/master/examples)。

#### 4.3.2 Python 接口 pyrealsense2

`pyrealsense2` 是 `librealsense2` 的官方 Python 绑定，Python 程序可通过它调用 librealsense SDK。`pyrealsense2` 不属于 ROS 2；使用 `realsense2_camera` 时不需要安装或导入它。

`pyrealsense2` 不保证随当前安装命令提供，使用前应执行以下命令检查：

```bash
python3 -c "import pyrealsense2; print('pyrealsense2 is available')"
```

具体接口和安装方式请参阅 [librealsense Python Wrapper](https://github.com/realsenseai/librealsense/blob/master/wrappers/python/readme.md)，使用示例请参阅 [librealsense Python 示例](https://github.com/realsenseai/librealsense/tree/master/wrappers/python/examples)。

### 4.4 停止相机

使用 ROS 2 驱动时，在各相机终端中分别按 `Ctrl+C`。直接调用 librealsense SDK 时，按照对应程序的退出方式停止相机访问。确认所有外部相机进程均已结束后，再按照实机文档退出开发者模式。

## 5. 数据验证

本节主要用于验证 ROS 2 驱动发布的相机数据。直接调用 librealsense SDK 时，请按照对应的官方示例验证程序运行结果。

### 5.1 Topic

三台相机的主要彩色和深度图像 Topic 如下：

| 相机 | 彩色图像 | 深度图像 |
| --- | --- | --- |
| `camera1` | `/camera1/camera1/color/image_raw` | `/camera1/camera1/depth/image_rect_raw` |
| `camera2` | `/camera2/camera2/color/image_raw` | `/camera2/camera2/depth/image_rect_raw` |
| `camera3` | `/camera3/camera3/color/image_raw` | `/camera3/camera3/depth/image_rect_raw` |

### 5.2 消息类型和发布频率

建议先在 NOS 主机上检查消息类型和接收频率：

```bash
source /opt/ros/humble/setup.bash
ros2 topic type /camera1/camera1/color/image_raw
ros2 topic hz /camera1/camera1/color/image_raw
ros2 topic type /camera1/camera1/depth/image_rect_raw
ros2 topic hz /camera1/camera1/depth/image_rect_raw
```

其余两台相机可将命令中的 `camera1` 替换为 `camera2` 或 `camera3`。

原始彩色和深度图像的数据量较大。从开发主机订阅时，实际接收频率还会受到机器人网络带宽、DDS 配置和开发主机性能影响，因此应先以 NOS 主机上的测试结果判断相机采集是否正常。

## 6. 常见问题

### 6.1 未发现三台相机

执行以下命令重新检查设备：

```bash
rs-enumerate-devices -s
```

若设备数量不足，请检查 USB 连接、线缆、接口供电以及相机指示灯。

### 6.2 启动时提示设备被占用

确认机器人已经进入开发者模式，并确认机器人内部相机驱动或其他外部相机程序没有占用该设备。

### 6.3 启动时提示找不到指定设备

确认启动命令中的序列号与 `rs-enumerate-devices -s` 输出一致，并确认序列号前包含下划线。

### 6.4 指定的 profile 无法启动

执行 `rs-enumerate-devices`，确认所配置的分辨率、帧率和格式组合受当前设备支持。

### 6.5 图像频率低或数据不稳定

三台相机同时发布彩色和深度图像会占用较高的 USB 带宽和系统资源。请先单独测试一台相机，再检查 USB 连接、线缆、接口供电和 NOS 主机的系统负载。

从开发主机订阅原始图像时，还应检查机器人网络带宽。能够发现 Topic 并不代表网络能够持续传输全部原始图像数据。

### 6.6 Topic 没有数据

确认对应相机进程仍在运行，并执行：

```bash
ros2 node list
ros2 topic list | grep camera
```

## 7. 官方资源

- [RealSense ROS 2 Wrapper](https://github.com/realsenseai/realsense-ros)
- [librealsense SDK](https://github.com/realsenseai/librealsense)
- [librealsense Python Wrapper](https://github.com/realsenseai/librealsense/blob/master/wrappers/python/readme.md)
- [Jetson 安装指南](https://github.com/realsenseai/librealsense/blob/master/doc/installation_jetson.md)
