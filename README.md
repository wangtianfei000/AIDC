# AIDC — AI-Driven Creation

本目录用于集中存放 AI 驱动创作所使用的软件、软件源码及必要的软件说明。

面向 Codex、Cursor 等 AI 主机的统一 CLI、MCP、安装和依赖管理规划见 [AIDC AI 工具插件化与正式发行实施计划](doc/AIDC全套软件产品化实施计划.md)。

安装、宿主接入、完整工程流程和故障处理见 [AIDC Engineering Tools 使用说明](doc/AIDC工程工具使用说明.md)。

P0 产品契约与供应链基线位于 [distribution/](distribution/README.md)，可运行 `npm run verify:distribution:p0` 验证。

## 统一 CLI

开发工作区可直接运行：

```powershell
.\bin\aidc.cmd doctor
.\bin\aidc.cmd deps status
.\bin\aidc.cmd sysml open .\samples\jingang-10axis-system-design\architecture.sysml
.\bin\aidc.cmd model3d open .\samples\jingang-10axis-system-design\model3d
.\bin\aidc.cmd modelica open .\samples\modelica-t2-single-joint
.\bin\aidc.cmd browser status
.\bin\aidc.cmd browser stop --all
```

CLI 自动分配端口、执行健康检查、复用相同工程实例，并在 `%LOCALAPPDATA%\AIDC\state` 记录任务、进程和脱敏日志。`--json` 可以放在命令任意位置，输出 `aidc.cli-result/1.0` 结构。

## 存放范围

- 每款软件使用独立子目录，保留其上游目录结构和许可证。
- 本目录只存放软件相关内容；机器人零件、CAD 设计文件、STL 模型、渲染结果、打印文件及其他创作产物应存放在对应项目目录中。
- 软件源码与已安装的可执行程序应明确区分；克隆源码不代表已完成安装。

## 软件清单

| 目录 | 软件 | 用途 | 来源与状态 |
|---|---|---|---|
| [FreeCAD/](FreeCAD/) | FreeCAD | 开源参数化机械 CAD，可用于零件设计、装配及 Python 自动化 | [官方源码仓库](https://github.com/FreeCAD/FreeCAD)，源码检出，不是已安装程序 |
| [FreeCAD-runtime/](FreeCAD-runtime/) | FreeCAD 1.1.3 Windows x64 | 可直接运行的官方免安装版 | [官方发行页](https://github.com/FreeCAD/FreeCAD/releases/tag/1.1.3)，与开发分支源码独立 |
| [FreeCAD-downloads/](FreeCAD-downloads/) | FreeCAD 下载包 | 保留官方发行归档，便于重新部署 | SHA-256 已核验 |
| [freecad-mcp/](freecad-mcp/) | FreeCAD MCP 0.1.22 | AI 通过 MCP 读取文档、操作模型和获取视图 | [社区上游](https://github.com/neka-nat/freecad-mcp)，本地提交 `3da6db5`；独立 Python 环境位于 `.venv/` |
| [sysml-parser-runtime/](sysml-parser-runtime/) | AIDC SysML Parser Runtime 0.8.0 | 将参考 C++ Parser 转换为浏览器可用的只读 ModelSnapshot 与 AnalysisSnapshot HTTP 服务 | 已加载官方 SysML 2026-08 模型库，支持架构语义、Table View、Calculation 及单位感知分析；Release 免安装包位于 `dist/`，参考 Parser 许可证待确认 |
| [sysml-browser/](sysml-browser/) | AIDC SysML 浏览器 0.9.0 | 将 SysML 文本渲染为需求、架构、参数分析、属性和源码视图 | 独立软件，生产构建位于 `dist/` |
| [model3d-viewer/](model3d-viewer/) | AIDC 三维模型浏览器 0.1.0 | CAD 物理结构浏览、运动学、干涉校核、爆炸图和装配模拟 | 独立软件；包含模型包契约、FreeCAD 发布器和 Three.js 运行时 |

FreeCAD MCP 的部署位置、启动方式和验证方法见 [MCP 接入说明](FreeCAD-runtime/MCP接入说明.md)。

## 运行 FreeCAD

双击本目录中的 `启动 FreeCAD 1.1.3.lnk`，或运行：

```text
FreeCAD-runtime/FreeCAD_1.1.3-Windows-x86_64-py311/FreeCAD.exe
```

该程序来自官方预编译发行包，并非由本目录的开发分支源码构建。软件已具备现成 Windows 版本，因此无需为使用软件而重新编译。

部署日期：2026-09-09。原始归档：`FreeCAD_1.1.3-Windows-x86_64-py311.7z`。

SHA-256：`9c6959dc9c4dba64dd818a62447e3dfedb4221d776fb044b239d462f150bcec4`，与 GitHub 官方发行资产摘要一致。

已通过命令行建模验证：创建参数化方块、改变尺寸并重算、检查实体及体积、保存并重新打开 FCStd、导出 STEP 和 STL。验证脚本为 `FreeCAD-runtime/smoke_test.py`，测试模型仅写入临时目录并自动清理。

## FreeCAD 源码获取与更新

首次获取采用浅克隆，下载默认分支的最新提交及对应源码，不下载完整历史：

```powershell
git clone --depth 1 https://github.com/FreeCAD/FreeCAD.git FreeCAD
```

在 `AIDC` 目录内执行以下命令可更新源码；更新前先确认本地修改已妥善保存：

```powershell
git -C FreeCAD status --short
git -C FreeCAD pull --ff-only
```

默认开发分支不等同于稳定发行版。构建环境、第三方依赖和子模块应根据 FreeCAD 官方构建文档另行配置；此处仅保存软件源码。
