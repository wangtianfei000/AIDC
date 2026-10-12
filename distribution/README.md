# AIDC Distribution Baseline

该目录保存 AIDC Engineering Tools 的公共产品契约和供应链基线。P0 冻结后续实现必须遵守的机器可读边界；P1 已增加统一 CLI 与运行管理器；P2 已增加现有工具的内部便携发行构建；P3 已增加 Runtime Pack 管理和安装器，尚不包含 MCP 服务。

P1 的开发入口位于 `bin/aidc.cmd`，可执行文件通过 `distribution/scripts/build_cli.ps1` 生成。运行管理器提供组件发现、动态端口、健康检查、本机会话令牌、服务复用和进程回收。

P2 使用 `distribution/scripts/build_portable.ps1` 构建三个前端、CLI、Modelica Project Server、Modelica 工具和三维装配导出器，并组装到被 Git 忽略的 `distribution/out/aidc-portable/`。正式运行只读取 `bin/`、`components/`、`distribution/` 和 `skills/`，不调用 npm、Vite 或系统 Python。OpenModelica 和 FreeCAD 仍由 P3 Runtime Pack 提供；缺少 OMC 时 Modelica 浏览器以只读源码索引启动，编译仿真能力明确保持不可用。

P3 规定 `.aidcpack` 为自包含运行时包：根目录包含 `pack-manifest.json` 和 `payload/`，清单锁定平台、版本、入口、健康检查以及每个文件的大小和 SHA-256。`aidc deps install/verify/repair/rollback/uninstall` 负责断点下载、缓存、事务切换、安装收据与回滚快照。运行时固定安装到产品目录的 `runtimes/freecad`、`runtimes/openmodelica` 和 `runtimes/python`，不修改系统 PATH。未经许可证及摘要审核的包只能通过显式 `--source` 用于内部验收，默认目录不会绕过供应链门禁。

```text
distribution/
├─ contracts/                 # CLI、MCP、工程和产物目录
├─ manifests/                 # AIDC 自研组件清单
├─ schemas/                   # JSON Schema 2020-12
├─ audits/                    # 供应链和干净机器基线
├─ licenses/                  # 第三方许可结论与待办
├─ runtime-packs/             # Runtime Pack 发布目录与在线目录
├─ installer/                 # 在线、离线安装和卸载入口
├─ scripts/validate_p0.py     # P0 自动校验入口
├─ dependencies.lock.json     # 外部运行时锁定与阻断状态
└─ sbom.cdx.json              # CycloneDX 供应链基线
```

验证：

```powershell
cd AIDC
python -X utf8 distribution/scripts/validate_p0.py
npm run verify:distribution:p0
npm run build:distribution:p2
npm run verify:distribution:p2
npm run verify:distribution:p3
```

内部构建与安装：

```powershell
python distribution/scripts/build_runtime_pack.py openmodelica D:\audited\OpenModelica distribution/cache/openmodelica-1.27.1.aidcpack
bin\aidc.cmd deps install openmodelica --source distribution/cache/openmodelica-1.27.1.aidcpack
bin\aidc.cmd deps verify openmodelica
python distribution/scripts/build_offline_bundle.py distribution/out/aidc-portable distribution/out/aidc-offline.zip --pack distribution/cache/openmodelica-1.27.1.aidcpack
```

校验器检查所有 JSON、JSON Schema、组件清单、CLI/MCP 目录、依赖引用、相对入口安全性，以及发布阻断项是否被审计报告明确记录。

当前 P0 基线版本为 `0.1.0`。公共契约的破坏性变化必须提升主版本并提供迁移说明。

