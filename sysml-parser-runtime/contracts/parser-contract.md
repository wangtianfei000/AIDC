# Parser Contract v0.1

## 1. 边界

浏览器只依赖 `ModelSnapshot`，不直接依赖 Qt `Resource*`、`Element*`、ANTLR 上下文或引擎内部类名。Parser Runtime 接收 UTF-8 SysML 文本和文档 URI，返回符合 `model-snapshot.schema.json` 的一次性只读快照。

## 2. 操作

| 操作 | 输入 | 输出 |
|---|---|---|
| `parse` | `uri`、`text`、可选 `requestId` | `ModelSnapshot 0.1` |
| `analyze` | `uri`、`text`、可选 `requestId` | `AnalysisSnapshot 0.1`（含 `parseDiagnostics`） |
| `capabilities` | 无 | 能力 ID、状态、基线和版本 |
| `health` | 无 | 进程、Parser 基线和 Schema 版本 |
| `cancel` | `requestId` | 是否接受取消 |

P0 只冻结契约，HTTP/进程传输在 P1 实现。

## 3. 确定性规则

- 同一 Parser 基线与同一内容哈希必须生成相同的元素、关系顺序和 ID。
- Schema 中行、列均从 1 开始，offset 从 0 开始；Adapter 必须把 ANTLR 的 0 基列号规范化。
- P1 在 Parser 缺少原生 ID 时，按 `文档 URI + kind + qualifiedName + source start` 生成确定性 ID；同名且无源码位置的元素必须产生 `AIDC_ID_AMBIGUOUS` 诊断，不能静默合并。
- 所有数组按源码位置排序；无源码位置时按 `kind + qualifiedName + name` 排序。
- 未知元素保留为其实际 `kind` 和通用属性，不能丢弃整棵子树。

## 4. 缺失能力表达

- Parser 无法提供源码范围时，`source` 为 `null`，同时将 `model.source-span` 报为 `unsupported`。
- 无法解析关系端点时仍可保留关系，但端点数组为空，并产生 `AIDC_UNRESOLVED_ENDPOINT`。
- `supported` 表示已由断言式测试验证；只有 Visitor 或语法规则而没有动态验证时最多标为 `partial`。
- 语法恢复后产生的元素必须与 error 诊断同时返回，消费者不能把 `resourceCreated=true` 等同于解析成功。
- 不用猜测补出 verify、derive、refine 等关系。能力为 `unsupported` 时，浏览器显示“Parser 尚不支持”。

## 5. 当前内置 Parser 适配要求

内置 Parser 的类型化对象树不能直接满足 Schema：缺少稳定 ID、元素源码范围、结构化诊断和完整追溯关系。Adapter/IR 层负责补齐需求 MVP 使用到的投影；Qt 对象模型只能作为进程内部中间状态。

## 6. C1 计算与视图扩展

Runtime 0.6.0 在同一个 `ModelSnapshot 0.1` 中增加两个可选投影：

- `calculationModel` 引用通用元素稳定 ID，提供 CalculationDefinition、CalculationUsage、AnalysisCase、Parameter、结构化 Expression AST 和 Constraint；
- `viewModel` 引用同一批稳定 ID，提供 ViewDefinition、ViewUsage、RenderingUsage、`expose` 限定名与有序 `columnView`。

这些投影是 Parser 语义对象的只读索引，不是第二套所有权模型。跨文件 `expose` 目标尚未加载时必须保留 `exposedReferences` 并将 `exposedElementIds` 留空，消费者不得按名称猜测目标。表达式 AST 当前为 `partial`，只承诺已经过测试的标量、单位、引用、四则、比较与调用子集。

## 7. C2 计算扩展

Runtime 0.8.0 的 `/analyze` 在同一次只读请求中先生成 ModelSnapshot，再消费其 CalculationModel。`AnalysisSnapshot 0.1` 按分析返回结果、约束、求值顺序和直接依赖，结果状态限定为 `computed`、`missing-input`、`unit-error`、`unsupported`、`cycle` 或 `runtime-error`。约束在可以比较时返回 `actual`、`target`、`margin`、`unit`、`pass/fail` 和直接目标参数 ID。CalculationUsage 同时返回有序的形式参数/实际参数绑定，实际参数使用 ModelSnapshot 的稳定元素 ID。

数值统一换算到 SI 基值运算，并保留模型声明的显示单位。相加、相减和比较要求量纲一致；乘除生成合成量纲。未知单位、类型量纲冲突、除零、非有限结果和循环依赖必须产生显式诊断，不能返回伪数值。浏览器只消费计算结果，不执行 SysML 表达式。
