# AIDC Engineering Tools 使用说明

版本：0.6.0
适用平台：Windows x86-64
发行状态：工程内测通过；公开发行仍受供应链闸门阻断

## 1. 产品是什么

AIDC Engineering Tools 是供 Codex、Cursor、通用 MCP 客户端和终端调用的本地机器人工程工具。它提供统一 CLI、MCP 服务、工程 Skills，以及按需启动的 SysML、三维模型和 Modelica 浏览器。它不是独立桌面工作台，也不会替代 AI 主机的对话、代码编辑和版本管理。

用户只需认识两个程序：

- `aidc.exe`：终端、脚本和 CI 的统一入口。
- `aidc-mcp.exe`：由 Codex、Cursor 等宿主自动启动，通常不手工运行。

浏览器服务只监听本机 `127.0.0.1`，自动选择空闲端口并执行健康检查。FreeCAD 和 OpenModelica 以受管 Runtime Pack 安装，不要求用户配置系统 PATH。

## 2. 安装与首次检查

在源码开发工作区中，双击 `bin\aidc.cmd` 会显示交互菜单。菜单只启动空白浏览器，模型和工程在对应软件的“打开”入口中选择，不需要先在命令行输入路径。也可以在终端中给它传入与 `aidc.exe` 相同的参数，例如 `bin\aidc.cmd doctor`；`sysml launch`、`model3d launch` 和 `modelica launch` 分别只启动对应软件。正式发行包使用下述 `aidc.exe`，不需要安装 Python。

当前 0.6.0 为内部便携发行。解压到固定目录，或由内部安装脚本安装：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\distribution\installer\install.ps1 `
  -Source .\distribution\out\aidc-portable
```

默认安装位置为 `%LOCALAPPDATA%\Programs\AIDC`。安装后执行：

```powershell
$aidc = "$env:LOCALAPPDATA\Programs\AIDC\bin\aidc.exe"
& $aidc version
& $aidc doctor
& $aidc deps status
```

`doctor` 检查核心、三个浏览器和 SysML Runtime。`deps status` 额外显示 FreeCAD、OpenModelica 与 Python Runtime Pack 的状态。普通用户无需输入依赖安装路径；内部验收人员可以使用经过审计的 `.aidcpack`：

```powershell
& $aidc deps install freecad --source D:\packs\freecad-1.1.3.aidcpack
& $aidc deps install openmodelica --source D:\packs\openmodelica-1.27.1.aidcpack
& $aidc deps verify freecad
& $aidc deps verify openmodelica
```

公开下载目录在供应链闸门关闭前不会提供未审计的运行时包。不要把开发机上的 FreeCAD 或 OMC 目录直接复制给最终用户。

## 3. 接入 Codex、Cursor 和其他 MCP 客户端

安装完成后运行一次接入命令：

```powershell
& $aidc integrate codex
& $aidc integrate cursor
```

命令会写入 AIDC 管理区块并安装配套 Skills，同时保留原配置基线。接入后检查实际协议兼容性：

```powershell
& $aidc integrate codex --check
& $aidc integrate cursor --check
```

检查会真实启动 `aidc-mcp`，验证 17 个工具、4 个 Skill Resources 和核心能力调用。通用 MCP 客户端可生成标准配置：

```powershell
& $aidc integrate mcp --config D:\config\aidc-mcp.json
& $aidc integrate mcp --check --config D:\config\aidc-mcp.json
```

移除 AIDC 管理项时不会修改宿主的其他 MCP 服务：

```powershell
& $aidc integrate codex --uninstall
& $aidc integrate cursor --uninstall
```

`--restore` 会恢复首次接入前保存的完整基线，适合宿主配置异常时使用：

```powershell
& $aidc integrate codex --restore
```

## 4. 标准工程流程

路径可以包含中文和空格。PowerShell 中请用引号包住路径。

### 4.1 SysML 校验与浏览

```powershell
& $aidc sysml validate "D:\机器人 项目\architecture.sysml"
& $aidc sysml open "D:\机器人 项目\architecture.sysml"
```

校验返回元素数、关系数和诊断；打开命令启动只读浏览器。AI 主机中可直接要求“用 AIDC 校验并打开这个 SysML 文件”。

### 4.2 三维工程发布与浏览

三维工程应包含 `generated/model3d.manifest.json`。由 FreeCAD 发布配置生成正式三维发布物：

```powershell
& $aidc model3d publish "D:\机器人 项目\model3d\publish.config.json" "D:\机器人 项目\published"
& $aidc model3d open "D:\机器人 项目\published"
```

发布目录包括 `visual.glb`、`collision.glb`、`kinematics.json`、`model3d.manifest.json` 和 `publish-report.json`。浏览器读取运动学、姿态、干涉与几何数据；原始 CAD 工程仍由用户项目保存。

### 4.3 导出装配并生成 Modelica

三维工程还应包含 Robot IR 的 `physics.json`、`devices.json` 和 `interfaces.json`。导出和生成命令为：

```powershell
& $aidc model3d export-modelica "D:\机器人 项目\published" "D:\机器人 项目\robot.aidc-assembly"
& $aidc modelica generate "D:\机器人 项目\robot.aidc-assembly" "D:\机器人 项目\modelica" --profile p3
& $aidc modelica validate "D:\机器人 项目\modelica"
& $aidc modelica open "D:\机器人 项目\modelica"
```

`.aidc-assembly` 是三维发布与 Modelica 生成之间的稳定交付物，包含唯一清单、Robot IR、必要几何和摘要，不保存构建缓存。

### 4.4 OMC 仿真与结果包

