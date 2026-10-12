# 金刚原生十轴样例来源登记

| 模型内容 | 值或决定 | 状态 | 来源 |
|---|---|---|---|
| 目标配置 | 腿部八轴＋头颈两轴 | 已选研究主线 | `Jingang/docs/自研机器鸭_低舵机稳固结构设计_V0.3.md` |
| 排除配置 | 十四轴旧模型；十二轴仅回退比较 | 已明确 | `Jingang/docs/自研机器鸭_10轴主方案_CAD绘制计划.md` |
| 执行器 | HD-1910-C001 ×10 | 型号指定，适用性待验证 | 同上 |
| 执行器公开质量上界 | 24.5 g/个 | 参考上界，不参与当前 CAD-2 分类合计 | `Jingang/docs/自研机器鸭_CAD2_部件布置评审.md` |
| 整机质量目标 | 不大于 800 g | 设计目标 | `Jingang/docs/自研机器鸭_低舵机稳固结构设计_V0.3.md` |
| 十个舵机分类质量 | 225.0 g | CAD-2 规划估计，非逐件实测 | `Jingang/docs/自研机器鸭_CAD2_部件布置评审.md` |
| 结构件及脚垫 | 349.8 g | 当前实心几何体积与名义密度估计 | 同上 |
| 电池、主控及其他电子件 | 172.0 g | 未冻结全部 SKU 的规划估计 | 同上 |
| 轴承、轮毂、紧固件 | 45.0 g | 分配额度 | 同上 |
| 线束与接头 | 25.0 g | 分配额度 | 同上 |
| 当前规划总质量 | 816.8 g | 由上述五类输入通过 `ComputeTotalMass` 计算，不再手填 | `architecture.sysml` 的 `coreParameterAnalysis` |
| 供电模型 | 6.4 V、1.8 Ah、3 A | 仿真研究基线，实物待冻结 | `Jingang/docs/金刚机器鸭_原生10轴走路训练实施计划.md` |
| 电子件电流 | 0.5 A | 仿真研究基线 | 同上 |
| 关节控制周期 | 5 ms / 200 Hz | 已冻结训练协议 | 同上 |
| 策略周期 | 20 ms / 50 Hz | 已冻结训练协议 | 同上 |
| IMU | 躯干 IMU | 类别确定，SKU 与安装坐标待确认 | 同上；`Jingang/docs/自研机器鸭_CAD2_部件布置评审.md` |
| 续航目标 | 不少于 2 h | 本样例业务需求 | `requirements.sysml` |
| 电池名义能量 | 11.52 Wh | 由 6.4 V × 1.8 Ah 在 `ComputeEndurance` 内计算 | `architecture.sysml` |
| 可用能量比例 | 0.8 | C3 方案假设，非实测 | `architecture.sysml`；需要完整放电测试 |
| 典型执行器总线平均电流 | 0.25 A | C3 方案假设，非实测 | `architecture.sysml`；需要典型行走任务电流记录 |
| 执行器总线峰值电流 | 2.6 A | C3 方案假设，用于检查 3 A 共享限流 | `architecture.sysml`；需要峰值电流与温升测试 |
| 电子件电流 | 0.5 A | 原生十轴训练模型基线，非实物测量 | `Jingang/docs/金刚机器鸭_原生10轴走路训练实施计划.md` |
| 控制处理时间 | 3 ms | C3 方案假设，非实测 | `architecture.sysml`；需要控制链路时间戳测量 |
| 执行器停止响应时间 | 120 ms | C3 方案假设，非实测 | `architecture.sysml`；需要单舵机台架测试 |
| 命令检测时间 | 5 ms | 取 200 Hz 控制周期作为上界 | `Jingang/docs/金刚机器鸭_原生10轴走路训练实施计划.md` |
| 腿部关节初始窗口 | 髋侧摆 ±18°、髋俯仰 ±35°、膝 5°～65°、踝俯仰 ±30° | 研究窗口，非安全限位 | `Jingang/docs/自研机器鸭_低舵机稳固结构设计_V0.3.md` |
| 头颈关节初始窗口 | 颈根 5°～35°、头偏航 ±60° | 研究窗口，非安全限位 | 同上 |
| CAD 关联 | `layout://...` | 19 个 SysML ID 已映射到详细 CAD 对象；模型状态为 `NOT_RELEASED_DETAILED_REVIEW_REQUIRED` | `architecture.sysml`；`layout/jingang-10axis-layout.FCStd` |
| 详细自制件 | 30 个 STL 文件、39 个打印件 | 尺寸、体积、三角面数和 SHA-256 已核对；材料、公差、载荷和实物配合仍待放行 | `layout/p4-detailed-parts.csv`；`Jingang/apps/models/CAD/Microduck_Final/STL/` |
| HD-1910 几何 | 外壳 `34 × 20 × 23 mm`、总厚 29 mm、安装孔距 `30 × 16 mm` | 按已有尺寸图重建外壳与孔型；花键、螺纹、盲孔深度和样件配合未验证 | `layout/p4-validation.json`；`Jingang/workspace/hd1910_dimension_update.json` |
| 计算主控 | 当前详细 CAD 为 Raspberry Pi Zero 2 W；资料中另有 Radxa ZERO 3W | 配置冲突，P4 阻断项 | `layout/p4-validation.json` |
| 运动控制器 | 当前详细 CAD 为 RPI Robot HAT；第一台机器说明另有 OpenRB-150 | 配置冲突，P4 阻断项 | `layout/p4-validation.json` |
| 电池几何 | `19 × 37 × 68 mm`、6.4 V、1800 mAh | 现有实物包络记录；SKU 和连接器身份未记录 | `Jingang/apps/models/CAD/Microduck_Final/STL/打印清单.md` |
| IMU 几何 | `25.8 × 15.5 mm` 平面包络 | 仍为占位；SKU、孔位、连接器、坐标和元件高度未冻结 | `layout/p4-validation.json` |

## 关键未关闭项

1. HD-1910-C001 正式机械尺寸图、STEP、轴心、花键、孔位、对侧支承和连接器包络。
2. 电池、电池座、主控、实时控制板、配电模块和 IMU 的具体 SKU。
3. 五类规划质量的实物称重、典型/峰值电流、电池可用容量、保护策略和连续续航。
4. 允许动作范围内的连续净空、线束弯曲和防夹。
5. 单腿/髋部真实安装、紧固空间、承力与刚度验证。
6. 命令检测、控制处理和单舵机停止响应的时间戳测试；当前 128 ms 只是方案计算值。
