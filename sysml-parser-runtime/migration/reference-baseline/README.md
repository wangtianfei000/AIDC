# SysML 参考解析器实际基线

这里冻结了 AIDC 在迁入解析器源码前已经验证过的本地参考实现。基线取自本地工作树，不只取自 Git 提交，因此包含当前有效的已跟踪修改与未跟踪源码。

- 上游仓库：`https://gitee.com/wtf0/sys-ml.git`
- 上游提交：`9bfba0bced98b7586bab8a6d45746064aff70fe1`
- 源码清单：554 个文件，共 2,219,084 字节
- 范围内已修改文件：11 个
- 保留的未跟踪源码：62 个
- ANTLR 生成文件：12 个
- 已验证静态库：7 个（只记录哈希，不复制构建产物）
- 许可证状态：参考项目 README 标注为“待定”，公开发行仍需确认

## 文件说明

- `baseline-manifest.json`：每个源码、ANTLR 生成文件和已验证静态库的来源、状态、大小与 SHA-256。
- `tracked-changes.patch`：相对上游提交的已跟踪源码补丁。
- `untracked/`：纳入迁移范围但未被参考仓库跟踪的源码。
- `generated/`：当前已验证构建实际使用的 ANTLR 生成代码。
- `behavior/`：健康状态、能力、典型模型、分析结果及错误输入的规范化 HTTP 输出。
- `validation-summary.json`：本轮 C++、HTTP、浏览器与便携包的验收汇总。
- `ctest.txt`、`http-smoke.json`、`browser-p2-r4.json`、`portable-verification.json`：各层原始验收证据。

## 校验与重建

在 AIDC 根目录运行：

```powershell
python sysml-parser-runtime/tools/verify_reference_baseline.py
```

校验器会确认本地参考工作树、ANTLR 生成文件和 7 个静态库未变化；随后从记录的提交创建临时源码树，应用 `tracked-changes.patch`，合并 `untracked/`，逐文件比较规范化内容哈希。验证过程不会写入参考项目。

冻结脚本默认拒绝覆盖已有基线，只有显式添加 `--force` 才会重新生成，以免误删行为与测试证据。
