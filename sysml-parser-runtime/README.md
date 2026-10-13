# AIDC SysML Parser Runtime 1.0

本目录提供完全不依赖 Qt 的只读 SysML Parser、分析引擎和本机 HTTP Runtime。解析、模型投影、JSON、HTTP、并发与测试均使用 Python 3.12 标准库实现；发行时通过 PyInstaller 冻结为单文件 EXE，目标机器不需要安装 Python、Qt、CMake、编译器或外部参考 Parser。

## 组成

- `src/aidc_sysml_runtime/model.py`：词法扫描、递归结构解析、稳定 ID、源码位置、连接、Calculation 和 View 投影。
- `src/aidc_sysml_runtime/analysis.py`：表达式、单位、量纲、依赖、约束和异常状态计算。
- `src/aidc_sysml_runtime/server.py`：仅监听 loopback 的 HTTP 服务、并发请求、请求限制、Origin 校验和取消。
- `tests/test_runtime.py`：解析、连接、视图、计算和机器人正式样例的源码级契约测试。
- `tests/http_smoke.py`：对冻结 EXE 执行 JSON Schema、确定性、并发、取消和安全边界黑盒测试。

## 构建

开发机需要 Python 3.12 与 PyInstaller：

```powershell
npm run build:sysml-runtime
```

脚本先执行源码级契约测试，再生成 `dist/bin/aidc-sysml-parser-runtime.exe` 和带版本名的 `aidc-sysml-parser-runtime-1.0.0.exe`。构建脚本会检查 `dist`，发现 `Qt*`、`PySide*` 或 `PyQt*` 文件即失败。发行 EXE 使用 Windows 系统网络栈和 Python 冻结运行时，不加载 Qt DLL。

## 接口

| 方法 | 路径 | 用途 |
|---|---|---|
| `GET` | `/health` | Runtime、Parser 和无 Qt 状态 |
| `GET` | `/capabilities` | 能力矩阵 |
| `POST` | `/parse` | 输入 `{uri,text,requestId?}`，返回 ModelSnapshot |
| `POST` | `/analyze` | 解析并返回 AnalysisSnapshot |
| `POST` | `/cancel` | 取消活动任务 |

运行：

```powershell
./run-runtime.ps1 -Port 47321
```

## 1.0 支持边界

- 支持包、需求、部件、端口、属性、连接、Calculation、Analysis、Constraint、View 和表格 Rendering 的只读子集。
- 支持稳定 ID、结构化诊断、嵌套 Package、局部类型、标量表达式、单位换算、量纲检查、约束余量、除零与循环诊断。
- 复杂泛化、完整 KerML 语义、跨文件工作区解析、高级集合表达式仍属于后续范围。

接口 Schema 与稳定契约位于 `../reference-parser-p0/model-snapshot.schema.json`、`analysis-snapshot.schema.json` 和 `parser-contract.md`。
