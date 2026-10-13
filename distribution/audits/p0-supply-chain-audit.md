# P0 供应链基线审计

版本：0.1.0  
日期：2026-10-12  
发布结论：**阻断公开二进制发行；允许继续内部开发和契约实现。**

本文件是工程供应链审计，不构成法律意见。任何对外二进制发行都必须由项目所有者确认 AIDC 自身许可，并完成第三方许可复核。

## 审计范围

- AIDC 自研浏览器、服务、Modelica 库和 Skills；
- FreeCAD 1.1.3 Windows x64；
- OpenModelica 1.27.1 Windows x64；
- Python 3.12.9 运行时方案；
- AIDC Standalone SysML Parser Runtime 0.9.0；
- SysML v2 Standard Library 2026-08；
- 前端 npm 生产依赖和本机原生运行库。

## 已确认事实

| 组件 | 来源与版本 | 哈希证据 | 许可结论 | 发布状态 |
|---|---|---|---|---|
| FreeCAD | 官方 1.1.3 Windows x64 | 官方归档 SHA-256 已记录；本机 `FreeCAD.exe` 已另行抽样哈希 | 官方说明为 LGPL-2.1-or-later；随包第三方许可仍需汇总 | 候选，未批准 |
| OpenModelica | 官方 1.27.1 Windows x64 | 本机曾以 MD5/数字签名验证；当前缺少安装归档 SHA-256 | 官方 OSMC-PL 要求选择使用模式并随发行提供相应文件 | 阻断 |
| Python | 当前开发环境 3.12.9 | 尚未选择官方嵌入包，缺 SHA-256 | PSF-2.0 及 wheels 的各自许可证待汇总 | 阻断 |
| SysML Parser Runtime | 仓库内独立构建 0.9.0 | 发布构建时生成 EXE SHA-256 | 已移除外部参考 Parser 代码和静态库；AIDC 项目许可证仍待确定 | 阻断公开发行 |
| SysML 标准库 | Systems-Modeling 2026-08 | 本地 LICENSE 哈希已记录；发行归档哈希待补 | 上游仓库声明 EPL-2.0 | 候选，未批准 |
| AIDC 自研代码 | 本工作区 | 由 Git 提交固定 | 顶层 LICENSE 尚未选择 | 阻断 |

## 许可证来源

- FreeCAD 官方许可说明：<https://www.freecad.org/contributing.php>
- OpenModelica OSMC Public License：<https://openmodelica.org/free-and-open-source-software/full-license/>
- OpenModelica 许可资源：<https://openmodelica.org/useresresources/license/>
- SysML v2 Release：<https://github.com/Systems-Modeling/SysML-v2-Release>

## 当前哈希

| 文件 | SHA-256 |
|---|---|
| FreeCAD 1.1.3 官方归档（已在项目 README 记录） | `9c6959dc9c4dba64dd818a62447e3dfedb4221d776fb044b239d462f150bcec4` |
| 本机 `FreeCAD.exe` 抽样 | `a89514e9d174ffac59c8795e8f7b8312497f556586b7e207e7b60eec72b7f860` |
| 本机 `omc.exe` 抽样 | `461af7f3a8ea20252fa1659902206598739c49f5f5114617a6d8751173bf1da5` |
| SysML Parser Runtime 0.9.0 EXE | 每次正式构建后写入 SBOM 和发布清单 |
| SysML 2026-08 LICENSE | `22b203a3f2bfa35ff272ed57702d154aed8966ea0978c44ceca6658f2c60bc51` |
| npm lock file | `e98b25b94307aa8b427a59ac198b11239a1eee160c9e0fa270047617c888c2a1` |

抽样可执行文件哈希只证明当前本机文件身份，不能替代发行归档哈希。

## 阻断项

机器可读状态位于 `supply-chain-gates.json`。公开发行前必须关闭：

1. AIDC 顶层许可选择；
2. OpenModelica 的归档 SHA-256、OSMC-PL 使用模式、最小包和第三方通知；
3. Python 嵌入运行时与 wheels 锁定；
4. FreeCAD CAD Pack 的完整 SBOM/通知；
5. AIDC 构建产物签名。
6. AIDC 自研 EXE 和安装器签名策略。

## P0 决策

- Git 仓库只提交自研源码、契约、文档和必要文本样例；不提交第三方源码镜像、解压运行时、下载缓存和开发依赖。
- 依赖锁文件允许记录 `blocked`/`candidate`，但只有 `approved` 组件能进入正式发行构建。
- 任何 `null` SHA-256 都必须对应发布阻断项。
- P1 可以实现 CLI 与运行管理器；安装器和公开二进制发布必须等待关键供应链阻断关闭。

