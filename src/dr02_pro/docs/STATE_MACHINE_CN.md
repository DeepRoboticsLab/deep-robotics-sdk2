# DR02 Pro 状态机

[返回 DR02 Pro SDK 使用指南](../README_CN.md)

DR02 Pro 状态机订阅 `/JOINTS_DATA`，发布 `/JOINTS_CMD`，AMP 策略使用 `/STEER` 和 `/REAL_STEER` 的方向命令，Mimic 策略执行参考动作。

## 1. 运行方式

### 1.1 实机控制

实机运行前，请先完成[实机部署](REAL_ROBOT_CN.md)，并按照[开发者模式文档](DEVELOPER_MODE_CN.md#handle-switching)进入全身关节控制模式。

> [!IMPORTANT]
>
> - 当前状态机代码默认仅支持全身关节控制模式。
> - 如需在上半身关节控制模式下运行，需根据实际控制需求自行修改状态机代码。
> - 修改后建议先在仿真环境中验证，再部署到实机。

> [!WARNING]
>
> 状态机运行期间，不得同时运行其他 `/JOINTS_CMD` 发布者。

### 1.2 仿真运行

状态机支持配合 MuJoCo 仿真运行。SDK 侧编译和运行方式请参阅[仿真环境与运行](SIMULATION_CN.md)。

## 2. 启动状态机

```bash
source install/setup.bash
ros2 run dr02_pro state_machine
```

## 3. 状态流转

```text
Idle -> ZeroPos -> StandUp -> RLControlAMP
                          -> RLControlMimic
RLControlAMP <-> RLControlMimic
```

- `Idle`：保持初始命令，等待用户请求。
- `ZeroPos`：控制机器人回到零位。
- `StandUp`：从零位请求策略控制后，进行约 3 秒的姿态过渡，再进入请求的策略。
- `RLControlAMP`：运行 AMP 策略并处理方向命令。
- `RLControlMimic`：运行 Mimic 策略和参考动作，动作完成后自动返回 AMP。
- `JointDamping`：用户请求停止或安全处理时进入的阻尼状态。

AMP 与 Mimic 之间的手动切换直接进行，不经过 `StandUp`；切换前代码会检查方向命令、
机身姿态和关节速度。不满足条件时保留当前策略。Mimic 动作完成后的自动返回是单独的路径。
完整的策略准备与部署顺序见 [README 工作流程](../README_CN.md#52-部署-amp-策略)。

## 4. 键盘控制

- `z`：从 `Idle` 进入 `ZeroPos`
- `c`：从 `ZeroPos` 经 `StandUp` 进入 AMP；从 Mimic 请求切换到 AMP
- `v`：从 `ZeroPos` 经 `StandUp` 进入 Mimic；从 AMP 请求切换到 Mimic
- `r`：请求 `JointDamping`
- `w/s`：AMP 控制下前进 / 后退
- `a/d`：AMP 控制下左移 / 右移
- `q/e`：AMP 控制下左转 / 右转

## 5. 手柄控制

- L1：从 `Idle` 进入 `ZeroPos`
- L2：从 `ZeroPos` 经 `StandUp` 进入 AMP
- R1：从 AMP 请求切换到 Mimic
- R2：请求 `JointDamping`
- 左摇杆：AMP 的前后、左右移动命令
- 右摇杆：AMP 的转向命令

实机开发者模式切换和 SDK 状态机控制是两套操作；先按
[开发者模式](DEVELOPER_MODE_CN.md)完成实机模式准备，再使用上述状态机按键。

## 6. 安全要求

> [!WARNING]
>
> - 实机运行状态机前，应确认机器人已进入正确的开发者模式。
> - 进入 `ZeroPos`、`StandUp` 或策略控制前，应确认机器人状态和运动空间安全。
> - 实机请求 AMP 或 Mimic（包括进入 `StandUp`）前，必须先将机器人放下并确认双脚稳定接触地面。吊绳可以保留用于保护，但不得使机器人处于悬空状态。悬空切换可能产生突然动作或失稳，造成人身伤害或设备损坏。
> - `/JOINTS_CMD` 同一时间只允许一个发布者。
> - 发生异常时，应优先使用手柄红色停止按钮退出控制。
