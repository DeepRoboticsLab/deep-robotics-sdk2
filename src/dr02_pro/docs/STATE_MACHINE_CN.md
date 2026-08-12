# DR02 Pro 状态机

[返回 DR02 Pro SDK 使用指南](../README_CN.md)

DR02 Pro 状态机订阅 `/JOINTS_DATA`，发布 `/JOINTS_CMD`，并在 RL 控制状态下接收 `/STEER` 和 `/REAL_STEER`。

## 运行方式

### 实机控制

实机运行前，请先完成[实机部署](REAL_ROBOT_CN.md)并进入正确的[开发者模式](REAL_ROBOT_CN.md#developer-mode)。

> [!IMPORTANT]
>
> - 当前状态机代码默认仅支持全身关节控制模式。
> - 如需在上半身关节控制模式下运行，需根据实际控制需求自行修改状态机代码。
> - 修改后建议先在仿真环境中验证，再部署到实机。

> [!WARNING]
>
> 状态机运行期间，不得同时运行其他 `/JOINTS_CMD` 发布者。

### 仿真运行

状态机支持配合 MuJoCo 仿真运行。SDK 侧编译和运行方式请参阅[仿真环境与运行](SIMULATION_CN.md)。

## 启动状态机

```bash
source install/setup.bash
ros2 run dr02_pro state_machine
```

## 状态流转

```text
Idle -> ZeroPos -> RLControl
```

- `Idle`：保持初始命令，等待用户请求。
- `ZeroPos`：控制机器人回到零位。
- `RLControl`：运行 RL policy，并接收 `/STEER` 和 `/REAL_STEER`。
- `JointDamping`：RL 控制和安全处理使用的阻尼状态。

## 键盘控制

- `z`：进入 `ZeroPos`
- `c`：从 `ZeroPos` 进入 `RLControlAMP`；在 `RLControlMimic` 下切换到 AMP
- `v`：从 `ZeroPos` 进入 `RLControlMimic`；在 `RLControlAMP` 下切换到 Mimic
- `r`：进入 `JointDamping`
- `w/s`：AMP 控制下前进 / 后退
- `a/d`：AMP 控制下左移 / 右移
- `q/e`：AMP 控制下左转 / 右转

## 手柄控制

- L1：进入 `ZeroPos`
- L2：从 `ZeroPos` 进入 `RLControlAMP`
- R1：从 `ZeroPos` 进入 `RLControlMimic`
- R2：进入 `JointDamping`
- 左摇杆：前进 / 左移 / 后退 / 右移命令
- 右摇杆：转向命令

## 安全要求

> [!WARNING]
>
> - 实机运行状态机前，应确认机器人已进入正确的开发者模式。
> - 进入 `ZeroPos` 或 `RLControl` 前，应确认机器人状态和运动空间安全。
> - 实机切换至 `RLControl` 前，必须先将机器人放下并确认双脚稳定接触地面。吊绳可以保留用于保护，但不得使机器人处于悬空状态。悬空切换可能产生突然动作或失稳，造成人身伤害或设备损坏。
> - `/JOINTS_CMD` 同一时间只允许一个发布者。
> - 发生异常时，应优先使用手柄红色停止按钮退出控制。
