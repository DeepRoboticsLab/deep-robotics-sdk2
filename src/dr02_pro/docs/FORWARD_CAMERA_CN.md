# DR02 Pro 前向 RGB 相机使用指南

[返回 DR02 Pro SDK 使用指南](../README_CN.md)

[RealSense 相机](CAMERA_CN.md)文档中介绍的三台 Intel RealSense D435 相机通过 USB 连接至 NOS 主机（`10.21.33.106`），并将数据发布为 ROS 2 Topic。在本文测量所使用的那台机器人上，这三台相机均向下倾斜安装，没有任何一台提供沿行进方向的前向视野。

该机器人的前向视野来自另一台独立的 RGB 相机，该相机挂在 AOS 主机（`10.21.33.103`）上。它不是 RealSense 相机，不由 `realsense2_camera` 打开，**也不发布任何 ROS 2 Topic**，只能通过 RTSP 视频流访问。本文说明如何读取该视频流、如何区分已解码图像与未解码图像，以及如何避免该视频流成为机器人上占用 CPU 最多的负载。

> [!NOTE]
>
> 本文所有内容均在 2026 年 9 月 3 日于一台 DR02 Pro 上实测得到——一台机器人，一天时间。主机地址、视频流路径和相机安装角度均为该机器人的实际情况。本文描述的各类现象的*形态*预计具有普遍性，但具体数值不作保证。在依赖这些信息之前，请先在自己的设备上确认相机安装角度和视频流地址。

## 相机访问方式

| 相机 | 主机 | 传输方式 | 访问方式 |
| --- | --- | --- | --- |
| 3 × RealSense D435 | NOS 主机（`10.21.33.106`） | USB | `realsense2_camera` ROS 2 驱动，或 librealsense SDK。参阅 [RealSense 相机](CAMERA_CN.md)。 |
| 前向 RGB | AOS 主机（`10.21.33.103`） | 经机器人网络的 RTSP | 任意 RTSP 客户端。不发布 ROS 2 Topic。 |

前向相机的数据链路如下：

```text
相机 -> AOS 主机上的 RTSP 服务 -> RTSP 客户端（OpenCV、FFmpeg、VLC、GStreamer）
```

如果其他 ROS 2 节点需要前向图像，应由用户程序读取 RTSP 流并自行发布所需消息。机器人上没有任何程序会自动完成这一步。

## 视频流地址

| 路径 | 结果 |
| --- | --- |
| `rtsp://10.21.33.103:8554/video1` | 提供 1280×720 的前向 RGB 视频流。 |
| `rtsp://10.21.33.103:8554/video0` | 无法打开。 |

在编写任何代码之前，先用命令行确认视频流：

```bash
# 在机器人网络中的开发主机上执行，或直接在 AOS 主机上执行
ffprobe -rtsp_transport tcp -v error -show_streams \
  rtsp://10.21.33.103:8554/video1
```

输出中报告的编码格式为 `hevc`，即 H.265。下一节中的大部分现象都源自这一点。

## 读取视频流

### 1. 必须在打开视频流之前强制使用 TCP

OpenCV 的 FFmpeg 后端默认对 RTSP 使用 UDP 传输。在该视频流上，UDP 会在解码器尚不能输出图像的那段时间内丢失分片，并显著延长这段时间。应通过 FFmpeg 后端读取的环境变量设置传输方式。该变量必须在**打开视频流之前**设置；在 `import cv2` 之前设置是可靠的顺序：

```python
import os

os.environ["OPENCV_FFMPEG_CAPTURE_OPTIONS"] = "rtsp_transport;tcp|max_delay;5000000"

import cv2

cap = cv2.VideoCapture("rtsp://10.21.33.103:8554/video1", cv2.CAP_FFMPEG)
```

直接使用 `ffmpeg` 或 `ffprobe` 时，对应参数是 `-rtsp_transport tcp`；使用 VLC 时是 `--rtsp-tcp`。

### 2. 应根据图像方差判断，而不是根据读取是否成功判断

> [!IMPORTANT]
>
> 使用默认参数打开时，该视频流能够正常连接，也能正确报告 1280×720 的分辨率，但返回的是**一致的灰色图像**，且 `cap.read()` 对其中每一帧都返回 `True`。相机返回的一致灰色图像看起来与镜头被遮挡完全一样，在产生本文的那次实测中，它最初正是被误判为镜头遮挡。实际上相机自始至终工作正常。

