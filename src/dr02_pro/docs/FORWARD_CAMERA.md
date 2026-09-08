# DR02 Pro Forward RGB Camera Guide

[Back to the DR02 Pro SDK Guide](../README.md)

The three Intel RealSense D435 cameras described in [RealSense Cameras](CAMERA.md) connect to the NOS host (`10.21.33.106`) and publish ROS 2 Topics. On the robot used for the measurements below, all three were mounted at a downward angle, and none of them provided a forward view along the direction of travel.

The forward view on that robot came from a separate RGB camera attached to the AOS host (`10.21.33.103`). That camera is not a RealSense, is not opened by `realsense2_camera`, and **publishes no ROS 2 Topic**. It is reached as an RTSP video stream. This document describes how to read that stream, how to tell a decoded picture from an undecoded one, and how to keep the stream from becoming the largest CPU consumer on the robot.

> [!NOTE]
>
> Everything in this document was measured on a single DR02 Pro on 3 September 2026 — one robot, one day. Host addresses, stream paths, and camera mounting angles are that robot's. The *shape* of each behavior described here is expected to generalize; the specific values are not promised to. Confirm the mounting angles and the stream address on your own unit before relying on them.

## Camera Access Methods

| Camera | Host | Transport | Access method |
| --- | --- | --- | --- |
| 3 × RealSense D435 | NOS host (`10.21.33.106`) | USB | `realsense2_camera` ROS 2 driver, or the librealsense SDK. See [RealSense Cameras](CAMERA.md). |
| Forward RGB | AOS host (`10.21.33.103`) | RTSP over the robot network | Any RTSP client. No ROS 2 Topic is published. |

The data path for the forward camera is:

```text
camera -> RTSP server on the AOS host -> RTSP client (OpenCV, FFmpeg, VLC, GStreamer)
```

If the forward image is required by other ROS 2 nodes, the user program must read the RTSP stream and publish the messages itself. Nothing on the robot does this automatically.

## Stream Address

| Path | Result |
| --- | --- |
| `rtsp://10.21.33.103:8554/video1` | Serves the forward RGB stream at 1280×720. |
| `rtsp://10.21.33.103:8554/video0` | Does not open. |

Confirm the stream from the command line before writing any code against it:

```bash
# From a development host on the robot network, or on the AOS host itself.
ffprobe -rtsp_transport tcp -v error -show_streams \
  rtsp://10.21.33.103:8554/video1
```

The codec reported is `hevc` — H.265. That single fact explains most of the behavior in the next section.

## Reading the Stream

### 1. Force TCP Before the Capture Is Opened

OpenCV's FFmpeg backend defaults to UDP transport for RTSP. On this stream, UDP loses fragments during the interval before the decoder can produce a picture and extends that interval considerably. Set the transport through the environment variable that the FFmpeg backend reads. The variable must be set **before** the capture is opened; setting it before `import cv2` is the reliable order:

```python
import os

os.environ["OPENCV_FFMPEG_CAPTURE_OPTIONS"] = "rtsp_transport;tcp|max_delay;5000000"

import cv2

cap = cv2.VideoCapture("rtsp://10.21.33.103:8554/video1", cv2.CAP_FFMPEG)
```

With `ffmpeg` or `ffprobe` directly, the equivalent is `-rtsp_transport tcp`. With VLC, it is `--rtsp-tcp`.

### 2. Judge a Frame by Its Variance, Not by Whether the Read Succeeded

> [!IMPORTANT]
>
> Opened with default options, this stream connects, correctly reports 1280×720, and hands back a **uniform grey frame**. `cap.read()` returns `True` for every one of those frames. A uniform grey frame from a camera looks exactly like a covered lens, and it was initially reported as one during the session that produced this document. The camera was working correctly the whole time.

The cause is visible in VLC's own log on the same stream, which repeats:

```text
hevc packetizer: Waiting for VPS/SPS/PPS
```

The stream is H.265/HEVC, and its parameter sets — VPS, SPS, and PPS — are transmitted infrequently rather than ahead of every keyframe. No decoder can produce a picture until one arrives. Every client shows grey during that window, including VLC, which hides it behind a black window until it has something to draw. A capture loop that takes the first frame `read()` returns to it therefore samples that window, and a program that treats a successful read as a valid image will report a working camera as a broken one.

A frame's own statistics separate the two cases cleanly. Measured across one capture on this stream:

| Frame | Standard deviation | Laplacian variance | Reading |
| --- | --- | --- | --- |
| 0 | 1.06 | 6.2 | Uniform grey. `cap.read()` returned `True`. |
| 20 onward | ≈68 | ≈330 | A picture. |

The two populations are separated by more than an order of magnitude, so the threshold does not need to be tuned carefully:

