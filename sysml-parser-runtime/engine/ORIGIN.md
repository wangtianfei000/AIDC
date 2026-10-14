# SysML 模型层来源

`core/common`、`core/model` 和 `core/infrastructure` 来自 P0 冻结的本地参考工作树：

- 上游：`https://gitee.com/wtf0/sys-ml.git`
- 基准提交：`9bfba0bced98b7586bab8a6d45746064aff70fe1`
- 实际来源：该提交加 P0 保存的本地补丁和未跟踪源码
- 首次导入：2026-10-13
- 文件级来源与 SHA-256：`../P1-IMPORT.json`

P1 迁入模型层；P2 继续迁入 Grammar、对应的 12 个 ANTLR 4.13.2 生成文件、`SysMLTextParser`、`ModelBuilderVisitor` 和独立 Serializer。P2 文件级来源、摘要和排除项见 `../P2-IMPORT.json`。

桌面 UI、历史构建产物以及依赖架构图 UI metadata 的 `AnnotationParser` 未迁入核心 target。P4 起这些源码由 AIDC 仓库直接维护，构建和运行不再访问原项目；其历史来源和许可仍需在公开发行前完成复核。

## 同步记录

| 日期 | 引擎版本 | 来源 | 结果 |
|---|---|---|---|
| 2026-10-13 | 0.2.0 | P0 冻结工作树，基准提交 `9bfba0bced98b7586bab8a6d45746064aff70fe1` | 迁入模型层、Grammar、ANTLR 生成代码、Parser、Visitor 与 Serializer |
| 2026-10-14 | 0.2.1 | AIDC 本地维护 | 建立 P5 维护清单、版本门禁和稳定的生成文件来源路径；无语法或解析行为变化 |

以后从上游或其他来源同步时，必须在此追加来源提交、选择的文件范围、冲突处理和行为验证结果。
