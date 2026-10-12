# Modelica Browser Web

Modelica 工程浏览器。模型结构、参数、仿真设置、运行结果和诊断均来自同源 Project Server API；参数与仿真设置写回 Modelica 文件，前端不保存模型私有业务状态。

## 工作台布局

- 左侧模型树：根包、类和组件的统一导航。
- 中间模型视图：组件连接图与 Modelica 文本切换。
- 右侧“参数”页：当前模型或组件的 Modelica 参数。
- 右侧“仿真结果”页：运行选择、变量树和指定时刻的变量值；变量节点打开独立曲线弹窗。

## 命令

在 `AIDC/` 下执行：

```powershell
npm run build:modelica
npm run serve:modelica -- -Project .\samples\modelica-t0-single-joint
npm run verify:modelica:t1-3
npm run verify:modelica:t1-4
```

生产构建由 Project Server 在 `http://127.0.0.1:4185/` 同源提供，开发服务器默认使用 `4186` 并把 `/api` 代理到 `4185`。
