# AIDC — AI-Driven Creation

本仓库保存 AIDC 工程工具的源码、契约、测试和必要样例。第三方软件源码、下载归档、安装包、依赖目录和构建结果不属于仓库源码。

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

## 源码结构

| 目录 | 模块 | 用途 |
|---|---|---|
| [aidc_core/](aidc_core/) | AIDC 运行核心 | CLI、任务、进程、依赖和运行时管理 |
| [distribution/](distribution/) | 发行工程 | 组件清单、供应链契约、构建、安装及验收脚本 |
| [integrations/](integrations/) | AI 宿主适配 | Codex、Cursor 和通用 MCP 配置生成 |
| [skills/](skills/) | Skills | 面向 AI 工具的标准工作流 |
| [sysml-parser-runtime/](sysml-parser-runtime/) | AIDC SysML Parser Runtime | 将 SysML 转换为只读快照和分析服务 |
| [sysml-browser/](sysml-browser/) | AIDC SysML 浏览器 | 需求、架构、参数分析、属性和源码视图 |
| [model3d-viewer/](model3d-viewer/) | AIDC 三维模型浏览器 | CAD 物理结构浏览、运动学、干涉校核、爆炸图和装配模拟 |
| [modelica-browser/](modelica-browser/) | AIDC Modelica 浏览器 | Modelica 模型浏览、生成、检查和仿真 |
| [samples/](samples/) | 工程样例 | 端到端流程所需的受控输入资产 |
| [tests/](tests/) | 发行验收 | P1-P6 自动化测试 |

## 本机依赖与清理

FreeCAD、OpenModelica、Python 和 SysML Parser 的版本、摘要、许可证状态及安装入口统一由 `distribution/dependencies.lock.json` 和 `aidc deps` 管理。开发环境的 FreeCAD 位于同级 `tools/freecad/1.1.3/`，OpenModelica 位于 `tools/openmodelica/1.27.1/`；正式安装使用 `.aidcpack` 和产品运行时目录。第三方运行时、源码镜像、下载缓存及社区 MCP 副本不再放在 AIDC 根目录。

预览或清理可重建内容：

```powershell
npm run clean:preview
npm run clean
npm run clean:legacy
```

常规清理会删除包管理器依赖目录、编译输出、测试结果和缓存；`clean:legacy` 额外删除旧的 FreeCAD 源码镜像、根目录运行时、下载缓存、社区 MCP 副本和快捷方式。两种模式均保留产品源码、锁文件、标准库、样例输入和已安装的 `runtimes/`，也不会操作仓库之外的 `tools/`。依赖目录可用 `npm ci` 恢复，发行物可按使用说明重新构建。
