# SysML 参考解析器源码迁移计划

## 1. 目标

把历史参考解析器的必要源码迁入 AIDC，由 `sysml-parser-runtime` 直接编译和维护，移除以下开发期外部依赖：

- `REFERENCE_ROOT`
- `REFERENCE_BUILD`
- 参考项目预编译的 `.a` 静态库
- 参考项目的 `build` 目录

迁移后保留现有技术路线：C++17、ANTLR4 C++ Runtime、Qt、只读本机 HTTP Runtime。浏览器接口、ModelSnapshot、AnalysisSnapshot、CLI 命令和发布目录结构保持兼容。

本次只迁移解析内核，不迁入参考项目的桌面应用、编辑器、树视图、需求表格、架构图 UI、示例程序和历史设计文档。

## 2. 当前基线与风险

当前 AIDC 在编译时包含参考项目头文件，并静态链接以下外部产物：

- `libsysml_parser.a`
- `libsysml_model.a`
- `libsysml_infrastructure.a`
- `libsysml_common.a`
- `libsysml_architecture_metadata.a`
- `libantlr4-runtime-static.a`

实际解析入口是 `SysMLTextParser::parseText()`。它调用 ANTLR 生成的 Lexer/Parser，再由 `ModelBuilderVisitor` 构造 `Resource` 和类型化 `Element` 对象树。AIDC 的 `ModelSnapshotAdapter` 将对象树转换为浏览器使用的 JSON。

参考项目当前不是干净工作区：存在 33 个已修改文件以及未跟踪的分析、计算、渲染和注解源码。当前已经通过验收的静态库来自这份工作区，而不只是 Git 提交 `9bfba0b`。因此迁移源必须以“当前已验证工作区快照”为基线，并记录：

- 上游仓库地址和提交：`https://gitee.com/wtf0/sys-ml.git`、`9bfba0bced98b7586bab8a6d45746064aff70fe1`
- 已跟踪文件相对该提交的补丁
- 被纳入迁移的未跟踪源码清单和 SHA-256
- ANTLR4 C++ Runtime 版本 `4.13.2`
- 迁移日期、迁移脚本版本和验收结果

参考项目 README 标记许可证“待定”。内部迁移可以执行，但在许可证和代码归属明确前，公开发行继续保持阻断状态。

## 3. 迁移范围

### 3.1 必须迁入

| 范围 | 当前规模 | 用途 |
|---|---:|---|
| `core/common`、`core/infrastructure`、`core/model` | 172 文件，约 7,686 行 | Resource、Element、Relationship 和类型化 SysML 模型 |
| `parser/grammar/SysML.g4` | 包含于 Parser 范围 | AIDC 维护的语法唯一来源 |
| `parser/src/parser` | 包含于 Parser 范围 | `SysMLTextParser` 入口 |
| `parser/src/visitor` | 包含于 Parser 范围 | ANTLR 语法树到模型对象的构造 |
| ANTLR 生成代码 | 12 文件，约 0.90 MB | 常规构建不依赖 Java/JAR |
| ANTLR4 C++ Runtime | 312 文件，约 1.02 MB | 离线、可复现的 C++ 运行库 |

`parser/src/serializer` 只有在回写 SysML 文本的近期路线中需要。首轮可以迁入但单独成 target，Runtime 默认不链接。

### 3.2 需要先解耦再决定

`parser/src/annotation/AnnotationParser.cpp` 直接引用 `ui/mid_windows/architecture` 下的图形元数据类，导致 Parser 与 Qt Widgets/UI 静态库形成循环依赖。AIDC 浏览器当前解析链不调用 `AnnotationParser::parseGraphicMetadata()`，因此首轮迁移方案是：

1. 保留通用 `Annotation` 模型和语法解析；
2. 不把 `AnnotationParser.cpp` 编入核心解析 target；
3. 图形元数据继续以普通键值投影到 Snapshot；
4. 若后续需要强类型图形元数据，在 AIDC 内新增不依赖 Widgets 的纯数据结构。

这一步完成后，解析内核只需要 Qt Core；Runtime 服务继续使用 Qt Network/Concurrent。Qt Gui 是否仍需要由链接检查决定，不再为了历史 UI 类强制引入。

### 3.3 明确不迁入

- `build/` 及所有 `.a`、对象文件和 CMake 缓存
- `ui/` 桌面界面实现
- `services/` 编辑器服务
- `examples/`
- `CMakeLists.txt.user`
- 参考项目的桌面应用入口 `main.cpp`
- 与 AIDC Runtime 无关的文档和截图

