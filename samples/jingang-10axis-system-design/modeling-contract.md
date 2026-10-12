# 金刚原生十轴架构建模契约 V0

本契约冻结 P0 样例中 SysML、浏览器视图和初步 CAD 装配之间的最小数据边界。它只约束本项目首版，不宣称覆盖完整 SysML v2。

## 1. 文件边界

- 需求事实源：`requirements.sysml`。
- 架构事实源：`architecture.sysml`。
- 空间和装配事实源：P4 建立的 `layout/jingang-10axis-layout.FCStd`。
- Parser 输出和浏览器视图都是派生数据，不是新的事实源。
- 首版不要求跨文件类型解析；需求到架构使用稳定需求 ID。

## 2. 根结构

`architecture.sysml` 只能有一个根 Package。根 Package 内包含：

1. 首版使用的 PortDefinition；
2. 可复用的关键部件 PartDefinition；
3. 一个代表目标机器人的顶层 PartDefinition；
4. 顶层 PartDefinition 下的实际装配层级、端口、属性和连接。

当前根对象固定为：

```text
JingangNative10Architecture
└── JingangNative10
    ├── bodyFrame
    ├── leftLeg
    │   └── 4 个执行器
    ├── rightLeg
    │   └── 4 个执行器
    ├── headNeck
    │   └── 2 个执行器
    ├── powerSystem
    ├── computeSystem
    ├── motionController
    └── sensingSystem
```

该层级表达本轮装配责任，不按学科建立另一棵树。

## 3. 稳定业务 ID

Parser 生成的 Element ID 用于当前快照导航；`componentId` 用于跨 SysML、CAD、BOM 和后续验证保持业务身份。

| 对象 | 格式 | 示例 |
|---|---|---|
| 整机 | `SYS-<型号>` | `SYS-JG10` |
| 装配 | `ASM-<名称>` | `ASM-LEG-L` |
| 执行器/运动件 | `ACT-<位置>` | `ACT-L-KNEE-PITCH` |
| 其他系统 | `SYS-<职责>` | `SYS-POWER` |

规则：

- `componentId` 在同一机器人配置中必须唯一；
- 重命名显示名称不能自动改变 `componentId`；
- SysML PartUsage 与 CAD 对象使用相同 `componentId`；
- 删除后不把旧 ID 重新分配给不同部件；
- 左右件必须在 ID 中明确区分。

## 4. 部件字段

### 4.1 所有主要系统和装配

| 字段 | 必填 | 业务含义 | 浏览用途 |
|---|---|---|---|
| `componentId` | 是 | 跨工具稳定身份 | 部件表、CAD 关联 |
| `responsibility` | 是 | 部件为什么存在 | 部件表、属性查看 |
| `assemblyRelation` | 装配件必填 | `fixed` 或 `moving` | 装配交接表 |
| `cadRef` | 布局对象必填 | CAD 文件或对象引用 | 装配交接表、外部跳转 |
| `requirementIds` | 按需 | 负责的需求 ID，逗号分隔 | 需求责任视图 |

`requirementIds` 是 Parser 过渡写法。正式 `satisfy` / `allocate` 关系只有在 Runtime 能可靠解析和跨文件定位后才能替换它。

### 4.2 定量工程属性

- 只在已有数据或明确预算时声明；
- 数值必须带 SysML 单位；
- 字段名不承担单位含义；
- 缺失值直接不写，浏览器显示“—”或“不完整”；
- 目标值、估算值和实测值不能共用一个字段；
- 数据来源和可信状态登记在 `evidence/source-register.md`。

示例：

```sysml
attribute maximumMass :> ISQ::mass = 800[g];
attribute actuatorGroupPlanningMass :> ISQ::mass = 225[g];
attribute minimumEndurance :> ISQ::duration = 2[h];
```

### 4.3 采购件和占位体

| 字段 | 用途 |
|---|---|
| `componentType` | 已指定的型号或类别 |
| `planningMass` | 用于布局的质量，必须在来源登记中说明性质 |
| `envelopeStatus` | 说明几何是已核实、公开外廓、设计预留还是待验证 |

不得仅因为具有 `cadRef` 就把占位体描述为已核实 CAD。

## 5. 关节和运动装配字段

运动部件至少表达：

