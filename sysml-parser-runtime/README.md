# AIDC Standalone SysML Parser Runtime

该目录包含可独立构建的只读 SysML Parser、标准库索引、分析引擎和本机 HTTP Runtime。0.9.0 不再引用外部参考 Parser，不需要 `REFERENCE_ROOT`、`REFERENCE_BUILD`、ANTLR 生成代码或参考项目静态库。参考 Parser 的 P0 材料仅保留在 `../reference-parser-p0` 作为历史兼容基线。

## 组成

- `StandaloneSysMLParser.*`：仓库内自带的词法扫描和递归结构解析，建立最小语义对象树。
- `StandaloneModel.h`：Parser 与快照适配器之间的内部对象模型。
- `ModelSnapshotAdapter.*`：稳定 ID、源码位置、类型/连接解析以及 ModelSnapshot 投影。
- `StandardLibraryIndex.*`：固定版本 SysML/KerML 标准库符号索引。
- `AnalysisEngine.*`：表达式、单位、依赖、约束和诊断计算。
- `RuntimeServer.*`：仅监听 loopback 的 `/parse`、`/analyze`、`/health`、`/capabilities`、`/cancel` 服务。

## 独立构建

源代码构建只要求 CMake、Qt 6.8.1 和与 Qt 匹配的 MinGW。可以通过 `AIDC_QT_ROOT`、`AIDC_MINGW_ROOT` 指定工具链位置：

```powershell
npm run build:sysml-runtime
```

脚本会配置、编译、运行契约测试、安装标准库，并使用 `windeployqt` 将 Qt DLL 放入 `dist/bin`。生成的 `dist` 可复制到没有参考 Parser 源码和构建目录的机器运行。

也可以直接配置源码：

```powershell
cmake -S . -B build -G "MinGW Makefiles" `
  -DCMAKE_PREFIX_PATH=D:/Software/Qt/6.8.1/mingw_64 `
  -DCMAKE_CXX_COMPILER=D:/Software/Qt/Tools/mingw1310_64/bin/g++.exe `
  -DCMAKE_MAKE_PROGRAM=D:/Software/Qt/Tools/mingw1310_64/bin/mingw32-make.exe
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
```

## 接口

| 方法 | 路径 | 用途 |
|---|---|---|
| `GET` | `/health` | Runtime、Schema、Parser 和标准库状态 |
| `GET` | `/capabilities` | supported/partial/unsupported 能力矩阵 |
| `POST` | `/parse` | 输入 `{uri,text,requestId?}`，返回 ModelSnapshot |
| `POST` | `/analyze` | 解析并返回 AnalysisSnapshot |
| `POST` | `/cancel` | 协作式取消活动任务 |

运行：

```powershell
./run-runtime.ps1 -Port 47321
```

## 0.9.0 支持边界

- 包、需求、部件、端口、属性、连接、计算定义、分析、约束、View 和表格 Rendering 已有契约测试。
- 支持确定性稳定 ID、结构化诊断、嵌套 Package owner、局部类型和官方标准库类型解析。
- 支持标量表达式 AST、调用绑定、单位换算、量纲检查、约束判断、除零和循环诊断。
- 复杂泛化、完整 KerML 语义、跨文件工作区解析、高级集合表达式和标准库整体对象化仍为后续范围。
- SourceSpan 当前采用保守名称定位；重名或匿名元素仍可能报告 `AIDC_SOURCE_SPAN_PARTIAL`。

接口 Schema 与稳定契约位于 `../reference-parser-p0/model-snapshot.schema.json`、`analysis-snapshot.schema.json` 和 `parser-contract.md`。
