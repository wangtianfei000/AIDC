# SysML 模型层来源

`core/common`、`core/model` 和 `core/infrastructure` 来自 P0 冻结的本地参考工作树：

- 上游：`https://gitee.com/wtf0/sys-ml.git`
- 基准提交：`9bfba0bced98b7586bab8a6d45746064aff70fe1`
- 实际来源：该提交加 P0 保存的本地补丁和未跟踪源码
- 首次导入：2026-10-13
- 文件级来源与 SHA-256：`../P1-IMPORT.json`

P1 迁入模型层；P2 继续迁入 Grammar、对应的 12 个 ANTLR 4.13.2 生成文件、`SysMLTextParser`、`ModelBuilderVisitor` 和独立 Serializer。P2 文件级来源、摘要和排除项见 `../P2-IMPORT.json`。

桌面 UI、参考构建产物以及依赖架构图 UI metadata 的 `AnnotationParser` 未迁入核心 target。参考项目许可证仍为“待定”，因此公开发行继续受供应链闸门约束。
