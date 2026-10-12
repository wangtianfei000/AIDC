# 参考 C++ Parser 缺口清单

## P0 结论

当前 Parser 可作为起始代码，但不能原样作为需求浏览器解析内核。它具备 Package、基础需求元素、Part/Port 和若干视图元素的恢复能力；稳定标识、源码定位和需求追溯链路尚未形成。

## 阻断需求 MVP 的缺口

| 优先级 | 缺口 | 证据 | 后续处理 |
|---|---|---|---|
| P0-blocker | 元素没有源码范围 | `Element` 无行、列、offset；仅错误监听器保存行列 | 在 Visitor 建模时保存 token start/stop，Adapter 输出 `SourceSpan` |
| P0-blocker | 元素没有稳定 ID | `Element` 无 id/uuid | P1 先生成确定性 ID；后续支持模型内显式标识 |
| P0-blocker | verify/verification 不存在 | 语法和 Visitor 均未检出 | 按目标 SysML v2 子集补语法、元模型、Visitor 和夹具 |
| P0-blocker | derive 不存在 | 语法和 Visitor 均未检出 | 同上 |
| P0-blocker | refine 不存在 | 语法和 Visitor 均未检出 | 同上 |
| P0-blocker | satisfy 仅局部实现 | 只在 View body 处理；复杂形式有 TODO | 改为通用显式关系并覆盖需求到架构场景 |
| P0-blocker | 许可证不明确 | 仓库根目录无 LICENSE/COPYING/NOTICE | 对外分发源码或二进制前确认权属与授权 |
| P1 | 诊断不是结构化对象 | API 返回格式化 `QStringList` | 分离 code、severity、message、range |
| P1 | 错误恢复可能被误判成功 | 有语法错误仍返回非空 Resource | Contract 以 diagnostics 判定，UI 显示不完整模型状态 |
| P1 | 跨文件解析不完整 | 外部类型大量警告；Package 导入查找有 TODO | 建立项目级 URI、导入和符号解析器 |
| P1 | 标准库未加载 | String、Real、ISQ 等均产生警告 | 配置标准库搜索路径和版本，区分外部未加载与真正缺失 |
| P1 | Parser 依赖范围偏大 | 当前链接 Qt Gui 和 architecture metadata | 抽离颜色/图形注解，运行时目标仅保留必要依赖 |
| P1 | Parser 测试未接入顶层构建 | `parser/tests` 未被顶层 CMake 纳入 | 将 Parser contract tests 接入 CI/CTest |
| P2 | connection end 未完成 | Visitor 中有显式 TODO | 在架构视图阶段补齐并验证端点 |
| P2 | ViewDefinition 绑定未完成 | ViewUsage 查找有 TODO | 建立统一符号解析后处理 |

## 样例与覆盖缺口

- `Jingang` 和 `AIDC` 当前未发现 `.sysml`/`.kerml` 业务模型，因此 P0 无法形成金刚业务样例人工真值。
- 参考项目需求夹具本身包含 Parser 不接受的语法；应把“夹具意图”“目标标准语法”“当前 Parser 行为”分别记录。
- 当前测试重在对象是否创建，未核验需求字段、关系端点、限定名、源码位置和序列化完整性。
- P1 开始前应建立最小业务夹具：需求定义、需求实例、subject/stakeholder、约束、satisfy、verify、derive、refine、验证用例和跨文件引用。

