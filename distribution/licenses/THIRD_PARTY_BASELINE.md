# 第三方许可基线

P0 只冻结已知来源和发布闸门。此清单不替代最终安装包中的逐文件 SBOM 与许可文本。

| 组件 | 许可 | 状态 | 发行要求 |
|---|---|---|---|
| FreeCAD 1.1.3 | LGPL-2.1-or-later，另含第三方组件 | 复核中 | 包含 FreeCAD 和所有随包组件的许可/通知 |
| OpenModelica 1.27.1 | OSMC-PL，加选定附属模式 | 阻断 | 选择使用模式；提供 OSMC-PL、附属许可、源代码获取方式和使用模式文件 |
| Python 3.12.9 | PSF-2.0，wheels 各自许可 | 阻断 | 选择嵌入包并汇总 wheels 许可证 |
| SysML v2 Standard Library 2026-08 | EPL-2.0 | 候选 | 保留 LICENSE 和来源/版本 |
| AIDC SysML Parser Runtime | AIDC 项目许可证待定 | 阻断公开发行 | 发布前确定仓库许可证 |
| C++ SysML 参考 Parser（提交 `9bfba0b`） | 待核实 | 阻断公开发行 | 记录源码来源、许可证与修改，并随发布包提供通知 |
| ANTLR4 C++ Runtime 4.13.2 | BSD-3-Clause | 已识别 | 保留版权和许可文本 |
| Qt 6.8.1 Core/Network | LGPL-3.0-only/GPL 商业多许可 | 阻断公开发行 | 选择适用许可路径，保留 LGPL 文本、Qt 通知及可替换动态库机制 |
| Three.js | MIT | 已识别 | 保留许可文本 |
| React/ReactDOM | MIT | 已识别 | 从 npm lock 生成最终 SBOM 和通知 |
| maxGraph | Apache-2.0 | 已识别 | 保留许可和 NOTICE（如上游提供） |
| ELK.js | EPL-2.0 | 已识别 | 保留许可文本 |
| Vite/Vitest/TypeScript | MIT/Apache-2.0 | 仅构建期 | 若不进入发行包则只记录构建来源 |

项目所有者尚未选择 AIDC 自研源码和二进制的许可。正式公开发行前必须补充仓库顶层 `LICENSE`，并明确第三方依赖不受 AIDC 自身许可覆盖。

