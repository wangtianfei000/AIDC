# SysML Engine 长期维护规则

`engine/` 是 AIDC 直接维护的解析内核。`P1-IMPORT.json` 和 `P2-IMPORT.json` 只记录首次迁入来源，不再作为当前源码不可修改的约束。迁移过程的详细证据已从工作树清理，必要时可通过 Git 历史恢复。

## 必须通过的门禁

提交引擎修改前执行：

```powershell
python -X utf8 tools/verify_engine_maintenance.py
./build-runtime.ps1
```

`verify_engine_maintenance.py` 同时检查以下规则：

1. `MAINTENANCE.json` 中的引擎版本、Grammar、12 个生成文件、ANTLR 版本和全部维护源码摘要与工作树一致。
2. 新增模型类必须在 `MODEL-MAINTENANCE.json.newModelCoverage` 中登记 Parser 构造测试和 ModelSnapshot 投影测试。
3. `engine/` 不得引用浏览器、桌面 UI、AIDC CLI、`aidc_core`、`RuntimeServer`、Qt Widgets/Gui/Network。
4. 当前版本必须对应维护历史中的最后一条记录。
5. `src/` Runtime 层不得重新实现单位目录、表达式求值或 Calculation/Constraint 语义；这些能力必须位于 `engine/core/semantic`、`engine/core/analysis` 或 `engine/services`。

## 参数分析边界

- `engine/core/semantic` 维护标准库符号、类型、单位表达式和量纲规则；
- `engine/core/analysis` 维护参数依赖、表达式求值、Calculation 调用和 Constraint 判断；
- `engine/services` 是解析与分析门面，并负责稳定快照投影；
- `src/RuntimeServer` 只负责 HTTP、并发、取消、超时和请求校验。

增加单位或修改分析语义时必须修改内核及其测试。禁止在 Runtime 或浏览器中通过单位字符串白名单、公式正则或结果修正规则弥补内核缺口。

CTest 中的 `aidc-p5-engine-maintenance` 会再次执行同一门禁，发行构建无法绕过。

## 修改 Grammar 或 Parser

Grammar 与生成代码必须在同一提交更新。常规构建使用已经提交的生成代码，不需要 Java；只有修改 `SysML.g4` 时执行：

```powershell
./tools/regenerate-parser.ps1 `
  -AntlrJar <antlr-4.13.2-complete.jar> `
  -AcceptChanges `
  -EngineVersion <新的x.y.z版本>
```

脚本固定 ANTLR Tool 4.13.2、核对 12 个文件、规范化生成来源路径，并更新 `MAINTENANCE.json`。如果源码摘要发生变化但版本没有递增，更新器会失败。

## 修改模型层

新增 `engine/core/model/Foo.h/.cpp` 时，需要：

1. 在 `P2EmbeddedParserTests.cpp` 或等价 C++ 测试中加入能由 SysML 文本构造 `Foo` 的用例。
2. 在 `RuntimeTests.cpp` 或等价测试中验证 `Foo` 到 ModelSnapshot 的元素、关系和关键属性投影。
3. 在 `MODEL-MAINTENANCE.json.newModelCoverage.Foo` 中记录两个测试文件及可搜索的测试标记。
4. 使用新的引擎版本更新维护清单：

```powershell
python -X utf8 tools/update_engine_maintenance.py `
  --engine-version <新的x.y.z版本> `
  --note "Add Foo model and parser/snapshot coverage"
```

修改现有模型、Visitor、Serializer 或 CMake 时同样要更新引擎版本和维护清单，并增加覆盖行为变化的测试。

## 版本与来源

- `runtimeVersion` 表示 HTTP Runtime 的发行版本。
- `engineVersion` 表示解析内核版本。
- `schemaVersion` 表示 JSON 契约版本。
- `/health`、`/capabilities` 和 `parserBaseline` 同时暴露内核版本和源码摘要。
- 首次来源和以后每次上游同步记录在 `ORIGIN.md`；每次本地引擎修订记录在 `MAINTENANCE.json.history`。

需要同步上游时先在独立分支形成补丁，逐项合入 `engine/`，补测试并递增引擎版本。禁止重新引入外部源码路径、外部构建目录或预编译 Parser 静态库。