原因可以从 VLC 自身在同一视频流上的日志中看到，日志反复输出：

```text
hevc packetizer: Waiting for VPS/SPS/PPS
```

该视频流为 H.265/HEVC 编码，其参数集（VPS、SPS、PPS）的发送间隔较长，并不会在每个关键帧之前发送。在参数集到达之前，任何解码器都无法输出图像。所有客户端在这段时间内显示的都是灰色，包括 VLC——它只是用黑色窗口把这段时间遮住，直到能够绘制画面为止。因此，取 `read()` 返回的第一帧的采集循环正好落在这段时间内；把读取成功当作图像有效的程序，会把一台工作正常的相机报告为故障相机。

图像自身的统计量可以清晰地区分这两种情况。在该视频流上的一次采集中实测：

| 帧号 | 标准差 | 拉普拉斯方差 | 判读 |
| --- | --- | --- | --- |
| 0 | 1.06 | 6.2 | 一致灰色。`cap.read()` 返回 `True`。 |
| 20 及之后 | ≈68 | ≈330 | 正常图像。 |

两组数值相差一个数量级以上，因此阈值无需精细调整：

```python
import cv2


def frame_is_decoded(frame, min_std: float = 10.0) -> bool:
    """解码器收到参数集之后返回 True。

    在 VPS/SPS/PPS 到达之前产生的帧是一致灰色，而 read() 对其返回成功。
    该视频流实测：之前标准差为 1.06，之后约为 68。取两者之间的任意阈值均可。
    """
    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
    return float(gray.std()) >= min_std
```

拉普拉斯方差（`cv2.Laplacian(gray, cv2.CV_64F).var()`）同样能区分这两组数值，并可额外反映清晰度，但标准差计算量更小，单独使用已足以判断一帧是否完成解码。

在该检查通过之前应丢弃所有帧。在这台机器人上，图像大约在第 20 帧出现；建议设置一个较宽松的超时上限而不是固定帧数，并在超时后报告超时，而不是发布灰色图像。

### 3. 同一时间只能有一个客户端

该机器人上的 RTSP 服务表现为**同一时间只服务一个客户端**。在已有客户端连接的情况下打开第二个客户端，会导致第一个客户端被踢出并报错：

```text
454 Session Not Found
```

应按单一读取端设计。如果多个使用方都需要前向图像，应只读取一次视频流再转发——发布为 ROS 2 Topic，或使用应用已有的传输方式——而不是让每个使用方都去连接同一个 RTSP 地址。请注意，开发主机上遗留的 `ffplay`、`ffprobe` 或 VLC 窗口同样是一个客户端，这也是应用程序的视频流莫名中断的常见原因。

## 不要把开销留给机器人

在 1280×720 分辨率下解码 H.265 并非没有代价，而这部分 CPU 属于机器人。在产生本文的那次实测中，一个同时显示四路相机的监控页面成为了机器人上占用 CPU 最多的单个负载——超过了激光雷达驱动——达到**一个核的 91%**。查看相机画面的操作员，不应该与机器人自身的感知程序争抢它正站立其上的这台机器的算力。

有两项改动解决了这个问题，二者遵循同一个原则：**不要为没有人在看的图像付出代价。**

### 应释放采集句柄，而不是读取后丢弃

读取图像再丢弃，仍然会对每一帧完整解码。解码才是主要开销，之后的编码和显示相对次之。仅跳过编码而继续读取，只把进程从一个核的 121% 降到 91%，并不够。

真正消除开销的做法是关闭采集：

```python
class ForwardCamera:
    """只在有人查看时保持 RTSP 采集处于打开状态。"""

    URL = "rtsp://10.21.33.103:8554/video1"

    def __init__(self):
        self.cap = None

    def open(self):
        if self.cap is None:
            self.cap = cv2.VideoCapture(self.URL, cv2.CAP_FFMPEG)
        return self.cap.isOpened()

    def close(self):
        # 不是「停止编码」，也不是「读取后丢弃」。H.265 解码才是开销所在，
        # 只有释放采集句柄才能真正停止它。
        if self.cap is not None:
            self.cap.release()
            self.cap = None
```

