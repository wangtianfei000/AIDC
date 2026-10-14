# AIDC SysML 只读 Parser Runtime

该程序把仓库内维护的 C++/ANTLR Parser 和类型化模型封装为仅监听 `127.0.0.1` 的只读 HTTP Runtime。Runtime 使用 Qt Core、Network 和 Concurrent 提供模型适配、JSON 与本机 HTTP 服务；Web 浏览器只消费稳定 JSON 契约，不直接链接 Qt 或 ANTLR 类型。

## 接口

| 方法 | 路径 | 用途 |
|---|---|---|
| `GET` | `/health` | Runtime、Schema 和 Parser 基线 |
| `GET` | `/capabilities` | 明确 supported/partial/unsupported 能力 |
| `POST` | `/parse` | 输入 `{uri,text,requestId?}`，返回 ModelSnapshot |
| `POST` | `/analyze` | 输入 `{uri,text,requestId?}`，返回 AnalysisSnapshot，并附解析诊断 |
| `POST` | `/cancel` | 输入 `{requestId}`，协作式取消活动任务 |

`/parse` 和 `/analyze` 的请求 ID 通过响应头 `X-Request-Id` 返回。前者严格返回 ModelSnapshot；后者先解析再求值，返回 AnalysisSnapshot，并把 Parser 诊断放在 `parseDiagnostics`。取消是协作式的：Parser 自身尚无中断点，因此正在执行的 ANTLR 阶段会完成，结果在返回前被丢弃。

## 构建

开发机构建需要 Qt 6.8.1 MinGW 和 CMake。P2 已把 Grammar、ANTLR 生成代码、Parser 和 Visitor 迁入仓库；普通构建不需要 Java、ANTLR Tool JAR、参考工程源码或参考工程静态库：

```powershell
./build-runtime.ps1
```

脚本执行 CMake Release 构建、C++ 契约测试、安装，并通过 `windeployqt` 把所需 Qt DLL 放入 `dist/bin`。最终用户运行发布目录时不需要单独安装 Qt，但发布包必须携带这些运行库。

也可直接执行独立的内建 Runtime 构建：

```powershell
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

该模式构建 `aidc_sysml_common`、`aidc_sysml_model`、`aidc_sysml_infrastructure`、`antlr4_static`、`aidc_sysml_parser`、独立的 `aidc_sysml_serializer` 和完整 HTTP Runtime，并校验源码边界、P4 自包含门禁和 P5 引擎维护契约。P1/P2 导入哈希保留为历史来源证据，不再阻止有版本记录和测试覆盖的正常源码修改。P3 的双实现等价结果保存在 `migration/p3-validation.json`；生产构建只有仓库内引擎这一条路径。

常规构建直接使用已提交的 12 个 ANTLR 生成文件。只有修改 `engine/parser/grammar/SysML.g4` 时才需要 ANTLR Tool 4.13.2：

```powershell
./tools/regenerate-parser.ps1 -AntlrJar <antlr-4.13.2-complete.jar>
```

脚本先在临时目录生成并比较固定文件集合；确认差异后使用 `-AcceptChanges -EngineVersion <新版本>` 更新生成代码和 `engine/MAINTENANCE.json`。完整维护流程见 `engine/MAINTENANCE.md`。

## 运行

```powershell
$env:PATH="D:/Software/Qt/6.8.1/mingw_64/bin;D:/Software/Qt/Tools/mingw1310_64/bin;$env:PATH"
./build/bin/aidc-sysml-parser-runtime.exe --port 47321
```

进程启动后向 stdout 输出实际监听地址和端口。可用 `--port 0` 自动选择空闲端口，`--max-request-bytes` 限制正文大小，`--parse-timeout-ms` 设置协作式超时。

源码构建默认加载仓库内固定版本的官方模型库。发布包从 `library/sysml.library` 携带同一版本，也可通过 `--standard-library <path>` 或 `AIDC_SYSML_LIBRARY_ROOT` 指定目录。`/health` 和 `/capabilities` 会返回库版本、提交、文件数和符号数；库不可用时 Runtime 状态为 `degraded`。

完成 Release 部署后也可以直接运行：

```powershell
./run-runtime.ps1 -Port 47321
```

## 当前边界

- 稳定 ID 由 URI、元素类型、限定名、owner、保守源码位置和碰撞序号确定。
- 参考 Parser 不保存 token 范围；当前 SourceSpan 定位到元素名称，重名或匿名元素会产生 `AIDC_SOURCE_SPAN_PARTIAL`，能力状态为 `partial`。
- Parser 语法中没有 verify、derive、refine，Runtime 明确返回 `unsupported`，不会推测关系。
- `requirement.constraint` 为 `supported`：Runtime 分别输出 `RequirementConstraint`、`AssertConstraint`、`AssumeConstraint` 和 `Constraint` 关系。该状态只表示已解析语句的适配完整，不代表参考 Parser 可接受所有正式 SysML v2 语法。
- `calculation.definition`、`calculation.analysis` 和 `view.table-rendering` 已通过金刚 C0 契约黑盒测试；表达式 AST 当前覆盖标量字面量、单位、特征路径、四则运算、比较和函数调用，高级函数与集合表达式仍为 `partial`。
- `/analyze` 支持确定性标量四则运算、函数输入绑定、无环依赖求值、兼容单位换算、量纲检查、除零/溢出/循环诊断，以及约束目标、余量和通过状态。首版单位覆盖质量、时间、电流、电荷、电压、能量和功率；幂、集合、`sum/min/max`、采样函数和外部求解器尚不支持。
- View 文件跨文件解析时，`exposedReferences` 始终保留限定名；只有目标元素已位于当前解析资源中时才填写 `exposedElementIds`。跨文件链接将在后续工作区索引阶段完成。
- Package 层级由源码大括号作用域校正，可在部分解析结果中保持模型导航树的包结构。普通模型元素的 owner 仍来自参考 Parser；解析错误较多时，个别非 Package 元素仍可能被挂到上级 Package。
- Runtime 只接受无 Origin、`null` Origin 或 localhost/127.0.0.1 Web Origin，并限制请求大小。
- 内建 Parser 核心不编译 UI 耦合的 `AnnotationParser`，也不链接架构 metadata；Serializer 作为独立 target，不进入只读 Runtime 默认解析路径。
- 标准库使用官方文本模型建立只读符号索引，不将 94 个库文件整体转成当前参考 Parser 的 Qt 对象图；ModelSnapshot 只物化用户文件实际引用到的库元素，并保存规范名、源文件、发布标签和提交。

Schema 与契约位于本目录的 `contracts/model-snapshot.schema.json`、`contracts/analysis-snapshot.schema.json` 和 `contracts/parser-contract.md`；官方标准库位于 `standard-library/2026-08`。