参考项目总目录约 699 MB，其中 `build` 约 671 MB、`doc` 约 23 MB。按上述边界迁入的有效源码预计约 3–5 MB，不应复制整个项目。

## 4. 目标目录

```text
sysml-parser-runtime/
├─ CMakeLists.txt
├─ engine/
│  ├─ CMakeLists.txt
│  ├─ ORIGIN.md
│  ├─ core/
│  │  ├─ common/
│  │  ├─ infrastructure/
│  │  └─ model/
│  └─ parser/
│     ├─ grammar/SysML.g4
│     ├─ generated/
│     └─ src/
│        ├─ parser/
│        ├─ visitor/
│        └─ serializer/          # 可选 target
├─ third_party/
│  └─ antlr4-runtime-4.13.2/
│     ├─ LICENSE.txt
│     ├─ VERSION
│     └─ runtime/
├─ tools/
│  ├─ regenerate-parser.ps1
│  └─ verify_p4_self_contained.py
├─ src/                          # AIDC Runtime、Adapter、Analysis
├─ contracts/
├─ standard-library/
└─ tests/
```

`engine` 是 AIDC 正式维护的解析内核，不再称为临时 `vendor`。`third_party` 只存放版本固定、许可证清晰的 ANTLR Runtime。

## 5. 构建设计

新的 CMake target 划分如下：

```text
antlr4_static
  └─ aidc_sysml_model
       ├─ aidc_sysml_infrastructure
       └─ aidc_sysml_parser
            └─ aidc_parser_runtime_core
                 ├─ aidc-sysml-parser-runtime
                 └─ aidc-parser-runtime-tests
```

正常构建直接编译已提交的 ANTLR 生成代码，不要求用户安装 Java，也不访问网络。只有修改 `SysML.g4` 时才运行 `tools/regenerate-parser.ps1`，该脚本固定 ANTLR Tool 4.13.2，并在生成后验证版本、文件列表和确定性差异。

`build-runtime.ps1` 最终只保留 Qt、CMake 和编译器参数，删除 `ReferenceRoot` 与 `ReferenceBuild`。发布仍通过 `windeployqt` 携带 Qt DLL。

## 6. 实施阶段

### P0：冻结实际基线

**状态：已完成（2026-10-13）。** 冻结产物位于 `sysml-parser-runtime/migration/reference-baseline/`，源码重建、C++、HTTP、浏览器集成与便携包验收均已通过，汇总见 `validation-summary.json`。

1. 停止修改参考项目。
2. 生成受控导入清单，记录每个迁移文件的来源、状态和 SHA-256。
3. 导出相对 `9bfba0b` 的补丁，并记录未跟踪源码。
4. 使用当前外部静态库重跑 C++、HTTP、浏览器和便携包验收，保存基线结果。

验收：可以证明后续 AIDC 内建版本与当前正在运行的版本使用同一份逻辑源码。

### P1：迁入模型层和 ANTLR Runtime

**状态：已完成（2026-10-13）。** 已迁入模型层 168 个文件及 ANTLR4 C++ Runtime 4.13.2 的 309 个源码文件，建立仓库内独立 targets、导入完整性校验和外部路径边界测试；关闭 Runtime 后可在全新构建目录独立编译并通过测试。

1. 迁入 `core/common`、`core/infrastructure`、`core/model`。
2. 迁入 ANTLR4 C++ Runtime 4.13.2、版本文件和许可证。
3. 建立独立 CMake targets，禁止引用 AIDC 目录之外的头文件和库。
4. 增加 include 边界检查，防止重新出现绝对路径。

验收：模型层和 ANTLR Runtime 可以在清理后的全新 `build` 目录中编译。

### P2：迁入 Grammar、生成代码和 Parser

**状态：已完成（2026-10-13）。** 已迁入 `SysML.g4`、对应的 12 个 ANTLR 4.13.2 生成文件、`SysMLTextParser`、`ModelBuilderVisitor` 和独立 Serializer target；新增 `external|embedded` 双实现开关、P2 导入摘要与本地完整 Runtime 构建测试。UI 耦合的 `AnnotationParser` 已明确排除。

1. 迁入当前工作区版本的 `SysML.g4`、`SysMLTextParser` 和 `ModelBuilderVisitor`。
2. 迁入与该 Grammar 对应的生成代码。
3. 从核心 target 排除 UI 耦合的 AnnotationParser；Serializer 独立编译。
4. 将 Parser baseline 从外部 Git 提交改成 AIDC 内部 engine 版本和源码摘要。

