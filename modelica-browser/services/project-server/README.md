# Project Server

Project Server 只读取一个最小 Modelica 工程：`model-project.json` 定位默认模型，`modelica/` 保存模型和仿真 annotation，`results/` 保存运行结果。服务不维护参数集、场景、接口或证据副本。

```powershell
.\AIDC\modelica-browser\scripts\start-project-server.ps1 -Project .\AIDC\samples\modelica-t0-single-joint -Port 4185
```

主要 API：

- `GET /api/v1/project`
- `GET /api/v1/model/tree`
- `GET /api/v1/model/configuration`
- `POST /api/v1/model/simulation-settings`
- `GET /api/v1/source?path=&line=`
- `GET/POST /api/v1/runs`
- `GET /api/v1/runs/{id}`、`series`、`values`
- `GET /api/v1/tasks/{id}` 与 SSE `events`

仿真设置写入默认模型的 `experiment` 和 `__OpenModelica_simulationFlags` annotation。运行器直接编译 `modelica.defaultExperiment`，结果写入 `results/<run-id>/`。
