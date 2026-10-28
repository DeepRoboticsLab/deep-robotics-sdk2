# DR02 Pro 开发者模式

[返回 DR02 Pro SDK 使用指南](../README_CN.md)

本文档说明 DR02 Pro 开发者模式的用途、三种开发模式，以及通过手柄或 SDK 示例切换模式的方法。开发者模式仅用于实机控制。

<a id="developer-mode"></a>

## 1. 模式说明

开发者模式用于向 SDK 开放机器人的运动控制权限，包含高层、全身和上肢三种开发模式。三种模式的控制范围不同，运行程序前应先确认目标程序所需的模式。

| Mode | 名称 | 控制层级 | 控制方式 | 适用示例 |
| --- | --- | --- | --- | --- |
| 0 | 高层开发 | High-Level | 使用`/MOTION_STATE`、`/GAIT`、`/STEER`等高层接口，关节由内部策略控制 | [高层示例](EXAMPLES_CN.md#2-高层示例) |
| 1 | 上肢开发 | Low-Level Joint | SDK通过`/JOINTS_CMD`控制腰部和双臂，腿部可通过手柄或者`/STEER`下发移动指令 | [低层示例](EXAMPLES_CN.md#1-低层示例) |
| 2 | 全身开发 | Low-Level Joint | SDK通过`/JOINTS_CMD`直接控制全身关节 | [状态机](STATE_MACHINE_CN.md)、[低层示例](EXAMPLES_CN.md#1-低层示例) |

### 1.1 模式切换要求

| 模式 | 进入条件 | 按流程退出 | 快速阻尼退出 |
| --- | --- | --- | --- |
| 高层开发 | 进入开发者模式后的默认模式 | 非 `Idle` 时切换 `JointDamping`，等待回到 `Idle` 后退出开发者模式；已处于 `Idle` 时直接退出 | 非 `Idle` 时切换 `JointDamping`，等待回到 `Idle` 后退出开发者模式 |
| 上肢开发 | 在高层开发模式下，运动状态为 `RLControl` | 返回高层开发模式，运动状态为 `RLControl`，再按高层开发模式退出 | 非 `Idle` 时直接切换 `JointDamping`，等待回到 `Idle` 后退出开发者模式 |
| 全身开发 | 在高层开发模式下，运动状态为 `Idle` | 返回高层开发模式，经阻尼回到 `Idle` 后退出开发者模式 | 非 `Idle` 时直接切换 `JointDamping`，等待回到 `Idle` 后退出开发者模式 |

> [!WARNING]
>
> 进入阻尼可能使机器人失去主动支撑，请提前做好防倒保护。

> [!NOTE]
>
> - 手柄和 SDK 示例均支持**按流程退出**和**快速阻尼退出**。
> - 上肢开发模式和全身开发模式不能直接互相切换；切换前应先停止 SDK 程序并返回高层开发模式。
> - 直接发布 `/JOINTS_CMD` 时，同一时间只允许一个发布者。

## 2. 切换方式

1. **使用手柄切换：** 通过手柄设置并进入开发者模式，适合按照机器人现场操作流程控制实机。
2. **使用 SDK 示例切换：** 运行 `developer_mode_example`，通过 `/DEVELOPER_MODE` 服务和相关状态 Topic 完成模式切换，适合在终端中切换流程。

两种方式相互独立，不应同时操作。

<a id="handle-switching"></a>

## 3. 使用手柄切换

在手柄设置界面-辅助功能-开发者模式设置中开启开发者模式。设置完成后，按照下面的流程进入对应模式。

### 3.1 高层开发模式

- 进入：
  1. 确认机器人处于空闲状态。
  2. 点击手柄主界面左上角模式切换按钮，切换至开发者模式。
  3. 切换后默认在高层开发模式。
- 按流程退出：
  1. 在运行 SDK 的终端中退出程序，确认程序及相关控制 Topic 发布已经停止。
  2. 点击阻尼并等待机器人切换回空闲状态。
  3. 通过主界面左上角模式切换按钮退出开发者模式。

### 3.2 全身开发模式

- 进入：
  1. 确认机器人处于空闲状态。
  2. 点击手柄主界面左上角模式切换按钮，切换至开发者模式。
  3. 点击全身开发，模式切换完成。
- 按流程退出：
  1. 在运行 SDK 的终端中退出程序，确认程序及相关控制 Topic 发布已经停止。
  2. 点击手柄主界面的退出开发，返回高层开发模式并等待机器人回到空闲状态。
  3. 通过主界面左上角模式切换按钮退出开发者模式。

### 3.3 上肢开发模式

- 进入：
  1. 确认机器人处于空闲状态。
  2. 通过手柄主界面左上角模式切换按钮切换至开发者模式。
  3. **将机器人吊起，确认双脚离开地面**，点击悬吊起立。
  4. **将机器人放下，确认双脚完全接触地面**，点击开始运动。
  5. 点击上肢开发，模式切换完成。
- 按流程退出：
  1. 在运行 SDK 的终端中退出程序，确认程序及相关控制 Topic 发布已经停止。
  2. 点击手柄主界面的退出开发，返回高层开发模式。
  3. 点击阻尼并等待机器人切换回空闲状态。
  4. 通过主界面左上角模式切换按钮退出开发者模式。

<a id="sdk-example-switching"></a>

## 4. 使用 SDK 切换

`developer_mode_example` 用于通过命令进入或退出实机开发者模式，是完整目标流程示例，会切换状态并在危险阶段提示用户确认。

> ⚠️ **运行 `upper_body` 时的安全确认**
>
> 上肢开发模式的前置状态是 `RLControl`。示例从 `Idle` 状态切换到 `RLControl` 时，必须按照示例中的提示操作：
>
> 1. 将机器人吊起，确认双脚离开地面。
> 2. 切换到 `SuspendedStand`。
> 3. 将机器人放下，确认双脚完全接触地面。
> 4. 确认姿态稳定后，切换到 `RLControl`。

```bash
ros2 run dr02_pro developer_mode_example high_level
ros2 run dr02_pro developer_mode_example whole_body
ros2 run dr02_pro developer_mode_example upper_body

ros2 run dr02_pro developer_mode_example exit
ros2 run dr02_pro developer_mode_example damping_exit
```

参数说明：

- `high_level`：按流程进入高层开发模式。
- `whole_body`：按流程进入全身开发模式。
- `upper_body`：按流程进入上肢开发模式。
- `exit`：按流程退出开发者模式。
- `damping_exit`：快速阻尼退出开发者模式。

### 4.1 服务接口

`/DEVELOPER_MODE` 用于进入或退出开发者模式，以及选择具体的开发模式。

| 项目 | 内容 |
| --- | --- |
| 服务名称 | `/DEVELOPER_MODE` |
| 服务类型 | `drdds/srv/StdSrvString` |
| 请求字段 | `command`，JSON 字符串 |
| 响应字段 | `result`，`success` 表示成功，其他值表示未完成或被拒绝 |

`command` 字符串使用以下 JSON 格式：

| Action | 请求示例 | 作用 |
| --- | --- | --- |
| `enable` | `{"Action":"enable"}` | 进入开发者模式，成功后为`Enabled=true, Mode=0` |
| `set_mode` | `{ "Action": "set_mode", "Mode": 1, "Frequency": 500 }` | 选择开发模式；Mode 1 和 Mode 2 的 `Frequency` 范围为 `1` 到 `500`，Mode 0 忽略该字段 |
| `disable` | `{"Action":"disable"}` | 在Mode 0且`Idle`时退出开发者模式 |

`set_mode` 的 `Mode` 值如下：

| `Mode` | SDK 名称 | 开发模式 |
| --- | --- | --- |
| `0` | `HighLevel` | 高层开发模式 |
| `1` | `UpperBody` | 上肢开发模式 |
| `2` | `WholeBody` | 全身开发模式 |

<br>

`/DEVELOPER_MODE_STATUS` 用于确认当前状态，类型为 `std_msgs/msg/String`，`data` 字段是 JSON 字符串。例如：

```json
{"Enabled":true,"Mode":2,"Frequency":500}
```

`Enabled` 表示开发者模式是否开启，`Mode` 表示当前开发模式，`Frequency` 表示当前关节控制数据频率。高层开发模式不包含 `Frequency` 字段。示例通过该状态判断服务操作是否生效。

## 5. 紧急停止

发生异常或需要立即停止控制时，应优先按下手柄红色停止按钮。机器人状态恢复稳定后，应确认 SDK 程序和控制 Topic 已停止，再重新切换开发模式。
