# 开发工作流

以下路径均相对于 `AIDC/`。开始前检查实际仓库脚本和 `--help`，不要凭旧文档猜参数。

## 1. 盘点输入与证据

记录：

- 输入路径、schema 和内容哈希。
- 固定或浮动根基座。
- 刚体树、关节类型/轴/限位/初值、质量/质心/惯量。
- 执行器、传动、摩擦、供电、传感器和接触。
- 命令、状态、诊断通道及其稳定 ID 和顺序。
- 参数来源：实测、厂家资料、CAD、估计或默认值。

完整机器人转换的正式输入是 `.aidc-assembly/`：唯一入口 `assembly.manifest.json`、根目录 `physics.json`、`devices.json`、`interfaces.json`，以及 `physics.json` 实际引用的 `geometry/` 文件。STL 只有几何，不足以表达关节、惯量来源、设备和接口。

## 2. 校验输入

```powershell
cd AIDC/modelica-browser
.\scripts\aidc-modelica.ps1 validate <robot.aidc-assembly-or-robot-ir>
```

遇到未知 schema 时先运行显式迁移流程。发现孤立刚体、环路、重复 ID、非法惯量、缺失父子关系或未解析几何引用时，输出字段级诊断并停止生成。

## 3. 选择实现方式

### 整机生成

```powershell
cd AIDC/modelica-browser
.\scripts\aidc-modelica.ps1 generate <robot.aidc-assembly> generated/<robot>
```

生成器必须产生最小 Modelica 工程：`model-project.json`、`modelica/<Package>/package.mo`、Robot IR 快照/结构清单、覆盖文件和生成诊断。根包引用兼容版本的 Modelica 标准库与 `AIDCRobotics`。

### 标准库组件开发

1. 在对应子包中实现最小公共行为。
2. 复用 `Interfaces` 和 Modelica 标准连接器。
3. 在 `Examples` 增加最小可执行用例。
4. 在 `Conformance` 增加正向和失效边界检查。
5. 更新标准库 API/兼容性文档和版本。

### 单型号调整

优先修改 `overrides/user-overrides.json` 或 `UserOverrides` 所映射的配置。对厂商设备参数新增 `DeviceData` 记录。不要直接修改生成的 `package.mo`，因为重新生成会覆盖它并破坏来源追踪。

## 4. 检查生成结构

逐项核对：

- `bodyCount`、`jointCount`、可动关节数和 ID 数组与 Robot IR 一致。
- 每个刚体只在树中出现一次，根基座类型正确。
- 固定变换、关节轴、方向、限位、初始姿态和命名姿态一致。
- 每个驱动器连接正确关节；损耗、供电限制、传感器和接触绑定到预期对象。
- 公共 I/O 数量、单位、顺序和诊断语义稳定。
- `Experiment` 可以在无界面模式下运行。

## 5. 编译和仿真

单工程校验：

```powershell
cd AIDC/modelica-browser
.\scripts\run-project.ps1 -Project <project> -ValidateOnly
.\scripts\run-project.ps1 -Project <project>
```

按模型能力选择场景：

- 关节/舵机：静态保持、位置阶跃、周期运动、负载变化、低压/限流。
- 多刚体：零输入稳定性、命名姿态、关节限位和能量/约束残差。
- 浮动腿式：重力下落、足底接触、摩擦和无穿透稳定性。
- 网格接触：缓存、BVH 查询、固定尺寸接触输出和 FMU 动态库打包。

固定求解器、容差、步长/输出间隔、停止时间和初值。保存关键时序、终值、极值、事件数和诊断，不只保留“成功”状态。

## 6. 回归和便携交付

```powershell
cd AIDC/modelica-browser
.\scripts\aidc-modelica.ps1 compare <run-a> <run-b> --abs-tol <value> --rel-tol <value>
.\scripts\aidc-modelica.ps1 package <project> dist/<robot>.aidcmpkg --include-results
```

对同一输入至少生成两次，确认源码、锁文件和结构清单稳定，用户覆盖未丢失。需要跨求解器或 FMI 时，使用相同输入和采样网格比较关键变量，并记录变量映射和容差。

## 7. 仓库级回归

根据改动范围运行最小充分集合：

```powershell
cd AIDC
npm run verify:modelica:p0-robotics
npm run verify:modelica:p1-robotics
npm run verify:modelica:p3-generator
npm run verify:modelica:p4-contact-runtime
npm run verify:modelica:p5-productization

cd modelica-browser
npm run verify:modelica:p6-closed-loop
npm run ci:modelica
```

公共接口、生成器或原生运行时变化需要相应阶段和闭环回归；单一型号参数变化可以只运行该工程和受影响场景，但必须说明缩减理由。

## 8. 诊断原则

- schema/结构错误：在生成前失败，指明 JSON 路径、ID 和规则。
- Modelica 翻译错误：先区分缺库、版本、结构奇异、单位/维度和方程不平衡。
- 仿真不收敛：检查初值、刚性、接触正则化、关节冲突和参数量级，再调整求解器。
- 浏览器显示错误：比较源码 annotation、解析结果和渲染坐标；显示问题不能通过改变物理连接来绕过。
- 结果差异：先核对模型版本、参数、求解器、初值、采样和变量映射，再判断物理偏差。
