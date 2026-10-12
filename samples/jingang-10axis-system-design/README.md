# 金刚原生十轴系统设计样例

本目录是“机器人系统架构与总体布局”P0 的规范样例。目标配置固定为：

- 腿部八轴：每侧髋侧摆、髋俯仰、膝屈伸、踝俯仰；
- 头颈两轴：颈根俯仰、头部偏航；
- 十个 HD-1910-C001 执行器；
- 6.4 V、1.8 Ah、3 A 共享供电研究模型；
- 躯干 IMU；
- 200 Hz 关节控制、50 Hz 策略周期。

此配置排除旧十四轴模型和十二轴回退方案。它是系统架构与布局研究基线，不是制造冻结配置。

## 文件

- `requirements.sysml`：样例输入需求，只包含本轮架构使用的四项业务需求。
- `architecture.sysml`：当前架构事实源；C3 已把系统组成、端口、连接、原始方案参数、四条计算链、约束和参数 Table View 集成到同一模型。
- `evidence/source-register.md`：模型数值、选型和待验证项的来源登记。
- `p0-parser-result.json`：当前 Parser Runtime 的完整解析快照。
- `p1-parser-result.json`：Runtime 0.4.0 加载官方标准库后的完整解析快照。
- `p1-parser-summary.json`：P1 类型、端点、单位和诊断验收摘要。
- `views/diagram-expectations.json`：G0 冻结的整机、左腿和供电网络三张图的验收真值。
- `views/graphic-metadata-compatibility.sysml`：参考 `@GraphicMetadata` 最小兼容模型。
- `views/graphic-metadata-parser-result.json`：最小兼容模型的 Runtime 实测快照。
- `g0-parser-result.json`：增加左右腿和头颈内部连接后的完整解析快照。
- `g0-parser-summary.json`：G0 图形测试模型解析摘要和 typed instance 端点缺口。
- `g1-parser-result.json`：Runtime 0.5.0 补齐 typed part 实例端口后的完整解析快照。
- `g1-parser-summary.json`：G1 的 31 条 Connection 端点解析验收摘要。
- `analysis/calculation-contract.sysml`：C0 四条核心参数计算和 Analysis 契约模型。
- `analysis/c0-expected-results.json`：四项手算结果、余量、状态和 Table View 顺序真值。
- `analysis/verify-c0-expected.py`：C0 手算真值自动校验。
- `analysis/c0-capability-matrix.md`：Calculation、Analysis 和 Table View 的当前能力与 C1 目标。
- `analysis/c0-calculation-parser-*.json`：当前 Runtime 解析计算契约的黑盒基线。
- `views/parameter-analysis-table.sysml`：首个 `GridView + asElementTable` 契约模型。
- `views/c0-table-view-parser-*.json`：当前 Runtime 解析 Table View 契约的黑盒基线。
- `analysis/c3-expected-results.json`：C3 当前金刚方案的分类输入、假设状态、四项预期结果和 Table View 真值。
- `analysis/c3-model-snapshot.json`：Runtime 0.7.0 对集成架构模型的语义快照。
- `analysis/c3-analysis-snapshot.json`：Runtime 0.7.0 对集成架构模型的计算结果。
- `analysis/c3-verification-summary.json`：无手填派生结果、四条计算链、需求约束和 4×8 Table View 的自动验收摘要。
- `analysis/c3-http-smoke-summary.json`：正式架构模型的重复解析、重复计算、输入变化传播和 HTTP 边界回归摘要。
- `layout/jingang-10axis-layout.FCStd`：P4 详细机械装配基线，保留真实设计零件形状并使用与 SysML 一致的 19 个稳定部件 ID。
- `layout/jingang-10axis-layout-preview.png`：详细装配预览。
- `layout/p4-layout-manifest.json`、`layout/p4-detailed-parts.csv`、`layout/p4-validation.json`：详细几何清单、30 个 STL 尺寸与哈希以及配置阻断项。
- `layout/README.md`：装配复现方法和不能误读的验证边界。

`p0-*`、`p1-*`、`g0-*`、`g1-*` 和 C0/C1 文件是相应阶段的历史或最小契约快照。当前架构与参数分析状态以 `architecture.sysml` 和 `analysis/c3-*` 为准。

P4 已在 `layout/` 下把原方盒包络模型替换为详细十轴机械装配，并完成 19 个 `componentId` 映射。自制件形状来自现有 FreeCAD/STL 设计，当前结果按初步装配边界完成；电子件配置冲突、IMU SKU 和 HD-1910 实物接口继续作为机械详细设计未关闭项，制造状态仍为 `NOT_RELEASED`。

## 不能误读的内容

- `816.8[g]` 是五类 CAD-2 规划输入的计算结果，不是实物称重结果；软件显示未通过 800 g 目标。
- `24.5[g]` 是 HD-1910-C001 公开质量范围的上界，用于规划，不是本批次实测值。
- 电池和大部分电子件尚未冻结 SKU。
- 舵机真实安装孔、轴端、花键、对侧支承和线缆出口尚未核实。
- 2 小时续航是需求目标；当前 1.92 h 使用 0.8 可用能量比例和 0.25 A 总线平均电流假设，结论为未通过且必须实测。
- 关节角是初始研究窗口，不是机械安全限位。
