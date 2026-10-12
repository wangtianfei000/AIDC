# 参考 Parser P0 基线与能力盘点

> 状态：P0 已执行，结论为“有条件通过”  
> 日期：2026-09-17  
> 对象：`D:/Document/7.AWorks/BlockSim/2.Development/SysML`

## 1. 结论与决策

参考项目的 C++/ANTLR Parser 可以作为首期实现的代码起点，但不能直接作为需求浏览器内核交付。它能构建，并能恢复 Package、RequirementDefinition、RequirementUsage、Part、Port 等基础对象；需求追溯、源码定位和跨文件语义尚不满足浏览器场景。

P0 门禁判定为**有条件通过**：

- 通过：Parser 静态库可构建；Package、需求定义/实例和 ownership 能从样例中产生；语法错误能给出行列诊断。
- 未通过：稳定 ID、元素 SourceSpan、verify、derive、refine、完整 satisfy 和许可证条件。
- 实施决策：仍按既定方向使用参考 C++ Parser，但 P1 第一部分必须先实现 Parser Adapter、SourceSpan、确定性 ID、结构化诊断和最小需求追溯子集；在这些能力完成前不进入需求浏览 UI 的验收。
- 发布约束：本地仓库未发现许可证文件。可以继续做内部技术验证，不能据此认定可以复制、修改后分发或随产品发布。

## 2. 可复现基线

### 2.1 代码状态

| 项 | 值 |
|---|---|
| HEAD | `9bfba0bced98b7586bab8a6d45746064aff70fe1` |
| 提交时间 | 2026-01-09 00:44:07 +08:00 |
| 提交说明 | 初步完成需求视图 |
| 远程仓库 | `https://gitee.com/wtf0/sys-ml.git` |
| 工作树 | 非干净：31 个已跟踪文件有改动，并有未跟踪代码、文档、构建目录和样例 |
| 固定方式 | HEAD + `source-manifest.json` 中关键文件 SHA-256 |

不能只用 commit 表示本次测试对象，因为 Parser Visitor、Element、Parser CMake、需求夹具和架构注解均含工作树改动。P0 没有 reset、checkout 或写入参考项目。

### 2.2 构建环境

| 项 | 值 |
|---|---|
| CMake | 3.29.3 |
| Ninja | 1.12.0 |
| C++ | MinGW GCC 13.1.0 |
| Qt | 6.8.1 mingw_64 |
| ANTLR Runtime | 4.13.2 static |
| 构建类型 | Debug |

现有 `sysml_parser` 目标构建成功，产物为 `libsysml_parser.a`。当前 Parser 除 Qt Core、ANTLR 和模型层外，还依赖 Qt Gui 与 `sysml_architecture_metadata`，不符合原计划中的轻量只读 Runtime 边界。

## 3. 实测结果

### 3.1 测试汇总

| 测试 | 结果 | 说明 |
|---|---:|---|
| 核心 Element/Package/Resource/Serializer | 4/4 通过 | 现有可执行文件直接运行 |
| 参考项目 `parser/tests/test_parser.cpp` | 6/8 通过 | P0 在外部工程编译，参考顶层构建未包含这些测试 |
| 6 个代表性模型创建 Resource | 6/6 | 即使有语法错误仍可能返回 Resource |
| 代表性模型无语法错误 | 3/6 | 共 6 条语法错误 |
| 无效语法诊断 | 通过 | 返回 line 2、column 9；同时仍返回部分 Resource |

Parser 测试失败项都是 `feature f1;` 语法：`testFeature` 和包含相同写法的 `testComplexStructure`。这说明仓库自带 Parser 测试与当前语法实现不一致，而且这些测试没有接入顶层 CMake/CTest，平时构建不会暴露失败。

### 3.2 代表性模型

