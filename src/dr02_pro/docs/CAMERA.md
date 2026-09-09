# DR02 Pro RealSense Camera Guide

[Back to the DR02 Pro SDK Guide](../README.md)

The DR02 Pro is equipped with three Intel RealSense D435 depth cameras that provide color and depth images for applications such as environmental perception, object recognition, and 3D vision. All three cameras connect through USB to the NOS host (`10.21.33.106`). Camera drivers and related programs should be installed and run on this device.

> [!NOTE]
>
> The RealSense driver deployment, ROS 2 driver usage, and librealsense SDK calls in this document follow the official methods. Packages, dependencies, launch parameters, and APIs may change between versions. Refer to the official [RealSense ROS 2 Wrapper](https://github.com/realsenseai/realsense-ros) and [librealsense SDK](https://github.com/realsenseai/librealsense) documentation for the latest requirements. DR02 Pro-specific operations are identified separately in the relevant steps.

## 1. Camera Access Methods

RealSense cameras support the following two access methods:

| Method | Description | Typical Use |
| --- | --- | --- |
| ROS 2 driver | Accesses cameras through `realsense2_camera` and publishes color images, depth images, and other data as ROS 2 Topics. | Sharing data between ROS 2 nodes, viewing images in RViz2, or subscribing from a development host. |
| librealsense SDK | C/C++ or Python programs call `librealsense2` directly to acquire and process camera data without ROS 2 Topics. | Low-latency processing on the NOS host or publishing only processed results. |

The data paths are:

```text
ROS 2 driver: camera -> librealsense -> realsense2_camera -> ROS 2 Topic
Direct access: camera -> librealsense -> C/C++ program or pyrealsense2 program
```

The two methods cannot access the same camera at the same time.

## 2. Initial Setup

### 2.1 Log In to the NOS Host

Connect the development host to the robot WiFi network or the network port on the rear of the robot, then log in to the NOS host:

```bash
ssh user@10.21.33.106
```

### 2.2 Check and Configure Internet WiFi

After logging in to the NOS host, first check the network-device status:

```bash
# List network devices. Check the STATE and CONNECTION columns for wlan0.
nmcli device status
```

If `wlan0` is shown as `connected` and the connected WiFi network can access the internet, skip the remaining commands in this step.

If `wlan0` is not connected, run:

```bash
# Enable WiFi.
sudo nmcli radio wifi on

# Scan nearby networks and find the target network in the SSID column.
nmcli device wifi list

# Replace <wifi_ssid> with the actual SSID and enter the WiFi password when prompted.
sudo nmcli --ask device wifi connect "<wifi_ssid>"

# Confirm that wlan0 is connected to the target WiFi network.
nmcli connection show --active
```

Duplicate SSIDs in the list represent multiple wireless access points. Connect by SSID; specifying a BSSID is not required.

### 2.3 Install the RealSense ROS 2 Driver

The following commands use the Debian package deployment method documented by the official RealSense ROS 2 Wrapper:

```bash
sudo apt update
sudo apt install ros-humble-realsense2-camera
```

`apt` automatically installs `ros-humble-librealsense2`, the RealSense message package, and the required system dependencies.

### 2.4 Verify the Driver

```bash
source /opt/ros/humble/setup.bash
ros2 pkg prefix realsense2_camera
```

`ros2 pkg prefix` should print the installation path of `realsense2_camera`.

## 3. Camera Identification and Configuration

### 3.1 Camera Serial Numbers

Each camera is identified by its hardware serial number. Run:

```bash
rs-enumerate-devices -s
```

This command lists the connected cameras and their hardware serial numbers, but it cannot determine the physical mounting position associated with each serial number. Record all three serial numbers for use when launching the cameras.

### 3.2 Check Supported Resolutions and Frame Rates

Before launching a camera, run the following command to list all video-stream profiles supported by the connected devices:

```bash
rs-enumerate-devices
```

In the `Supported modes` output, inspect the modes for the Depth Module and RGB Camera separately. The `resolution` column gives the image resolution, and the `fps` column gives the frame rate.

## 4. Camera Operation

### 4.1 Preparation

Before starting `realsense2_camera` or a program that directly calls the librealsense SDK, use the gamepad to enter Developer Mode. This is a robot-side preparation step that stops the internal camera driver and releases the camera devices.

The cameras can be used in any of the three Developer Modes. See [Developer Modes](DEVELOPER_MODE.md#handle-switching) for the switching procedure. Running an SDK motion-control program at the same time is not required.

### 4.2 Use the ROS 2 Driver

This method starts the official RealSense ROS 2 Wrapper `realsense2_camera` driver node. The node opens the specified physical camera, acquires color and depth data, and continuously publishes the data as ROS 2 Topics. Other ROS 2 nodes can subscribe to these Topics without accessing the camera directly.

#### 4.2.1 Start and Check One Camera

For the first run or after changing a profile, select one recorded serial number and temporarily assign it to `camera1`. Replace `<camera1_serial>` with the actual serial number and remove the angle brackets:

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

The launch parameters are:

| Parameter | Description |
| --- | --- |
| `camera_namespace` | ROS 2 node namespace. Together with `camera_name`, it determines the node and Topic paths. |
| `camera_name` | Logical name of the camera. This document uses `camera1`, `camera2`, and `camera3`. |
| `serial_no` | Binds the process to a specific physical camera. Prefix a numeric serial number with `_`, for example `serial_no:=_123456789`. |
| `enable_color` | Enables or disables the color image stream. |
| `enable_depth` | Enables or disables the depth image stream. |
| `depth_module.depth_profile` | Depth-image resolution and frame rate in `<width>x<height>x<fps>` format. |
| `rgb_camera.color_profile` | Color-image resolution and frame rate in `<width>x<height>x<fps>` format. |

#### 4.2.2 Start the Remaining Cameras

After `camera1` is working correctly, temporarily assign the other two serial numbers to `camera2` and `camera3`, then start them in two additional terminals.

Terminal 2:

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

Terminal 3:

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
> `camera1`, `camera2`, and `camera3` are user-defined logical names. The `serial_no` value determines which physical camera is opened. Use the received images to identify the correspondence and adjust the serial numbers as required by the application.

### 4.3 Call the librealsense SDK Directly

`librealsense2` is the official low-level RealSense SDK. It can discover devices, configure video streams, read color and depth frames, generate point clouds, and access camera parameters. Direct librealsense SDK calls do not use ROS 2 and do not automatically publish ROS 2 Topics. To provide data to other ROS 2 nodes, the user program must publish the required messages.

#### 4.3.1 C/C++ Interface

C/C++ programs can link against and call `librealsense2` directly. Installing `ros-humble-realsense2-camera` also installs `ros-humble-librealsense2`, which can be used for C/C++ development after setup. See the official [librealsense C/C++ examples](https://github.com/realsenseai/librealsense/tree/master/examples) for API usage.

#### 4.3.2 Python Interface: pyrealsense2

`pyrealsense2` is the official Python binding for `librealsense2`, allowing Python programs to call the librealsense SDK. It is not part of ROS 2 and is not required when using `realsense2_camera`.

The current installation command does not guarantee that `pyrealsense2` is available. Check before using it:

```bash
python3 -c "import pyrealsense2; print('pyrealsense2 is available')"
```

For API and installation instructions, see the [librealsense Python Wrapper](https://github.com/realsenseai/librealsense/blob/master/wrappers/python/readme.md). For examples, see the [librealsense Python examples](https://github.com/realsenseai/librealsense/tree/master/wrappers/python/examples).

### 4.4 Stop the Cameras

When using the ROS 2 driver, press `Ctrl+C` in each camera terminal. When calling the librealsense SDK directly, stop camera access using the corresponding program's exit procedure. Confirm that all external camera processes have exited before leaving Developer Mode according to the real-robot documentation.

## 5. Data Verification

This section primarily verifies camera data published by the ROS 2 driver. When calling the librealsense SDK directly, verify the program output according to the relevant official example.

### 5.1 Topics

The primary color and depth image Topics for the three cameras are:

| Camera | Color Image | Depth Image |
| --- | --- | --- |
| `camera1` | `/camera1/camera1/color/image_raw` | `/camera1/camera1/depth/image_rect_raw` |
| `camera2` | `/camera2/camera2/color/image_raw` | `/camera2/camera2/depth/image_rect_raw` |
| `camera3` | `/camera3/camera3/color/image_raw` | `/camera3/camera3/depth/image_rect_raw` |

### 5.2 Message Types and Publication Rates

Check the message types and receive rates on the NOS host first:

```bash
source /opt/ros/humble/setup.bash
ros2 topic type /camera1/camera1/color/image_raw
ros2 topic hz /camera1/camera1/color/image_raw
ros2 topic type /camera1/camera1/depth/image_rect_raw
ros2 topic hz /camera1/camera1/depth/image_rect_raw
```

For the other two cameras, replace `camera1` with `camera2` or `camera3` in the commands.

Raw color and depth images require substantial bandwidth. When subscribing from a development host, the actual receive rate also depends on robot network bandwidth, DDS configuration, and development-host performance. Use the results measured locally on the NOS host first to determine whether camera acquisition is working correctly.

## 6. Troubleshooting

### 6.1 Fewer Than Three Cameras Are Detected

Run:

```bash
rs-enumerate-devices -s
```

If fewer than three devices are listed, check the USB connections, cables, port power, and camera indicators.

### 6.2 Device Is Reported as Busy at Startup

Confirm that the robot has entered Developer Mode and that neither the internal camera driver nor another external camera program is using the device.

### 6.3 Specified Device Is Not Found at Startup

Confirm that the serial number in the launch command matches the output of `rs-enumerate-devices -s` and that the serial number is prefixed with an underscore.

### 6.4 The Selected Profile Cannot Be Started

Run `rs-enumerate-devices` and confirm that the configured resolution, frame-rate, and format combination is supported by the device.

### 6.5 Low Image Rate or Unstable Data

Publishing color and depth images from three cameras simultaneously requires substantial USB bandwidth and system resources. Test one camera first, then check the USB connections, cables, port power, and system load on the NOS host.

When subscribing to raw images from a development host, also check robot network bandwidth. Discovering the Topics does not mean that the network can continuously carry all raw image data.

### 6.6 Topics Have No Data

Confirm that the corresponding camera process is still running, then run:

```bash
ros2 node list
ros2 topic list | grep camera
```

## 7. Official Resources

- [RealSense ROS 2 Wrapper](https://github.com/realsenseai/realsense-ros)
- [librealsense SDK](https://github.com/realsenseai/librealsense)
- [librealsense Python Wrapper](https://github.com/realsenseai/librealsense/blob/master/wrappers/python/readme.md)
- [Jetson Installation Guide](https://github.com/realsenseai/librealsense/blob/master/doc/installation_jetson.md)
