# AIDC Modelica Browser

## AIDC Modelica 产品入口

```powershell
.\scripts\aidc-modelica.ps1 validate packages/robot-ir/examples/reference-arm5
.\scripts\aidc-modelica.ps1 generate packages/robot-ir/examples/reference-arm5 generated/reference-arm5
# 产品入口：选择包含 assembly.manifest.json 的完整装配发布包
.\scripts\aidc-modelica.ps1 generate <robot.aidc-assembly> generated/<robot>
# 单零件网格测试入口
.\scripts\aidc-modelica.ps1 generate test-artifacts\stl\cube-10mm.stl generated/stl-cube --stl-unit mm --density 1000
.\scripts\aidc-modelica.ps1 compare <run-a> <run-b>
.\scripts\aidc-modelica.ps1 package generated/reference-arm5 dist/reference-arm5.aidcmpkg
```

`generate` 的产品输入是机器人装配发布包目录。目录只保留 `assembly.manifest.json`、根目录三份 Robot IR，以及 `physics.json` 实际引用的 `geometry/` 网格；刚体树、关节和姿态统一保存在 `physics.json`，不重复交付整机 GLB、运动学、报告等中间文件。底层工具仍接受 Robot IR 目录，原生 STL 仅用于按指定单位和密度验证单刚体。规范见 [AIDC 机器人装配发布包规范 1.0](docs/AIDC机器人装配发布包规范.md)，完整设计和 P0–P5 实施记录见 [三维模型到Modelica生成工具设计](docs/三维模型到Modelica生成工具设计.md)。

该目录承载 Modelica 模型工程浏览器及其公共契约。T1 已完成：工程使用 `model-project.json + modelica/ + results/` 最小结构；模型参数与仿真设置由 Modelica 文件保存；Project Server 负责 OMC 索引、编译、仿真、结果与诊断 API；Web 浏览器仅保留面板宽度等界面偏好。

T2 新增 `samples/modelica-t2-single-joint`：飞特 HD-1910-C001 单关节降阶模型、五类验证场景、厂家规格点校准、OpenModelica/Sysplorer 交叉对比及 FMI 2.0 ME+CS 接口基线。

统一验收入口：

```powershell
.\scripts\verify-t1-1.ps1
.\scripts\verify-t1-2.ps1
.\scripts\verify-t1-3.ps1
.\scripts\verify-t1-4.ps1

# 从 AIDC/ 执行完整 T1 验收
npm run verify:modelica:t1
npm run verify:modelica:t2
```

单工程校验与运行：

```powershell
.\scripts\run-project.ps1 -Project ..\samples\modelica-t0-single-joint -ValidateOnly
.\scripts\run-project.ps1 -Project ..\samples\modelica-t0-single-joint

# 启动 Project Server
.\scripts\start-project-server.ps1 -Project ..\samples\modelica-t0-single-joint -Port 4185
```