| 样例 | 错误 | 警告 | 元素 | 观察 |
|---|---:|---:|---:|---|
| `test_requirement_definition.sysml` | 2 | 18 | 41 | 恢复 5 个 RequirementDefinition、2 个 RequirementUsage |
| `test_requirement_table.sysml` | 0 | 24 | 88 | 恢复 7 个定义、5 个实例、3 个 ViewUsage；该文件含工作树修改 |
| `test_relationships_comprehensive.sysml` | 2 | 4 | 14 | 在 subclass/后续 part 附近报错，只恢复部分关系 |
| `test_architecture_view.sysml` | 0 | 25 | 116 | 恢复 Part/Port/Connect 和 42 个 Annotation；该文件未被 Git 跟踪 |
| `test_interface_port_connector.sysml` | 2 | 18 | 41 | interface usage 形式报错，仍恢复部分端口和连接器 |
| `Vehicle.sysml` | 0 | 14 | 58 | 恢复 Part、Port、State、Transition 等 |

六个样例合计 103 条警告，主要为 String、Real、ISQ 类型、Usage typing 和 import 未解析。警告不能简单隐藏：它们会直接影响需求表中的类型、架构端口类型和跨文件导航。

`Jingang` 与 `AIDC` 当前没有发现 `.sysml` 或 `.kerml` 文件，因此本轮没有“金刚业务模型”可用于人工真值比对。P1 需要先补一份最小机器人需求模型。

## 4. 对需求浏览器的能力判断

| 浏览能力 | 结论 | 原因 |
|---|---|---|
| 需求列表/树 | 部分可用 | 能产生需求对象与 ownership，但字段语义和类型解析不完整 |
| 需求详情 | 部分可用 | 名称、限定名和子元素可读；没有稳定 ID、源码范围和完整需求专有字段 |
| 点击定位源码 | 不可用 | Element 不保存 token 范围 |
| satisfy 追溯 | 不可验收 | 只在 View body 有局部实现，复杂形式留有 TODO |
| verify/derive/refine | 不可用 | 当前语法和 Visitor 中没有对应实现 |
| 验证矩阵 | 不可用 | Verification/verify 关系缺失 |
| 跨文件浏览 | 部分可用 | 未解析引用以警告保留，没有完整项目级符号解析 |
| 架构扩展 | 部分可用 | Part/Port/Connect 已有基础对象，但端点和类型仍有缺口 |

“能返回对象”不能等同于“模型正确”。Web 浏览器必须同时显示错误、警告和当前 Parser 能力，避免把错误恢复生成的部分模型呈现成完整事实。

## 5. P1 的调整后入口

P1 不应先封装 HTTP，再处理语义缺口。建议按以下顺序执行：

1. 将 P0 的 `ModelSnapshot 0.1` 和 Parser Contract 做成契约测试。
2. 在 Visitor 中保存元素 token start/stop，输出 URI、行、列和 offset。
3. 建立确定性 ID；检测同名且无定位信息的冲突。
4. 将格式化字符串诊断改为 code、severity、message、range。
5. 用一份最小机器人需求夹具补齐并验证 Requirement、subject/stakeholder、约束、satisfy、verify、derive、refine。
6. 增加标准库与多文件解析上下文，明确 unresolved reference。
7. 抽离 Qt Gui/架构元数据依赖后，再封装 `parse/cancel/capabilities/health` 接口。

P1 的新增门禁：最小需求夹具必须实现元素数、关系端点和 SourceSpan 的精确断言；同一输入连续解析结果相同；任何未支持关系都必须生成能力状态或诊断。

## 6. P0 交付物

| 交付物 | 位置 |
|---|---|
| 固定基线与复用/许可证记录 | `AIDC/reference-parser-p0/source-manifest.json` |
| 机器可读能力清单 | `AIDC/reference-parser-p0/parser-capabilities.json` |
| 缺口清单 | `AIDC/reference-parser-p0/parser-gaps.md` |
| ModelSnapshot JSON Schema v0.1 | `AIDC/reference-parser-p0/model-snapshot.schema.json` |
| Parser Contract v0.1 | `AIDC/reference-parser-p0/parser-contract.md` |
| 独立 C++ 探针与测试包装 | `AIDC/reference-parser-p0/probe/` |
| 原始运行结果 | `AIDC/reference-parser-p0/results/` |

