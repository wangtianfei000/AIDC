# AIDC Distribution Baseline

该目录保存 AIDC Engineering Tools 的公共产品契约和供应链基线。P0 冻结后续实现必须遵守的机器可读边界；P1 已增加统一 CLI 与运行管理器；P2 已增加现有工具的内部便携发行构建；P3 已增加 Runtime Pack 管理和安装器；P4 已增加宿主无关的 MCP stdio 服务和版本化 Skills；P5 已增加 Codex、Cursor 和通用 MCP 客户端适配。

P1 的开发入口位于 `bin/aidc.cmd`，可执行文件通过 `distribution/scripts/build_cli.ps1` 生成。运行管理器提供组件发现、动态端口、健康检查、本机会话令牌、服务复用和进程回收。

P2 使用 `distribution/scripts/build_portable.ps1` 构建三个前端、CLI、Modelica Project Server、Modelica 工具和三维装配导出器，并组装到被 Git 忽略的 `distribution/out/aidc-portable/`。正式运行只读取 `bin/`、`components/`、`distribution/`、`integrations/` 和 `skills/`，不调用 npm、Vite 或系统 Python。OpenModelica 和 FreeCAD 仍由 P3 Runtime Pack 提供；缺少 OMC 时 Modelica 浏览器以只读源码索引启动，编译仿真能力明确保持不可用。

P3 规定 `.aidcpack` 为自包含运行时包：根目录包含 `pack-manifest.json` 和 `payload/`，清单锁定平台、版本、入口、健康检查以及每个文件的大小和 SHA-256。`aidc deps install/verify/repair/rollback/uninstall` 负责断点下载、缓存、事务切换、安装收据与回滚快照。运行时固定安装到产品目录的 `runtimes/freecad`、`runtimes/openmodelica` 和 `runtimes/python`，不修改系统 PATH。未经许可证及摘要审核的包只能通过显式 `--source` 用于内部验收，默认目录不会绕过供应链门禁。

P4 的 `aidc-mcp` 使用 MCP JSON-RPC stdio，不监听网络端口。17 个工具覆盖系统、SysML、三维模型、Modelica、FreeCAD、任务和产物查询；长操作立即返回任务 ID，状态、取消、日志与产物索引持久化到用户状态目录。四个工程 Skills 由 `skills/catalog.json` 锁定版本和所需能力，并作为只读 MCP Resources 暴露。Skill 只引用稳定 MCP 工具或 `aidc` 命令，不要求用户运行仓库内 npm、Python 或 PowerShell 脚本。

P5 的 `aidc integrate codex|cursor|mcp` 将同一 `aidc-mcp` 注册到不同宿主。Codex 使用用户级 `config.toml` 托管区块，Cursor 合并用户级 `mcp.json`，通用模式输出可移植的 stdio 配置；Codex 和 Cursor 同时安装四个版本化 Skills。首次接入会保存宿主配置和同名 Skill 的基线备份。`--uninstall` 只移除 AIDC 管理项，`--restore` 显式恢复完整基线，`--check` 会启动配置中的服务并验证协议、17 个工具、四个资源和核心能力调用。

```text
AIDC/
├─ integrations/              # Codex、Cursor、通用 MCP 模板
└─ distribution/
   ├─ contracts/              # CLI、MCP、工程和产物目录
   ├─ manifests/              # AIDC 自研组件清单
   ├─ schemas/                # JSON Schema 2020-12
   ├─ audits/                 # 供应链和干净机器基线
   ├─ licenses/               # 第三方许可结论与待办
   ├─ runtime-packs/          # Runtime Pack 发布目录与在线目录
   ├─ installer/              # 在线、离线安装和卸载入口
   ├─ scripts/validate_p0.py  # P0 自动校验入口
   ├─ dependencies.lock.json  # 外部运行时锁定与阻断状态
   └─ sbom.cdx.json           # CycloneDX 供应链基线
```

验证：

```powershell
cd AIDC
python -X utf8 distribution/scripts/validate_p0.py
npm run verify:distribution:p0
npm run build:distribution:p2
npm run verify:distribution:p2
npm run verify:distribution:p3
npm run verify:distribution:p4
npm run verify:distribution:p5
```

宿主接入：

```powershell
aidc integrate codex
aidc integrate cursor
aidc integrate mcp
aidc integrate codex --check
aidc integrate cursor --uninstall
aidc integrate cursor --restore
```

默认命令不要求路径参数；`--config PATH` 仅用于企业托管目录、项目级配置或兼容性测试。Codex 的配置格式依据 [OpenAI 官方 MCP 文档](https://developers.openai.com/learn/docs-mcp)，Cursor 的配置与 Skills 目录依据 [Cursor MCP 文档](https://docs.cursor.com/context/model-context-protocol)和 [Cursor Skills 文档](https://cursor.com/docs/skills)。宿主适配只负责发现和注册，不复制 CLI、MCP 或模型处理逻辑。

内部构建与安装：

```powershell
python distribution/scripts/build_runtime_pack.py openmodelica D:\audited\OpenModelica distribution/cache/openmodelica-1.27.1.aidcpack
bin\aidc.cmd deps install openmodelica --source distribution/cache/openmodelica-1.27.1.aidcpack
bin\aidc.cmd deps verify openmodelica
python distribution/scripts/build_offline_bundle.py distribution/out/aidc-portable distribution/out/aidc-offline.zip --pack distribution/cache/openmodelica-1.27.1.aidcpack
```

校验器检查所有 JSON、JSON Schema、组件清单、CLI/MCP 目录、依赖引用、相对入口安全性，以及发布阻断项是否被审计报告明确记录。

当前 P0 基线版本为 `0.1.0`。公共契约的破坏性变化必须提升主版本并提供迁移说明。

