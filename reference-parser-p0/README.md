# 参考 Parser P0 复现说明

本目录保存参考 C++ Parser 的只读基线、独立探针、运行结果、能力清单、缺口清单和 Parser Contract。P0 没有修改参考项目工作树。

主要文件：

- `source-manifest.json`：commit、工具链、许可证状态和关键文件哈希。
- `parser-capabilities.json`：机器可读能力矩阵。
- `parser-gaps.md`：按阶段整理的缺口。
- `model-snapshot.schema.json`：Parser 与 Web 浏览器之间的 IR Schema v0.1。
- `parser-contract.md`：确定性、错误恢复和缺失能力规则。
- `probe/`：独立构建的 C++ 动态探针及参考 Parser 测试包装。
- `results/`：实际输出，包含 6 个样例、无效语法、4 个核心单元测试和参考 Parser 测试。

构建时需要 Qt 6.8.1 MinGW、GCC 13.1、CMake 和 Ninja，并通过 `REFERENCE_ROOT`、`REFERENCE_BUILD` 指向参考项目及其现有构建目录。

参考复现命令（PowerShell）：

```powershell
cmake -S probe -B probe/build -G Ninja `
  -DCMAKE_PREFIX_PATH=D:/Software/Qt/6.8.1/mingw_64 `
  -DCMAKE_CXX_COMPILER=D:/Software/Qt/Tools/mingw1310_64/bin/g++.exe `
  -DREFERENCE_ROOT=D:/Document/7.AWorks/BlockSim/2.Development/SysML `
  -DREFERENCE_BUILD=D:/Document/7.AWorks/BlockSim/2.Development/SysML/build/Desktop_Qt_6_8_1_MinGW_64_bit-Debug
cmake --build probe/build --parallel 4
```

`reference_parser_probe` 对 6 个样例的预期退出码是 1，因为其中 3 个样例包含当前 Parser 报出的语法错误。`reference_parser_unit` 的预期结果为 6 passed / 2 failed；该失败是本次基线的一部分。
