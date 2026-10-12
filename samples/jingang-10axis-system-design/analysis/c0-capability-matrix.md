# C0 Calculation 与 Table View 能力矩阵

基线：Parser Runtime 0.5.0，官方 SysML 2026-08 模型库已加载。

| 能力 | C0 目标 | 当前实测 | C1 要求 |
|---|---|---|---|
| `calc def` | 四个可复用标量计算 | 不支持；在 `calc` 处产生语法错误 | 输出 CalculationDefinition、参数、局部量、返回值和源码 |
| 计算调用 | 位置实参绑定到形式参数 | 不支持 | 输出 CalculationUsage 和有序参数绑定 |
| 表达式 AST | `+ - * /`、括号、比较和特征引用 | 仅保留部分属性文本，不能求值 | 输出结构化 AST、引用目标和量纲信息 |
| `analysis` | 一个分析上下文、四个结果 | 不支持 | 输出 AnalysisCase、subject、结果和约束 |
| Constraint | 四个上下限校核 | 普通需求约束可读；分析内约束未形成计算契约 | 输出比较方向、目标、结果和源码 |
| 标准类型与单位 | mass、energy、power、duration、电压、电流 | 标准库和常用单位已可加载 | 保持单位身份并提供换算信息 |
| ViewDefinition / ViewUsage | 一种 Grid View 和一个实例 | 元素可生成 | 输出类型、继承和稳定 ID |
| `expose` | 四个结果按声明顺序成为行 | ExposeStatement 元素可生成，但目标未投影 | 输出解析后的有序 `exposedElementIds` |
| `render asElementTable` | 声明元素表格 | ViewRenderingMember 可生成，但渲染方式未投影 | 归一化为 `renderingKind=element-table` |
| `columnView` | 八列且顺序稳定 | 不支持 | 输出列 ID、标题、单元格视图和源码 |
| View capabilities | 浏览器判断能否打开 Table | 无 `view.*` 能力标记 | 增加 definition、usage、expose、table 能力 |
| 求值 | 四条结果和余量 | 不支持 | C2 输出 AnalysisSnapshot |

## C0 语法边界

首版只要求：

- 单个文件会话中的显式 `calc def` 和计算调用；
- 位置实参，参数数量必须匹配；
- 确定性标量表达式；
- 标准数量类型与单位；
- 无循环计算图；
- 具名分析结果和简单上下限约束；
- 显式 ViewUsage、逐条 `expose`、单个 `asElementTable` Rendering 和有序列。

首版不要求集合聚合、递归、采样函数、外部脚本、优化、TradeStudy、动态过滤表达式或 Viewpoint 校核。
