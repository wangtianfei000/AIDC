# 初步 CAD 装配设计技能实施报告

## 1. 目标

把 P4 样例中的工程经验整理成可复用技能，指导工程师或 Agent 从 SysML 架构、硬件资料和已有 CAD 建立初步装配。技能的完成标准是能够支持空间、安装、运动、干涉、维护和质量判断，同时把未知接口清楚交给机械详细设计；它不承担制造放行。

## 2. 技能结构

技能位于 `AIDC/skills/robot-preliminary-cad-assembly/`：

- `SKILL.md`：任务入口、工程事实边界、建模步骤、验证方法和交付要求；
- `references/workflow.md`：配置冻结、来源盘点、接口优先和模块集成流程；
- `references/modeling-contract.md`：采购件、自制件、紧固件、关节和 CAD 属性契约；
- `references/verification-checklist.md`：配置、几何、运动、维护、质量和文件完整性检查；
- `references/acceptance-scenarios.md`：截图、版本冲突、STL、占位体和配置冲突的处理方式；
- `assets/assembly-manifest.example.json`：机器可读装配清单示例；
- `agents/openai.yaml`：技能显示名称、简介和默认调用提示。

## 3. 关键决策

1. 先冻结硬件配置，再建立装配；同一用途的多个候选型号分成不同配置。
2. 优先复用厂家 STEP、受控原生 CAD、正式图纸或实测重建，禁止用无来源方盒覆盖已有详细几何。
3. 采购件保留影响安装、干涉和维护的真实形状；自制件保留承力截面、孔、定位、支承和工具空间。
4. 包络只用于规划，并以 `PLANNING_ENVELOPE` 明示；承担真实接口判断时必须阻断对应检查。
5. SysML、CAD、BOM 和验证记录共享稳定 `componentId`，几何来源记录版本和 SHA-256。
6. `geometryChecksPassed`、`assemblyChecksPassed`、`p4ExitReady` 和 `manufacturingReleased` 分别管理。P4 可以退出，但制造状态继续为 `NOT_RELEASED`。

## 4. P4 边界同步

当前十轴装配已满足初步装配阶段用途，P4 状态改为完成。电子件型号冲突、IMU 包络和 HD-1910 实物接口细节继续保留为机械详细设计未关闭项，不再反向扩大初步装配阶段的完成条件。

## 5. 验证

使用 skill-creator 的 `quick_validate.py` 检查技能目录、YAML frontmatter、命名和 Agent 元数据；使用 JSON 解析检查装配清单示例；检查 `SKILL.md` 引用的全部本地文件存在。验证命令和结果随本次实现记录执行。
