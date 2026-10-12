# 金刚十轴 Model3D 样例

此目录是三维模型浏览器 V0 的可重复发布输入与输出。

- `publish.config.json`：FreeCAD 对象、19 个 SysML 部件 ID 与 10 个关节的发布映射。
- `expected/neutral-body-poses.json`：从 P4 FreeCAD 装配冻结的中性姿态基线。
- `expected/v2-asymmetric-motion-pose.json`：V2 非对称测试姿态的关节值与九个关键刚体世界变换基线。
- `simulation/neutral-frame.json`：单位为弧度的最小仿真帧。
- `assembly.sequence.json`：装配序列契约占位，V0 不编造工艺步骤。
- `pose-presets.json`：属于模型工程的命名姿态预设，保存关节值并随模型包发布。
- `generated/`：由发布器产生的 GLB、清单和验证报告。
- `release/`：通过全部 V0 验收后生成的自包含模型包。

在 `AIDC` 目录执行 `npm run verify:model3d` 可重新发布、验证、测试、构建并打包。