```python
import cv2


def frame_is_decoded(frame, min_std: float = 10.0) -> bool:
    """True once the decoder has received its parameter sets.

    A frame produced before VPS/SPS/PPS arrive is uniform grey, and read()
    reports success for it. Measured on this stream: std 1.06 before,
    approximately 68 after. Any threshold between the two works.
    """
    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
    return float(gray.std()) >= min_std
```

Laplacian variance (`cv2.Laplacian(gray, cv2.CV_64F).var()`) separates the same two populations and additionally reports focus, but the standard deviation is cheaper and is sufficient on its own to decide whether a frame is decoded.

Discard frames until the check passes. On this robot the picture appeared at approximately frame 20; allow a generous bound rather than a fixed count, and report a timeout rather than publishing grey.

### 3. One Client at a Time

The RTSP server on this robot appeared to serve **one client at a time**. Opening a second consumer while the first was connected evicted the first, which then failed with:

```text
454 Session Not Found
```

Plan for a single reader. If several consumers need the forward image, read the stream once and republish it — as a ROS 2 Topic, or over whatever transport the application already uses — rather than pointing every consumer at the RTSP URL. Note that a stray `ffplay`, `ffprobe`, or VLC window left open on a development host is a consumer, and is a common reason for an application's stream to die without an obvious cause.

## Keeping the Cost Off the Robot

Decoding H.265 at 1280×720 is not free, and the CPU it uses belongs to the robot. During the session that produced this document, a monitoring view that displayed all four cameras became the largest single CPU consumer on the robot — larger than the LiDAR driver — at **91% of one core**. An operator looking at a camera should not be competing with the robot's own perception stack for the machine it is standing on.

Two changes fixed it. Both are the same principle: **do not pay for pixels nobody is looking at.**

### Release the Capture, Do Not Read and Discard

Reading frames and throwing them away still decodes every one of them. The decode is the cost; the encode and display that follow it are comparatively minor. Skipping only the encode while continuing to read took the process from 121% to 91% of one core, which was not enough.

Closing the capture is what stops the cost:

```python
class ForwardCamera:
    """Holds the RTSP capture open only while someone is watching."""

    URL = "rtsp://10.21.33.103:8554/video1"

    def __init__(self):
        self.cap = None

    def open(self):
        if self.cap is None:
            self.cap = cv2.VideoCapture(self.URL, cv2.CAP_FFMPEG)
        return self.cap.isOpened()

    def close(self):
        # Not "stop encoding" and not "read and discard". The H.265 decode is
        # the cost, and only releasing the capture stops it.
        if self.cap is not None:
            self.cap.release()
            self.cap = None
```

Reconnecting is not instantaneous — the stream needs its parameter sets again, so the wait described above repeats on every reopen. Reopening once per viewer session is inexpensive; reopening once per second is not. Keep the capture open while the view is open, and close it when the view closes.

### For the ROS 2 Cameras, Destroy the Subscription

The same principle applies to the RealSense Topics, and the mechanism is worth stating explicitly because the obvious approach does not work.

**`rclpy` deserializes a message before the callback runs.** A callback that returns early on an unwanted frame has already paid for that frame in full. Three 640×480 color streams at 30 Hz is roughly 83 MB/s of pixel data copied into Python for images nobody is looking at, and an early `return` does not avoid any of it. The only way not to pay is not to be subscribed:

```python
def set_camera_wanted(self, topic: str, wanted: bool) -> None:
    """Create and destroy the subscription. Filtering in the callback is too late."""
    if wanted and topic not in self.subs:
        self.subs[topic] = self.node.create_subscription(
            Image, topic, self._on_image, qos)
    elif not wanted and topic in self.subs:
        self.node.destroy_subscription(self.subs.pop(topic))
```

The same measurement on the point cloud, where the messages are larger, makes the size of the effect clear: a `/LIDAR/POINTS` reader held open while its view was watched cost 4.0% of one core doing nothing but deserializing and discarding 4.65 MB sweeps at approximately 8 Hz. Created, used for one sweep, and destroyed again, the same measurement was 0.1%. Endpoint discovery takes roughly 200 ms, which is what makes creating and destroying a subscription affordable at that rate.

### Measured Result

| Configuration | CPU |
| --- | --- |
| All four camera views open; only the encode skipped when unwatched | 121% of one core |
| Encode skipped, captures and subscriptions still held open | 91% of one core |
| Captures released and subscriptions destroyed; no view open | **3.26% of one core** |
| Captures released and subscriptions destroyed; one view open | **6.72% of one core** |

The last two rows are the working configuration. Percentages are of a single core, as reported by `top` on the robot, and were observed during the session rather than recorded to a file; treat them as the size of the effect rather than as precise figures, and re-measure on your own unit.

