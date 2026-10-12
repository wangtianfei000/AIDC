# 机器人需求简明编写指南

目标是让 AI 把机器人业务要求写成一个简单、可读的 `requirements.sysml`。需求文件描述“机器人必须做到什么”，不记录项目管理流程。

## 1. 先问业务问题

编写前只需弄清几件事：

1. 这是什么机器人，谁在什么环境中使用？
2. 它必须完成哪些核心任务？
3. 用户最关心哪些结果和指标？
4. 哪些异常情况必须处理？
5. 什么结果代表满足需求？

典型指标包括续航时间、运动速度、负载、定位精度、重复精度、停止时间、通信距离、噪声、质量、尺寸、工作温度和维护时间。只选择与当前产品有关的指标。

## 2. 文件和分类

一个机器人项目只维护一个 `requirements.sysml`。使用 Package 帮助浏览，不需要为每个分类建立单独文件：

```text
RobotRequirements
├── FunctionalRequirements     核心功能
├── PerformanceRequirements    续航、速度、负载、精度
├── SafetyRequirements         停止、限位、异常响应
├── PhysicalRequirements       质量、尺寸、机械边界
└── ServiceRequirements        充电、维护和更换
```

没有需求的分类可以省略。

## 3. 每条需求的最小内容

默认只保留：

| 内容 | SysML 写法 | 例子 |
|---|---|---|
| 需求 ID | `attribute reqId` | `REQ-PER-001` |
| 业务正文 | `doc /*...*/` | 电池续航时间应不少于 2 小时 |
| 数值判据 | 物理量属性和 `require constraint` | `measuredEndurance >= 2[h]` |

没有明确数值的功能需求只需要 ID 和正文。只有门槛已经明确时才增加数值判据。

状态、专业、责任人、来源、理由、场景编号、验证方法和待办项不是默认字段。用户明确需要这些管理信息时再增加。

## 4. 写法示例

### 功能需求

```sysml
requirement def remoteControlWalking {
    attribute reqId :> String = "REQ-FUN-001";
    doc /*机器人应根据控制端发出的前进、后退、左转、右转和停止命令执行对应运动。*/
}
```

### 续航需求

```sysml
requirement def batteryEndurance {
    attribute reqId :> String = "REQ-PER-001";
    doc /*在电池充满、室温和额定负载条件下，机器人连续执行典型任务的续航时间应不少于 2 小时。*/
    attribute measuredEndurance :> ISQ::duration;
    attribute minimumEndurance :> ISQ::duration = 2[h];
    require constraint {measuredEndurance >= minimumEndurance};
}
```

### 停止需求

```sysml
requirement def commandStopTime {
    attribute reqId :> String = "REQ-SAF-001";
    doc /*机器人正常行走时收到停止命令后，应在 500 ms 内停止主动运动。*/
    attribute measuredStopTime :> ISQ::duration;
    attribute maximumStopTime :> ISQ::duration = 500[ms];
    require constraint {measuredStopTime <= maximumStopTime};
}
```

以上数值仅用于说明写法。正式文件必须使用用户确认的项目指标。

## 5. 简单质量检查

- 一条需求只写一个业务结果。
- 正文说明条件、对象、行为和门槛。
- 数值使用显式 SysML 单位，例如 `2[h]`、`500[ms]`、`0.5[m/s]` 和 `2[kg]`，不能只在变量名或正文中写单位。
- 不把设计方案当成业务需求。例如用户只要求“续航不少于两小时”时，不要自行规定电芯型号或电池结构。
- 当前能力或一次测试结果不能自动变成正式需求。
- 未确定的指标直接写“门槛待确定”，不创建虚假的数值约束。

完整示例见 `../assets/robot-requirements-example.sysml`。