| 字段 | 说明 |
|---|---|
| `componentId` | 关节执行器或运动模块的稳定 ID |
| `jointAxis` | 当前装配坐标下的轴向说明 |
| `lowerLimit` | 初始研究下限，带角度单位 |
| `upperLimit` | 初始研究上限，带角度单位 |

P0 的角度范围是研究窗口。机械安全限位、软件限位和实物允许范围在未验证前不得合并成一个“最终限位”。

初步 CAD 中还必须补充 SysML 纯文本难以可靠表达的几何信息：轴线上一点、局部坐标系、驱动端、支承端、安装面、实际包络和拆装方向。

## 6. 端口和连接

首版接口类别：

- `ElectricalPowerPort`：电源与回流；
- `ServoBusPort`：执行器命令与反馈总线；
- `MotionCommandPort`：上层运动命令；
- `ImuDataPort`：IMU 数据。

每条 Connection 必须：

1. 写出明确的两个端点；
2. 两端处于同一顶层结构上下文；
3. 不用名称猜测不存在的端口；
4. Parser 无法解析端口实例时保留原始 `ends` 文本并产生诊断。

P0 Runtime 只能把 `part.port` 归一化到直接 PartUsage。P1 必须在 ModelSnapshot 中提供真实端口端点，接口表不能长期依赖解析 `ends` 字符串。

## 7. 参数计算与分析

P0 使用固定字段记录预算的规则已由 C0 计算契约替代。后续正式模型应遵循：

- 原始参数记录在真实所属部件；
- 计算公式使用 `calc def`；
- 当前配置的实际参数绑定、输出和约束记录在 Analysis；
- 计算结果由 Runtime 生成，不与手填 `current*` 字段并存；
- 物理量带标准单位，缺失值不按零计算；
- 合成假设必须在来源登记中标明，不能显示成实测值；
- 参数分析表由显式 SysML View 声明，行来自 `expose`，列顺序来自 `columnView`。

C0 首版覆盖：

| 分析 | 输出 | 约束方向 |
|---|---|---|
| 整机质量 | `totalMassResult` | 不大于质量上限 |
| 典型工况续航 | `enduranceResult` | 不小于续航下限 |
| 峰值供电 | `peakElectricalPowerResult` | 不大于供电上限 |
| 停止响应 | `stopResponseTimeResult` | 不大于停止时间上限 |

`architecture.sysml` 已在 C3 集成四个 CalculationDefinition、当前配置 Analysis 和 `GridView + asElementTable`。`currentPlannedMass`、`batteryEnergy`、`maximumAveragePowerForEndurance`、`peakElectricalPowerLimit` 等手填派生字段已经删除。C0 文件继续作为最小 Parser/Runtime 回归契约，不再是金刚当前方案事实源。

## 8. CAD 交接

P0 使用两类引用：

- 顶层 `cadRef = "layout/jingang-10axis-layout.FCStd"` 指向计划中的布局文件；
- 部件 `cadRef = "layout://<componentId>"` 指向该文件内的对象 ID。

P4 建立 CAD 后必须验证：

1. 每个 `layout://` ID 在 CAD 中唯一存在；
2. CAD 装配层级与 SysML 父子关系一致；
3. 固定和运动关系一致；
4. 十个执行器的关节归属一致；
5. 质量和包络的来源状态可查询；
6. 缺少真实接口的部件继续标识为占位或待验证。

## 9. Parser 兼容写法

P0 只使用当前实测可以建立元素的语法：

- `package`；
- `part def`、嵌套 `part`、typed part；
- `port def`、`port`；
- `attribute ... = value[unit]`；
- `connect a.port to b.port`。

暂不在规范样例中使用：

- 跨文件 import 后的类型解析；
- `satisfy`、`allocate`；
- 多端连接；
- 依赖类型继承自动展开装配实例；
- 将 CAD 几何坐标编码成大量字符串属性。

## 10. P0 完成后的冻结项

P1 可以调整 Parser/IR 的实现，但不得在没有业务评审的情况下改变：

- 目标十轴配置；
- 顶层装配层级；
- 稳定 `componentId`；
- 预算字段的业务含义；
- SysML 与 CAD 的职责边界；
- “缺失数据不推测”的原则。

