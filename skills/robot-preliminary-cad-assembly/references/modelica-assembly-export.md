# Modelica 最小装配包导出规范

三维模型浏览器向 Modelica 交付一个可校验、可生成、可仿真的目录。三维显示网格负责外形，Robot IR 负责物理语义；只有两者身份与拓扑一致时才能导出。

## 交付结构

```text
<robot>.aidc-assembly/
|-- assembly.manifest.json
|-- physics.json
|-- devices.json
|-- interfaces.json
`-- geometry/                 # 仅保留 physics.json 实际引用的网格
    `-- ...
```

- `assembly.manifest.json` 是唯一入口，版本为 `aidc.robot.assembly/1.0`，记录模型身份、来源和每个负载文件的 SHA-256 与字节数。
- `physics.json` 定义刚体树、质量、质心、惯量、关节、接触和几何引用。
- `devices.json` 定义执行器、电源和传感器。
- `interfaces.json` 定义命令、状态、诊断通道及顺序。
- `geometry/` 只包含三份 Robot IR 中由 `physics.json.geometryAssets[].path` 实际引用的文件。对于带视觉资产的三维工程，每个刚体必须至少导出一个刚体局部坐标系下的运行时网格，并通过 `visualAssetId` 绑定；不得生成没有三维外形的“空壳”Modelica 模型。

不要把整机 `visual.glb`、`kinematics.json`、姿态、截图、报告、Schema、缓存或转换中间文件原样复制到 Modelica 装配包。导出器应按刚体归集 GLB 组件，转换为 `geometry/<bodyId>.stl` 等受 Modelica 支持的刚体局部网格。它们之外的文件属于三维浏览发布物或验证记录，不是 Modelica 生成输入。

## 导出前置条件

1. 三维工程已发布 `generated/model3d.manifest.json`，并通过自身 Schema 和节点映射校验。
2. 工程有同一型号和修订的已审核 Robot IR 目录，至少包含 `physics.json`、`devices.json`、`interfaces.json`。
3. 三维清单与 Robot IR 的 `rootBodyId`、刚体 ID 集合，以及每个关节的 ID、类型、父刚体、子刚体完全一致。
4. Robot IR 内单位统一为 SI，四元数顺序为 `xyzw`，质量、惯量、限位、设备和通道通过跨文档校验。

任一条件不满足都必须阻断导出并报告差异。导出器不得从 STL/GLB 的形状猜测质量、惯量、关节、接触、执行器或控制接口。

## 浏览器接口

三维模型浏览器工具栏提供“导出 Modelica”。工程服务调用：

```text
POST /api/model-projects/<modelId>/modelica-assembly
```

输入来自当前工程的 `generated/model3d.manifest.json` 和配置的 Robot IR 目录；输出默认写到：

```text
release/<modelId>-<revision>.aidc-assembly/
```

已有同模型、同修订且校验通过的输出作为缓存命中返回。已有损坏或身份不同的目录不覆盖，用户必须先处理旧产物。

## 闭环验收

每个发布器版本至少用一个真实的树形装配执行：

1. 从三维模型浏览器接口导出装配包；
2. 使用装配包校验器核对清单、哈希、Robot IR 和几何闭包；
3. 使用 Modelica 浏览器转换接口导入该目录并生成工程；
4. 校验生成工程，使用 OMC 编译并运行默认实验；
5. 确认生成结构中的根刚体、刚体和关节与输入一致，并保存可复现命令。

只有五步全部通过，才能声称“三维模型浏览器导出、Modelica 模型浏览器导入”闭环成立。
