# DEEPRobotics SDK

**English** | [简体中文](README_CN.md)

DEEPRobotics SDK provides ROS 2 packages and Topic examples for supported DEEPRobotics products. Product-specific deployment, runtime modes, state machines, and examples are documented in the corresponding product directories.

## Products

| Product | ROS 2 Package | Product Documentation |
| --- | --- | --- |
| DR02 Pro | `dr02_pro` | [English](src/dr02_pro/README.md) / [中文](src/dr02_pro/README_CN.md) |
| DR02 Std | `dr02_std` | [English](src/dr02_std/README.md) / [中文](src/dr02_std/README_CN.md) |

## Repository Layout

```text
scripts/          Dependency installation helpers
src/common/       Shared SDK utilities and dependencies
src/<product>/    Product-specific ROS 2 packages, deployment instructions, and user documentation
third_party/      Third-party source and integration files
```

## Robot Hosts

| Host | Address | Ubuntu | ROS 2 | Architecture | Internet |
| --- | --- | --- | --- | --- | --- |
| AOS | `10.21.33.103` | 24.04 | Jazzy | ARM64 (`arm64`) | None |
| NOS (DR02 Pro) | `10.21.33.106` | 22.04 | Humble | ARM64 (`arm64`) | None |

Download sources on an internet-connected development computer and transfer them
through the robot's local network. Build natively on the target host for its ROS
version. See the product's real-robot guide for the transfer and SDK build commands.

## Install the Message Interfaces

The script checks dependencies, downloads [deep-robotics-msg](https://github.com/DeepRoboticsLab/deep-robotics-msg.git),
and builds and installs its Debian package. Missing dependencies are reported;
the script does not install them. ROS 2, a compiler, CMake/CPack, colcon, and the
required ROS message-generation packages must already be present.

On an internet-connected development host, run from the SDK root:

```bash
./scripts/install_deep_robotics_msg.sh
```

On the AOS, use the source already transferred from your development computer:

```bash
cd ~/deep-robotics-sdk2
./scripts/install_deep_robotics_msg.sh --source-dir ~/deep-robotics-msg
source /opt/ros/jazzy/setup.bash
```

`--source-dir` skips downloading and requires no internet connection. The script
never runs apt. It builds a temporary copy, preserves the supplied source, and
removes temporary files when finished. Sudo is used only to install the built package.

Ubuntu 24.04 selects Jazzy; Ubuntu 22.04 selects Humble. Both amd64 and arm64 are
supported. The NOS already has `drdds` installed; source `/opt/ros/humble/setup.bash`
there. After installation, each new terminal needs only the matching ROS setup;
no separate message workspace is required.

Optional: `--ref <tag-or-commit>` selects the downloaded revision (default: `main`);
`--jobs N` sets compiler parallelism (default: 2). `--ref` cannot be combined with
`--source-dir`. See `--help` for usage.

## Related Repositories

- [deep-robotics-msg](https://github.com/DeepRoboticsLab/deep-robotics-msg.git): ROS 2 message interface package used by the SDK. Its ROS 2 package name is `drdds`.
- [deep-robotics-simulation](https://github.com/DeepRoboticsLab/deep-robotics-simulation): MuJoCo simulation environments for DEEPRobotics products.