重新连接并非瞬时完成——视频流需要重新等待参数集，因此上文描述的等待过程会在每次重新打开时重复一遍。每个查看会话重新打开一次的代价很小，每秒重新打开一次则不然。应在视图打开期间保持采集，视图关闭时再关闭采集。

### 对于 ROS 2 相机，应销毁订阅

同一原则也适用于 RealSense 的 Topic，而且这里的机制值得明确说明，因为看起来最直接的做法并不奏效。

**`rclpy` 会在回调执行之前完成消息反序列化。** 对不需要的图像在回调中提前 `return`，此时代价已经完整付出。三路 640×480 彩色图像以 30 Hz 发布，约相当于每秒 83 MB 的像素数据被复制进 Python，而这些图像根本没有人在看，提前 `return` 无法省下其中任何一部分。唯一不付出代价的方法就是不订阅：

```python
def set_camera_wanted(self, topic: str, wanted: bool) -> None:
    """创建和销毁订阅。在回调中过滤为时已晚。"""
    if wanted and topic not in self.subs:
        self.subs[topic] = self.node.create_subscription(
            Image, topic, self._on_image, qos)
    elif not wanted and topic in self.subs:
        self.node.destroy_subscription(self.subs.pop(topic))
```

在消息更大的点云上进行同样的测量，可以更清楚地看出这一效应的量级：在视图被查看期间一直保持打开的 `/LIDAR/POINTS` 订阅，仅仅是反序列化并丢弃约 8 Hz、每帧 4.65 MB 的点云，就占用了一个核的 4.0%；改为每次只为一帧点云创建订阅、用完即销毁后，同样的测量结果是 0.1%。端点发现约需 200 ms，这正是使得按此频率创建和销毁订阅仍然划算的原因。

### 实测结果

| 配置 | CPU |
| --- | --- |
| 四路相机视图全开；无人查看时仅跳过编码 | 一个核的 121% |
| 跳过编码，但采集句柄与订阅仍保持打开 | 一个核的 91% |
| 释放采集句柄并销毁订阅；未打开任何视图 | **一个核的 3.26%** |
| 释放采集句柄并销毁订阅；打开一路视图 | **一个核的 6.72%** |

最后两行是最终采用的配置。百分比均以单核为基准，取自机器人上 `top` 的输出，是实测过程中的观察值而非记录到文件的数据；请将其视为该效应的量级，而不是精确数值，并在自己的设备上重新测量。

> [!TIP]
>
> 保持这一特性的一个便捷做法是把它写成测试，而不是依靠习惯。回读源码，一旦发现在「被查看」路径之外打开了采集句柄，或者无条件创建了订阅就让测试失败——这样做代价很小，并且能在忙乱的下午里守住这一约束，而注释做不到这一点。

## 最小读取程序

以下程序完成连接、等待一帧可用图像、打印其统计量、写出一张 JPEG，然后释放采集句柄。它只依赖 OpenCV，可在机器人网络中的开发主机上运行，也可直接在 AOS 主机上运行。

```python
#!/usr/bin/env python3
"""从 DR02 Pro 前向 RGB 相机读取一帧可用图像。"""

import os
import sys
import time

# 必须在打开采集之前设置。在 import cv2 之前设置是可靠的顺序。
# 默认传输方式为 UDP，会在解码器等待 H.265 参数集期间丢失分片。
os.environ["OPENCV_FFMPEG_CAPTURE_OPTIONS"] = "rtsp_transport;tcp|max_delay;5000000"

import cv2

URL = "rtsp://10.21.33.103:8554/video1"
MIN_STD = 10.0        # 实测：解码前 1.06，解码后约 68
TIMEOUT_S = 20.0

cap = cv2.VideoCapture(URL, cv2.CAP_FFMPEG)
if not cap.isOpened():
    sys.exit(f"cannot open {URL}")

deadline = time.monotonic() + TIMEOUT_S
frames = 0
try:
    while time.monotonic() < deadline:
        ok, frame = cap.read()
        if not ok:
            continue
        frames += 1
        gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
        std = float(gray.std())
        if std < MIN_STD:
            # read() 成功，但图像仍是一致灰色。这是解码器在等待
            # VPS/SPS/PPS，而不是镜头被遮挡。
            continue
        focus = float(cv2.Laplacian(gray, cv2.CV_64F).var())
        print(f"frame {frames}: {frame.shape[1]}x{frame.shape[0]} "
              f"std={std:.2f} focus={focus:.1f}")
        cv2.imwrite("forward.jpg", frame)
        break
    else:
        sys.exit(f"no decodable frame within {TIMEOUT_S:.0f}s "
                 f"({frames} frames read, all below std {MIN_STD})")
finally:
    # 释放采集句柄才能真正停止 H.265 解码。
    cap.release()
```