> [!TIP]
>
> A convenient way to hold this property is to make it a test rather than a habit. Reading the source back and failing if a capture is opened outside the watched path — or if a subscription is created unconditionally — costs very little and survives a hurried afternoon in a way that a comment does not.

## A Minimal Reader

The following program connects, waits for a decodable frame, prints its statistics, writes one JPEG, and releases the capture. It uses only OpenCV and is intended to be run on a development host on the robot network, or on the AOS host itself.

```python
#!/usr/bin/env python3
"""Read one usable frame from the DR02 Pro forward RGB camera."""

import os
import sys
import time

# Must be set before the capture is opened. Setting it before importing cv2 is
# the reliable order. UDP is the default and loses fragments while the decoder
# waits for its H.265 parameter sets.
os.environ["OPENCV_FFMPEG_CAPTURE_OPTIONS"] = "rtsp_transport;tcp|max_delay;5000000"

import cv2

URL = "rtsp://10.21.33.103:8554/video1"
MIN_STD = 10.0        # measured: 1.06 before decode, approximately 68 after
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
            # read() succeeded and the frame is still uniform grey. This is the
            # decoder waiting for VPS/SPS/PPS, not a covered lens.
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
    # Releasing the capture is what stops the H.265 decode.
    cap.release()
```

Requires `python3-opencv` (or `pip install opencv-python`) on the machine running it. It is deliberately not built as a `dr02_pro` example, because that would add an OpenCV dependency to a package whose examples currently need only `rclcpp` and `drdds`.

## Lens Characteristics

The forward camera's lens is strongly wide-angle. A physically straight ceiling beam renders as a pronounced arc across the frame.

This matters for fiducial markers. Marker detection fits straight-edged quadrilaterals, and barrel distortion bows the edges of a marker enough to defeat the quad test while the marker remains perfectly legible to a person. A marker that a human observer can read in the image may still not be detected. Undistort before detection, or place markers near the optical center where the distortion is smallest.

The camera publishes no intrinsics. Any geometry derived from this image — an angle, a bearing, an artificial horizon — rests on an assumed focal length until the camera is calibrated. Label such values as estimated in the image itself, not only in the surrounding documentation, because a screenshot travels without its caption.

## Troubleshooting

### The Image Is a Uniform Grey Rectangle

This is the decoder waiting for the H.265 parameter sets, not a covered lens and not a fault. Confirm by checking the frame's standard deviation: a value near 1 with a successful `read()` is the undecoded case. Force TCP transport and continue reading until the variance rises. See [Judge a Frame by Its Variance](#2-judge-a-frame-by-its-variance-not-by-whether-the-read-succeeded).

### The Stream Connects and Then Stops

Check for a second consumer. The server appeared to serve one client at a time, and a second connection evicts the first with `454 Session Not Found`. A forgotten `ffplay` or VLC window on a development host is enough.

### The Stream Never Opens

Confirm the path is `video1`. `video0` does not open on this robot. Confirm network reachability to the AOS host, and that the port is answering:

```bash
ping -c 3 10.21.33.103
ffprobe -rtsp_transport tcp -v error -show_streams rtsp://10.21.33.103:8554/video1
```

### `/dev/video*` Reports Permission Denied

The forward camera is also exposed as `/dev/video0` and `/dev/video1` on the AOS host. On the robot measured, the device nodes were `root:video` with mode `0660`, while the login account belonged to `sudo`, `adm`, and `audio` but **not** to `video`. The camera therefore appears broken rather than unreadable — an application reporting that it cannot open the device is reporting a group membership problem, not a hardware one.

Prefer the RTSP stream, which needs no special group membership. If the device nodes are genuinely required, add the account to the `video` group:

```bash
sudo usermod -aG video "$USER"
# Log out and back in for the new group to take effect.
id -nG
```

### CPU on the Robot Is High While a Camera View Is Open

Confirm that unwatched cameras are actually released rather than read and discarded, and that ROS 2 camera subscriptions are destroyed rather than filtered in the callback. See [Keeping the Cost Off the Robot](#keeping-the-cost-off-the-robot). Reading and discarding pays the full decode and deserialization cost of every frame.

## Related Documentation

- [RealSense Cameras](CAMERA.md): the three USB depth cameras on the NOS host, their ROS 2 driver, and the librealsense interfaces.
- [Real-Robot Deployment and Control](REAL_ROBOT.md): host addresses and network access.
- [Developer Modes](DEVELOPER_MODE.md): mode switching. The relationship between Developer Mode and the RTSP service was not established during the measurements above; the stream was read without changing modes, but this was not tested systematically.
