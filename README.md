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

## Install the Message Interfaces

With ROS 2 and its apt repository already configured, run from the SDK root:

```bash
./scripts/install_deep_robotics_msg.sh
```

The script selects Humble on Ubuntu 22.04 or Jazzy on Ubuntu 24.04 and supports
native x86-64 (`amd64`) and 64-bit ARM (`arm64`) builds. It installs build
dependencies with apt, downloads [deep-robotics-msg](https://github.com/DeepRoboticsLab/deep-robotics-msg.git),
builds and installs its Debian package into `/opt/ros/<distro>`, and verifies the
`drdds` interfaces. It uses sudo for system installation. Temporary downloads
and build files are removed on success or failure; shell startup files are not edited.

```bash
source /opt/ros/humble/setup.bash  # Ubuntu 22.04; use jazzy on Ubuntu 24.04
```

Run this source command in each terminal before following the product build/run
instructions. No separate message workspace needs to be sourced. Hosts with
`drdds` already preinstalled (such as NOS) can skip this step. Running the script
again rebuilds and reinstalls the selected upstream revision.

The default source is upstream `main`. Use `--ref <tag-or-full-commit-SHA>` for a
repeatable version and `--jobs N` to adjust compiler parallelism (default: 2, to
limit memory use on ARM hosts). See `--help` for all options. ROS 2 itself is not
installed by this script.

## Related Repositories

- [deep-robotics-msg](https://github.com/DeepRoboticsLab/deep-robotics-msg.git): ROS 2 message interface package used by the SDK. Its ROS 2 package name is `drdds`.
- [deep-robotics-simulation](https://github.com/DeepRoboticsLab/deep-robotics-simulation): MuJoCo simulation environments for DEEPRobotics products.