运行该程序的机器需要安装 `python3-opencv`（或执行 `pip install opencv-python`）。本文有意没有把它做成 `dr02_pro` 的编译示例，因为那样会给一个当前仅依赖 `rclcpp` 和 `drdds` 的软件包引入 OpenCV 依赖。

## 镜头特性

前向相机的镜头广角很强。物理上笔直的天花板横梁在画面中会呈现为明显的弧线。

这一点对视觉标记（fiducial marker）有影响。标记检测依赖对直边四边形的拟合，而桶形畸变会把标记的边缘弯曲到足以让四边形检测失败的程度，但此时标记对人眼仍然完全清晰可读。也就是说，人能在图像中读出的标记，程序未必能检测到。建议在检测之前先做去畸变，或者把标记放在畸变最小的光心附近。

该相机不发布任何内参。凡是由该图像推导出的几何量——角度、方位、人工地平线——在完成标定之前都建立在假定的焦距之上。这类数值应在图像本身上标注为估计值，而不只是写在配套文档里，因为截图在传播时不会带着它的说明文字。

## 常见问题

### 图像是一整块灰色

这是解码器在等待 H.265 参数集，既不是镜头被遮挡，也不是故障。可通过图像标准差确认：`read()` 成功而标准差接近 1，即为未解码的情况。请强制使用 TCP 传输，并继续读取直到方差上升。参阅本文「应根据图像方差判断，而不是根据读取是否成功判断」一节。

### 视频流连接后中断

请检查是否存在第二个客户端。该服务表现为同一时间只服务一个客户端，第二个连接会把第一个踢出并报 `454 Session Not Found`。开发主机上被遗忘的 `ffplay` 或 VLC 窗口就足以造成这一现象。

### 视频流始终无法打开

确认路径为 `video1`。在这台机器人上 `video0` 无法打开。确认到 AOS 主机的网络可达，并确认端口有响应：

```bash
ping -c 3 10.21.33.103
ffprobe -rtsp_transport tcp -v error -show_streams rtsp://10.21.33.103:8554/video1
```

### `/dev/video*` 提示权限不足

前向相机在 AOS 主机上同时以 `/dev/video0` 和 `/dev/video1` 暴露。在实测的这台机器人上，设备节点属主为 `root:video`、权限为 `0660`，而登录账号属于 `sudo`、`adm` 和 `audio` 组，但**不**属于 `video` 组。因此这台相机看起来像是坏了，而不是不可读——程序报告无法打开设备，实际反映的是用户组权限问题，而不是硬件问题。

建议优先使用 RTSP 流，它不需要任何特殊的用户组权限。如果确实需要使用设备节点，请把账号加入 `video` 组：

```bash
sudo usermod -aG video "$USER"
# 需要重新登录，新的用户组才会生效。
id -nG
```

### 打开相机视图时机器人 CPU 占用很高

确认无人查看的相机确实被释放，而不是读取后丢弃；确认 ROS 2 相机订阅确实被销毁，而不是在回调中过滤。参阅本文「不要把开销留给机器人」一节。读取后丢弃仍然要为每一帧付出完整的解码和反序列化代价。

## 相关文档

- [RealSense 相机](CAMERA_CN.md)：NOS 主机上的三台 USB 深度相机、ROS 2 驱动及 librealsense 接口。
- [实机部署与控制](REAL_ROBOT_CN.md)：主机地址与网络访问方式。
- [开发者模式](DEVELOPER_MODE_CN.md)：模式切换。上述实测过程中没有确定开发者模式与 RTSP 服务之间的关系；当时未切换模式即可读取视频流，但这一点未经系统性验证。
