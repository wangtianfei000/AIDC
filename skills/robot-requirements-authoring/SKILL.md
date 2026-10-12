---
name: robot-requirements-authoring
description: 根据机器人的用途、使用场景和关键指标，创建或修改简洁的 SysML v2 requirements.sysml。适用于把“续航不少于两小时”等机器人业务要求写成可浏览、可量化的需求；不用于架构设计和产品实现。
---

# 机器人需求文件编写

创建一个简洁的 `requirements.sysml`，直接表达机器人需要实现什么。优先让产品、研发和测试人员一眼看懂业务要求，不为流程管理堆积字段。

开始前阅读[简明编写指南](references/authoring-guide.md)。需要起点时复制[机器人需求示例](assets/robot-requirements-example.sysml)，再根据实际机器人修改。

## 先理解机器人业务

从用户提供的资料中确定：

- 机器人是什么类型，给谁使用；
- 机器人要完成哪些核心任务；
- 在什么环境和条件下工作；
- 用户最关心哪些指标，例如续航、速度、负载、精度、停止时间、尺寸和质量；
- 故障或异常时必须出现什么可观察结果。

信息不足时，可以先形成草稿并把未知门槛写成“待确定”，不要猜测安全阈值或批准指标。

## 只创建一个需求文件

- 只创建或修改 `requirements.sysml`。
- 使用 Package 按业务分类，例如功能、性能、安全、机械和维护。
- 当前 AIDC Parser 下，每条业务需求使用一个 `requirement def`。
- 每条需求默认只写两个字段：稳定 ID `reqId` 和业务正文 `doc`。
- 只有需求包含明确数值时，才增加“测量值、带单位的门槛值、约束”。数值必须使用 SysML 单位表达式，例如 `2[h]`、`500[ms]`、`0.5[m/s]`、`2[kg]`。
- 除非用户明确要求，不添加状态、专业、变体、责任人、来源、理由、验证方法、验证用例或待办管理字段。

## 写成业务人员能读懂的需求

每条需求只表达一个结果。推荐句式：

> 在【使用条件】下，机器人应【完成的行为或达到的指标】。

例如：

> 在电池充满、室温和额定负载条件下，机器人连续执行典型任务的续航时间应不少于 2 小时。

对应的 SysML：

```sysml
requirement def batteryEndurance {
    attribute reqId :> String = "REQ-PER-001";
    doc /*在电池充满、室温和额定负载条件下，机器人连续执行典型任务的续航时间应不少于 2 小时。*/
    attribute measuredEndurance :> ISQ::duration;
    attribute minimumEndurance :> ISQ::duration = 2[h];
    require constraint {measuredEndurance >= minimumEndurance};
}
```

约束中的符号必须先声明，测量值和门槛值应使用相同的物理量类型。门槛值必须显式携带单位，不能只写 `2.0`、`500` 等无量纲数字，也不能只依赖变量名暗示单位。

## 完成前检查

确认：

- 每条需求都有唯一的 `reqId` 和清楚的 `doc`；
- 一条需求只包含一个可独立判断的结果；
- “快、稳定、足够、安全”等模糊说法已经转成明确指标，或明确写为待确定；
- 数值包含使用条件、比较方向和显式 SysML 单位；
- 示例值没有被当成项目已经批准的真实指标；
- 文件只有一个根 Package，并能由 AIDC SysML 浏览器打开。

保存后调用 `aidc_sysml_validate`；需要人工审查时调用 `aidc_sysml_open`。没有 MCP 的 CI 环境使用 `aidc sysml validate <requirements.sysml>`。解析失败时保留原诊断，不通过修改浏览器或跳过 Parser 声称完成。

