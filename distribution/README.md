# AIDC Distribution Baseline

该目录保存 AIDC Engineering Tools 的公共产品契约和供应链基线。P0 不实现最终 CLI、MCP 服务或安装器；它冻结后续实现必须遵守的机器可读边界。

```text
distribution/
├─ contracts/                 # CLI、MCP、工程和产物目录
├─ manifests/                 # AIDC 自研组件清单
├─ schemas/                   # JSON Schema 2020-12
├─ audits/                    # 供应链和干净机器基线
├─ licenses/                  # 第三方许可结论与待办
├─ scripts/validate_p0.py     # P0 自动校验入口
├─ dependencies.lock.json     # 外部运行时锁定与阻断状态
└─ sbom.cdx.json              # CycloneDX 供应链基线
```

验证：

```powershell
cd AIDC
python -X utf8 distribution/scripts/validate_p0.py
npm run verify:distribution:p0
```

校验器检查所有 JSON、JSON Schema、组件清单、CLI/MCP 目录、依赖引用、相对入口安全性，以及发布阻断项是否被审计报告明确记录。

当前 P0 基线版本为 `0.1.0`。公共契约的破坏性变化必须提升主版本并提供迁移说明。