已安装并验证 OpenModelica Runtime Pack 后运行：

```powershell
& $aidc modelica simulate "D:\机器人 项目\modelica"
& $aidc modelica package "D:\机器人 项目\modelica" "D:\机器人 项目\robot.aidcmpkg" --include-results
```

成功仿真会在工程 `results` 下生成一次运行目录，至少包含 `run-manifest.json` 和 `series.csv`。未安装 OMC 时返回 `DEPENDENCY_MISSING`，Modelica 工程仍可生成、校验和只读浏览。

### 4.5 FreeCAD 检查与打开

```powershell
& $aidc freecad check "D:\机器人 项目\assembly.FCStd"
& $aidc freecad open "D:\机器人 项目\assembly.FCStd"
```

`check` 使用受管 FreeCAD 的无界面入口，适合 AI 和 CI；`open` 启动同一版本的图形入口供人工修改。

### 4.6 精确几何后端验证

`geometry.backend` 当前注册两个实现：`freecad-occ` 是默认生产后端，`usd-brep` 是实验后端。查看运行时状态并执行同一能力基线：

```powershell
& $aidc geometry backends
& $aidc geometry validate --backend freecad-occ --report ".\freecad-validation.json"
& $aidc geometry validate --backend usd-brep --report ".\usd-brep-validation.json"
```

验证覆盖基础实体、布尔求交、距离、体积和网格化；USD BRep 还验证精确 BRep 的 OpenUSD 往返。实验后端验证通过不代表已经切换生产发布或干涉校核，具体接口和准入边界见 [几何后端接口与能力验证](几何后端接口与能力验证.md)。

## 5. 浏览器、任务与产物管理

```powershell
& $aidc browser status
& $aidc browser stop <instance-id>
& $aidc browser stop --all
& $aidc task status <task-id>
& $aidc task cancel <task-id>
& $aidc artifacts list
& $aidc artifacts get <artifact-id>
```

同一类型和工程的运行实例会复用。服务异常退出后，`browser status` 会标记为 `stopped`；再次执行相应 `open` 命令即可重启。长任务由 MCP 返回 `taskId`，完成后通过任务和产物工具读取结果。

自动化脚本应添加 `--json`。参数可以放在命令末尾：

```powershell
& $aidc doctor --json
```

输出遵循 `aidc.cli-result/1.0`，失败时同时返回非零退出码和稳定诊断码。

## 6. 依赖维护

```powershell
& $aidc deps status
& $aidc deps verify openmodelica
& $aidc deps repair openmodelica --source D:\packs\openmodelica-1.27.1.aidcpack
& $aidc deps rollback openmodelica
& $aidc deps uninstall openmodelica
```

Runtime Pack 安装时校验每个文件摘要并写入收据。升级采用暂存和事务切换；失败时恢复上一版本。`repair` 重新安装同一依赖，`rollback` 切回最近快照，`uninstall` 只删除指定受管运行时。

离线环境使用内部生成的离线包。解压后运行其中的 `install.ps1`，包内产品和 Runtime Pack 均通过摘要清单校验；不需要 Git、Node.js、Python、CMake 或 Visual Studio。

## 7. 诊断与常见问题

生成诊断报告：

```powershell
& $aidc doctor --report "$env:TEMP\aidc-diagnostics.json"
```

运行状态和日志默认位于 `%LOCALAPPDATA%\AIDC\state`。日志不应包含会话令牌、用户凭据或模型全文。

| 现象/诊断码 | 处理方法 |
|---|---|
| `DEPENDENCY_MISSING` | 运行 `aidc deps status`，安装相应受管 Runtime Pack |
| `DEPENDENCY_UNHEALTHY` | 运行 `aidc deps verify <id>` 查看缺失或摘要错误，再执行 `repair` 或 `rollback` |
| `INPUT_INVALID` | 检查工程清单、Robot IR 或 `model-project.json` 是否完整 |
| `SERVICE_START_FAILED` / `HEALTH_TIMEOUT` | 查看状态目录下 `logs`，运行 `browser stop --all` 后重试 |
| 端口已占用 | AIDC 会自动选择其他本机端口；无需改配置 |
| 宿主检查失败 | 运行 `integrate <host> --check`；配置损坏时用 `--restore` 恢复基线后重新接入 |
| OMC 仿真失败 | 保留生成工程和结果目录，检查命令返回的具体阶段与日志，不要覆盖用户工程 |

## 8. 升级与卸载

使用安装器覆盖安装时，既有 Runtime Pack 会被迁移到新版本；升级失败自动恢复旧核心和旧运行时。用户工程不应放在安装目录中。

卸载：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\distribution\installer\uninstall.ps1
```

默认行为：移除 AIDC 核心和宿主管理项，保留用户工程、`%LOCALAPPDATA%\AIDC\state` 和 Runtime Pack 备份。只有明确使用 `-RemoveRuntimePacks` 才删除运行时。

## 9. 发布验收

内部发行构建和 P6 验收由发布工程人员执行：

```powershell
npm run build:distribution:p2
npm run verify:release:p6
```

P6 使用冻结版可执行文件完成 SysML、机器鸭三维发布数据、`.aidc-assembly`、Modelica 生成、OMC 仿真和 FreeCAD 发布闭环，并测试故障恢复、离线、中文路径、升级回滚和卸载。结果保存在 `distribution/audits/p6-release-acceptance.json`。

当前 `passed-internal` 仅表示工程内测验收通过。`distribution/audits/supply-chain-gates.json` 中的关键阻断项关闭前，不得把内部便携目录或开发机运行时作为公开发行包。
