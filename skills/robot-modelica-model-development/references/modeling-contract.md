# 建模契约

## 分层和所有权

| 内容 | 保存位置 | 规则 |
|---|---|---|
| 公共连接器、方程、基础组件 | `modelica-browser/libraries/AIDCRobotics/modelica/AIDCRobotics/` | 按语义版本维护，新增行为必须带一致性模型 |
| 厂商和型号参数 | `AIDCRobotics.DeviceData` | 只保存物理参数记录，不复制组件方程 |
| 整机拓扑和型号参数 | 机器人实例包的 `Robot` | 引用标准组件，通过稳定 ID 对应 Robot IR |
| 用户可调配置 | `UserOverrides`、`overrides/user-overrides.json` | 重新生成时必须保留 |
| 仿真入口 | `Experiment` | 明确激励、初值、停止时间和观测量 |
| 生成元数据 | `generation.lock.json`、`generated-structure.json`、诊断文件 | 用于追踪输入、生成器版本、结构和确定性 |

禁止在生成包内复制 `AIDCRobotics` 的公共方程。若多个型号需要同一行为，把行为上移到标准库；若只是数值不同，使用参数记录。

## 公共约定

- 使用 SI 单位；Modelica 变量应声明对应 `Modelica.Units.SI` 类型或明确 `unit`。
- 世界坐标使用右手系：`+X` 向前、`+Y` 向左、`+Z` 向上。
- 公共机器人/FMI 四元数顺序为 `w, x, y, z`；Model3D 交换格式为 `x, y, z, w`。跨边界时必须显式转换。
- Robot IR 的 `bodyId`、`jointId`、`deviceId` 和接口 ID 是稳定身份；显示名称不可替代稳定 ID。
- 公共数组使用明确的 1 基通道索引；通道顺序写入描述符或结构清单，不依赖声明顺序。
- 关节方向只能为 `+1` 或 `-1`；比例和零偏分别建模，不能混入方向符号。
- `DiagnosticOutput.valid=false` 时，相应状态不得用于控制或验收。
- 惯量必须说明参考坐标系和质心位置，并在导入时检查对称性、正定性、量纲和三角不等式。

## 标准组件选择

| 目标 | 首选标准组件/接口 |
|---|---|
| 刚体和根基座 | `Mechanics.RigidBody3D`、`FixedBase3D`、`FloatingBase3D` |
| 固定、转动、移动关节 | `FixedJoint3D`、`RevoluteJoint3D`、`PrismaticJoint3D` |
| 摩擦、限位和回差 | `RotationalJointLoss`、`RotationalBacklash` |
| 扭矩轴和理想驱动 | `Mechanics.StandardTorqueAxis`、`Actuators.IdealTorque` |
| 位置舵机和直流电机 | `LimitedPositionServoAxis`、`LimitedDcMotor` |
| 供电 | `Electrical.SimpleBatteryBus` |
| 关节状态 | `Sensors.IdealJointSensor`、`Interfaces.JointStateOutput` |
| 命令和诊断 | `TorqueCommand`、`PositionCommand`、`DiagnosticOutput` |
| 解析接触 | `Contacts` 中的点、球、盒、胶囊和足底面片模型 |
| 网格查询 | `NativeGeometry` 稳定 C ABI，配合接触组件使用 |

先检查 `AIDCRobotics.Examples` 和 `Conformance`，避免创建功能重复的组件。

## Profile 和型号包

- 单关节台架：固定基座、单转动关节、扭矩或位置命令、位置/速度状态。
- 机械臂、云台：`ArticulatedRobot`，固定基座和有序关节命令/状态。
- 腿式机器人：`FloatingBaseRobot` 加树形刚体、关节驱动和足底接触；按需要组合电池与 IMU 能力。
- 网格接触：解析接触不能满足需求时才使用 `NativeGeometry`；保证目标平台具有匹配 ABI 的运行库。

推荐的生成包包含包级计数和稳定 ID 常量、`UserOverrides`、`Robot`、`Experiment`。`Robot` 暴露有单位的命令、状态、电源和必要诊断，不暴露内部求解细节。

## 新增标准组件的判定

按以下顺序判断：

1. 现有组件换参数即可表达：只新增 `DeviceData` 记录或型号配置。
2. 现有接口可表达但方程不同：新增实现并保持连接器兼容，同时添加 `Conformance` 模型。
3. 语义上出现新的物理域或公开数据契约：才新增公共连接器，并同时更新 API、兼容性、Profile 和一致性测试。
4. 只为单一型号服务且不会复用：保留在型号包中，不扩大标准库 API。

公共 API 的补丁版本不能移除已有类或连接器。次版本可以增加兼容组件和参数；破坏性变化需要新的主版本和显式迁移。

## 图形和连接视图

- 端口 Placement 必须落在组件边界上，方向与连接器语义一致。
- 连接线端点必须绑定实际端口坐标，不能通过视觉近似连接。
- 图形布局服务于阅读：根在左、叶在右，分支分层，电气、机械和信号连接使用稳定颜色。
- 模型语义来自组件和 `connect(...)`；图标、文字、线段和坐标注解均不得改变仿真结果。
