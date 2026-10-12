---
name: robot-modelica-model-development
description: 基于 AIDCRobotics 标准库、Robot IR 和 AIDC 装配发布包创建、扩展、校验并交付机器人 Modelica 模型。适用于新机器人实例、机械/执行器/电气/传感/接触组件开发、Robot IR 确定性生成、OMC 仿真、FMU/便携包和回归验收；不用于 CAD 几何设计、GPU 训练模型，或用未验证参数宣称硬件定型。
---

# 机器人 Modelica 模型开发

以现有 `AIDCRobotics` 标准库为唯一公共建模基础。优先复用稳定接口和组件，把机器人型号数据、公共方程、用户配置和生成产物分开，最后用可重复的编译、仿真和结果证据完成交付。

## 开始前

1. 通过 AIDC Skill 资源读取建模契约；需要检查产品能力时调用 `aidc_system_capabilities`。不要依赖源码仓库位置。
2. 判断任务属于哪条路径：
   - 完整机器人或 CAD 装配转换：使用合规 `.aidc-assembly/` 或 Robot IR，通过生成器构建型号包。
   - 新的可复用物理行为：在 `AIDCRobotics` 中开发公共组件和一致性模型。
   - 单型号参数变化：优先写 `DeviceData` 参数记录或 `UserOverrides`，不要复制方程。
   - 单刚体网格试验：可以直接使用 STL，但不得把它当成完整机器人输入。
3. 阅读与任务相关的引用：
   - 建模分层、坐标、单位、接口和扩展规则见 [建模契约](references/modeling-contract.md)。
   - 新建、修改、生成和调试步骤见 [开发工作流](references/development-workflow.md)。
   - 交付前的结构、数值、便携性和证据要求见 [验收清单](references/acceptance-checklist.md)。

正式自动化入口是 `aidc_modelica_validate`、`aidc_modelica_generate`、`aidc_modelica_simulate` 和 `aidc_modelica_package`；没有 MCP 的 CI 环境使用对应的 `aidc modelica ...` 命令。长任务返回 `taskId` 后使用 `aidc_task_status`，完成后通过 `aidc_artifacts_list` 获取产物。

## 必须遵守的边界

- 标准库保存可复用方程、连接器和能力契约；机器人型号包只组合标准组件并提供拓扑和参数。
- `DeviceData` 保存有来源的设备参数；`UserOverrides` 保存用户可调配置。参数来源、单位和置信度必须可追溯。
- 生成的 Modelica 源码由 Robot IR 和覆盖文件派生。需要变化时修改输入并重新生成，不直接维护生成文件。
- 未知 Robot IR schema 必须显式迁移，禁止静默降级。
- 连接关系必须使用 `connect(...)` 表达；`annotation` 只负责视图，不得承载物理语义。
- 缺少质量、质心、惯量、关节轴、限位或父子关系时，停止整机生成并给出诊断；不要猜测。
- 仿真通过只证明模型和工具链可用。没有台架或实机证据时，不得声明硬件定型或硬件发布就绪。

## 执行顺序

1. 冻结输入版本和目标能力 Profile，列出根基座、刚体、关节、执行器、供电、传感器、接触与公开 I/O。
2. 先运行 schema 与语义校验，再决定复用、参数化或新增标准组件。
3. 生成整机型号包，或按标准接口组合手写模型；同时提供可运行的 `Experiment`。
4. 检查刚体树、通道顺序、关节方向、单位、质量与惯量、命令/状态/诊断接口以及图形连接视图。
5. 用目标 OpenModelica 版本编译和仿真；按模型风险运行静态、阶跃、周期、负载变化、限流或接触场景。
6. 重复生成并比较锁文件、结构清单、源码与结果；需要跨工具或交付时执行结果比较和便携打包。
7. 报告实际命令、工具版本、输入摘要、产物路径、数值结果和未验证边界。

## 完成条件

只有在以下条件全部满足时才结束：

- 模型复用 `AIDCRobotics`，没有在型号包中复制公共实现。
- 输入、生成结构、Modelica 工程和公开通道可以通过稳定 ID 相互追踪。
- 编译与要求的仿真场景通过，结果满足已声明容差。
- 重新生成具有确定性，用户覆盖得到保留，错误输入会明确失败。
- 交付报告区分“模型/工具验收”和“硬件验证”，并列出尚缺证据。