验收：删除或重命名原参考项目目录后，AIDC 仍可从零构建 Runtime。

### P3：行为等价验证

**状态：已完成（2026-10-13）。** 外部静态库与仓内实现均通过完整 HTTP 黑盒；7 组能力、模型、分析和错误输入快照规范化后逐字段一致。迁移同时在 AIDC 快照适配层消除了参考模型 `Transition::m_isInitial` 未初始化造成的随机输出，没有改动冻结的迁入源码。验证证据位于 `sysml-parser-runtime/migration/p3-validation.json`，默认构建已切换为 `embedded`。

对同一组输入同时运行“旧外部静态库版本”和“AIDC 内建源码版本”，比较规范化后的输出：

- ModelSnapshot 元素、关系、类型、owner、诊断和能力声明
- AnalysisSnapshot 结果、单位、约束状态和余量
- `VehicleMode.sysml`
- 金刚架构、计算和参数表样例
- 无效语法、超大请求、并发、取消和超时
- 标准库 94 文件与 2,591 个符号的加载状态

稳定 ID、时间戳等非语义字段先规范化再比较。任何语义差异必须有单独变更说明，不能在迁移提交中顺带修改。

验收：C++ 单元测试、HTTP 黑盒、浏览器 39 项测试、P2-R4 集成回归和便携包验收全部通过。

### P4：切换并删除外部依赖

**状态：已完成（2026-10-14）。** CMake 和发行构建脚本已删除双引擎开关、参考源码/构建参数以及外部静态库分支；一次性迁移脚本已删除，历史基线与 P3 等价结果继续作为只读证据保存。SBOM、依赖锁、第三方通知和供应链闸门已改为仓库内引擎口径，并新增自包含 CTest 门禁。

1. 修改顶层 CMake 和 `build-runtime.ps1`，只引用仓库内 `engine`。
2. 删除 `REFERENCE_ROOT`、`REFERENCE_BUILD` 和外部 `.a` 文件检查。
3. 更新 README、SBOM、依赖锁、第三方通知和供应链闸门。
4. CI 增加“断开参考目录后构建”测试。

验收：在没有 `D:\Document\7.AWorks\BlockSim\2.Development\SysML` 的环境中完成构建、测试和便携包启动。

### P5：建立长期维护规则

**状态：已完成（2026-10-14）。** 已建立版本化 `MAINTENANCE.json`、模型新增覆盖策略、依赖边界检查、Grammar/生成代码原子更新工具和 CTest 门禁；Runtime 明确分别暴露 `runtimeVersion`、`engineVersion` 与 `schemaVersion`，来源同步记录由 `ORIGIN.md` 持续维护。

1. Grammar、生成代码和生成器版本必须同一提交更新。
2. 模型类新增时必须包含 Parser 构造测试和 Snapshot 投影测试。
3. 禁止 engine 依赖浏览器、桌面 UI、AIDC CLI 或 RuntimeServer。
4. `ORIGIN.md` 记录首次导入和以后每次上游同步。
5. 解析内核使用独立版本，例如 `engineVersion`，HTTP 契约继续使用 `runtimeVersion` 和 `schemaVersion`。

## 7. 提交拆分与回退

建议拆成四个可独立审查的提交：

1. `chore: record SysML parser source baseline`
2. `feat: import SysML model and ANTLR sources`
3. `feat: build SysML parser from in-repository sources`
4. `refactor: remove external reference parser dependency`

P3 期间保留 CMake 开关 `AIDC_SYSML_ENGINE=external|embedded` 完成双实现对照；P3 证据冻结后，P4 已删除该开关和 `external` 分支。需要审计迁移差异时读取 `migration/p3-validation.json`，不恢复生产构建对外部目录的访问。

## 8. 完成标准

- AIDC 仓库包含构建 Parser 所需的全部一方源码和固定版本第三方源码。
- 构建脚本不存在参考项目绝对路径。
- 普通构建不依赖 Java、ANTLR JAR、网络或预编译 `.a` 文件。
- 不复制参考项目 `build`、桌面 UI 和无关文档。
- 在临时重命名参考项目目录后，全量构建和测试仍通过。
- 便携包可在未安装 Qt、ANTLR、Java和参考项目的机器运行。
- 来源、许可证、版本和本地修改均可追溯。

## 9. 建议执行顺序

P0–P4 已按顺序完成：冻结真实基线、迁入模型与 ANTLR、迁入 Parser、完成双实现等价验证、删除外部路径。后续按 P5 维护规则演进 Grammar 和模型层，不再把历史参考工程接回生产构建。
