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
src/common/       Shared SDK utilities and dependencies
src/<product>/    Product-specific ROS 2 packages, deployment instructions, and user documentation
third_party/      Third-party source and integration files
```

## Working with Coding Agents

[`AGENTS.md`](AGENTS.md) ([中文](AGENTS_CN.md)) collects the rules a coding agent should follow when writing code that drives a DEEPRobotics robot: which Developer Mode a task needs, how joint commands and gains are structured per product, and the interface behaviours that fail without producing an error. It is organised as a common section followed by one section per product.

## Related Repositories

- [deep-robotics-msg](https://github.com/DeepRoboticsLab/deep-robotics-msg): ROS 2 message interface package used by the SDK. Its ROS 2 package name is `drdds`.
- [deep-robotics-simulation](https://github.com/DeepRoboticsLab/deep-robotics-simulation): MuJoCo simulation environments for DEEPRobotics products.
