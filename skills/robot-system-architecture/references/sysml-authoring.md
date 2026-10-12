# Architecture.sysml 编写规则

这些规则面向当前 AIDC Parser 与浏览器支持的 SysML v2 子集。优先保持模型简洁、可解析和可追溯。

## 文件结构

- 一个文件、一个根 Package。
- 根 Package 中放端口定义、可复用部件定义、顶层系统定义、计算定义和需要的 View 定义。
- 顶层系统定义中放当前配置的 PartUsage、端口、连接、原始参数和分析调用。
- 标准类型来自 SysML 官方标准库；不要在模型中复制 `String`、`ISQ::mass` 等定义。

```sysml
package RobotArchitecture {
    port def ElectricalPowerPort;

    part def RobotSystem {
        attribute componentId :> String = "SYS-ROBOT";
        part powerSystem {
            attribute componentId :> String = "SYS-POWER";
            attribute responsibility :> String = "储能、配电和保护";
        }
    }
}
```

## 最小业务字段

| 对象 | 默认内容 | 按需增加 |
|---|---|---|
| 系统或部件 | `componentId`、职责、父子组成 | 数量、原始质量/功率、需求 ID、CAD 引用 |
| 端口 | 名称、接口类别、所属部件 | 电压、电流、协议、速率、机械基准 |
| 连接 | 明确的两个端点 | 线束、总线、载荷或安全说明 |
| 参数分析 | 原始输入、`calc def`、绑定、输出、单位 | 目标约束、来源状态和 Table View |
| 装配交接 | 父装配、`fixed`/`moving`、`cadRef` | 轴线、研究限位、包络和维护要求 |

状态、负责人、审批流和进度不是默认字段。

## 稳定 ID

`componentId` 是跨 SysML、CAD、BOM 和验证的业务身份，不能使用 Parser 的快照元素 ID 代替。ID 在当前配置中唯一；显示名称变化时保持不变；左右件明确区分；已删除的 ID 不转给其他部件。

推荐形式：整机 `SYS-<型号>`，装配 `ASM-<名称>`，执行器 `ACT-<位置>`，功能系统 `SYS-<职责>`。

## 物理量和来源

物理量使用标准类型并显式携带单位：

```sysml
attribute planningMass :> ISQ::mass = 225[g];
attribute batteryVoltage :> ISQ::electricPotential = 6.4[V];
attribute controlPeriod :> ISQ::duration = 5[ms];
```

禁止用 `massKg`、`voltageV` 等名字代替量纲，也禁止给物理量赋裸数字。未知值直接不声明赋值；不要使用 `0`、空字符串或示例值冒充未知值。来源状态可用紧邻的 `doc` 或清楚命名的 evidence 属性说明，并在项目来源登记中给出资料位置。

## 端口和连接

端口名称表达职责，端口定义表达接口类别。连接必须写出存在于同一结构上下文中的两个端点：

```sysml
connect powerSystem.logicPowerOut to computeSystem.powerIn;
connect motionController.servoBus to leftLeg.controlIn;
```

不要通过相似名称推断端点，不创建没有业务交换的装饰性连接。

## 参数计算

原始输入属于真实部件；公式属于 `calc def`；当前配置的调用负责实际参数绑定；约束比较分析结果和需求目标。

```sysml
calc def ComputeTotalMass {
    in structureMass :> ISQ::mass;
    in equipmentMass :> ISQ::mass;
    return totalMass :> ISQ::mass = structureMass + equipmentMass;
}
```

- 同一结果只能有一个事实来源，不能同时手填派生值。
- 缺失输入使结果不可计算，不能按零处理。
- 比较表达式两侧量纲必须兼容。
- 单位换算由单位感知计算内核处理，不在字段名或注释中模拟。
- 只为方案决策所需的核心参数建计算链。

## 装配交接

使用 `assemblyRelation` 表示初步固定或运动关系，使用 `cadRef` 指向计划中的 CAD 文件或对象。`cadRef` 只证明建立了关联，不证明几何真实、无干涉或已经装配验证。

运动对象至少记录 `jointAxis`、带单位的研究限位和稳定 ID。轴线上一点、安装面、花键/轴端、对侧支承、线缆出口、实际包络和拆装方向应由 CAD 或来源资料承载，不编码成大量字符串坐标。

## 当前兼容边界

优先使用已验证的 `package`、`part def`、嵌套/带类型 `part`、`port def`、`port`、带单位属性、`connect a.port to b.port`、`calc def`、分析调用、约束以及项目已验证的 Table View 写法。

跨文件类型解析、复杂多端连接和尚未验证的 SysML 关系语法需要先确认 Runtime 能力。Runtime 不支持时保留业务意图和明确诊断，不用字符串拼接伪造已解析关系。

