# AIDC SysML 只读 Parser Runtime

该程序把参考项目的 C++/ANTLR Parser 封装为仅监听 `127.0.0.1` 的只读 HTTP Runtime。0.8.1 在单位感知计算和计算语义绑定基础上，将 `asElementTable` 列的稳定业务键与模型声明的 `label` 一并投影，浏览器可完全按 SysML View 生成列顺序和表头；Web 浏览器不需要链接 Qt、ANTLR 或参考项目类，也不自行解释公式。

## 接口

| 方法 | 路径 | 用途 |
|---|---|---|
| `GET` | `/health` | Runtime、Schema 和 Parser 基线 |
| `GET` | `/capabilities` | 明确 supported/partial/unsupported 能力 |
| `POST` | `/parse` | 输入 `{uri,text,requestId?}`，返回 ModelSnapshot |
| `POST` | `/analyze` | 输入 `{uri,text,requestId?}`，返回 AnalysisSnapshot，并附解析诊断 |
| `POST` | `/cancel` | 输入 `{requestId}`，协作式取消活动任务 |

`/parse` 和 `/analyze` 的请求 ID 通过响应头 `X-Request-Id` 返回。前者严格返回 ModelSnapshot；后者先解析再求值，返回 AnalysisSnapshot，并把 Parser 诊断放在 `parseDiagnostics`。取消是协作式的：参考 Parser 自身尚无中断点，因此正在执行的 ANTLR 阶段会完成，结果在返回前被丢弃。

## 构建

```powershell
cmake -S . -B build -G Ninja `
  -DCMAKE_PREFIX_PATH=D:/Software/Qt/6.8.1/mingw_64 `
  -DCMAKE_CXX_COMPILER=D:/Software/Qt/Tools/mingw1310_64/bin/g++.exe `
  -DREFERENCE_ROOT=D:/Document/7.AWorks/BlockSim/2.Development/SysML `
  -DREFERENCE_BUILD=D:/Document/7.AWorks/BlockSim/2.Development/SysML/build/Desktop_Qt_6_8_1_MinGW_64_bit-Debug
cmake --build build --parallel 4
```

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
- 参考 Parser 当前仍链接 Qt Gui 和架构 metadata；后续应继续抽离。
- 标准库使用官方文本模型建立只读符号索引，不将 94 个库文件整体转成当前参考 Parser 的 Qt 对象图；ModelSnapshot 只物化用户文件实际引用到的库元素，并保存规范名、源文件、发布标签和提交。

Schema 与契约位于 `../reference-parser-p0/model-snapshot.schema.json`、`../reference-parser-p0/analysis-snapshot.schema.json` 和 `../reference-parser-p0/parser-contract.md`。
