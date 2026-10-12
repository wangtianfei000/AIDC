# 模型工程与发布包规范

本规范区分用于建模的 **上游工程**、供浏览器运行的 **浏览发布物**、供归档或交付的 **模型包**。浏览器以发布物为产品输入，不要求打开 FCStd/STEP，也不得从 SysML 或文件名猜测物理结构。

供 Modelica 生成与仿真的输出不使用本页的浏览归档结构，而使用[Modelica 最小装配包导出规范](modelica-assembly-export.md)。浏览包服务交互显示，Modelica 装配包服务确定性生成与仿真，两者由稳定的模型、刚体和关节 ID 关联。

## 浏览发布物原则

浏览器主模型统一使用 GLB。GLB 保存可直接渲染的网格、材质、局部坐标和节点层级；运动、仿真和碰撞语义由独立数据契约提供。STL 只作为没有层级语义的网格交换格式或外部仿真兼容资产，不得作为浏览器主模型。

- `visual.glb` 是主显示资产，必须保留物理装配节点、稳定节点名、局部坐标、颜色和材质。
- `model3d.manifest.json` 是模型包入口，声明资产、产品结构、版本和哈希。
- `kinematics.json` 定义刚体、关节、轴线、零位、限位和驱动通道，并通过 GLB 节点名绑定显示模型。
- `collision.glb` 只包含低复杂度碰撞代理，用于实时预警；不得用显示模型的高面数网格冒充实时碰撞模型。
- `simulation-frame` 只传递时间、机体位姿和关节值。仿真引擎计算状态，浏览器据此更新 GLB 节点，不依赖预制动画。
- 上游可以使用 FCStd、STEP、Onshape 或其他 CAD，但这些文件只负责生成发布物，不属于浏览器运行时必需文件。

同一套发布物可以由设计工具、仿真器或离线发布流水线生成。浏览器只校验发布物内部的 ID、节点、单位、版本和哈希，不承担 CAD 编辑职责。

## 浏览器“打开模型”当前支持范围

| 打开方式 | 需要选择的文件 | 可用能力 | 限制 |
| --- | --- | --- | --- |
| 通用 GLB | 一个 `.glb` | 显示、相机、显隐、选择、由 GLB 场景层级推导的模型树、通用爆炸图 | 没有关节、命名姿态、模型契约和精确校核配置 |
| 通用 glTF | 一个资源内嵌的 `.gltf` | 与通用 GLB 相同 | 当前文件选择器不接收外置 `.bin` 和纹理文件；工程交换优先使用 GLB |
| AIDC 结构化模型 | `model3d.manifest.json`、其中 `visual` 指向的 `.glb` 和声明的 `kinematics.json`；需要工程姿态时一并选择 `pose-presets.json` | 明确的物理树、部件映射、关节运动、命名姿态和仿真数据驱动 | 多个文件必须在同一次选择中打开；本地文件模式只读，不把姿态写回磁盘 |

清单文件名必须包含 `manifest` 且扩展名为 `.json`。普通 JSON 不能独立作为模型打开。选择清单后，浏览器按 `assets[].role == "visual"` 查找视觉资产，并按文件名匹配同次选择的文件。

当前“打开模型”不直接打开 `.FCStd`、STEP、STL 或 `.robot3d.zip`。这些文件若作为上游输入，应由发布工具转换为 GLB 或 AIDC 结构化模型。当前手工打开流程不加载独立 `collision.glb`、验证报告或 Schema；它们属于完整发布归档。

## 推荐工程目录

```text
<design-project>/
|-- model3d/
|   |-- publish.config.json       # 发布输入与映射；人工维护
|   |-- pose-presets.json         # 属于该模型版本的命名姿态；人工/浏览器维护
|   |-- assembly.sequence.json    # 可选；工艺或历史数据，当前浏览器不播放
|   |-- expected/                 # 可选；中性姿态和运动学回归基线
|   |-- simulation/               # 可选；仿真数据样例或回放输入
|   |-- generated/                # 发布器生成，禁止手工维护
|   |   |-- model3d.manifest.json
|   |   |-- visual.glb
|   |   |-- kinematics.json       # 刚体、关节、限位和驱动通道
|   |   |-- collision.glb         # 可选低复杂度实时碰撞资产
|   |   |-- publish-report.json
|   |   `-- validation-report.json
|   `-- release/                  # 自包含归档及其哈希
|       |-- <model>-<revision>.robot3d.zip
|       |-- <model>-<revision>.robot3d.zip.sha256
|       `-- <modelId>-<revision>.aidc-assembly/ # Modelica 最小装配包
|-- layout/                       # 推荐放置受控 FCStd/STEP 等 CAD 源文件
`-- architecture.sysml            # 当前发布器的追溯输入；浏览器运行不读取
```

CAD 源文件可以位于工程内其他目录或受控 CAD 仓库，但 `publish.config.json` 中必须使用可复现的相对路径。不得引用临时目录、个人下载目录或仅在一台电脑存在的绝对路径。

## 文件职责与必需性

### 1. 源工程

| 文件 | 当前是否必需 | 职责 |
| --- | --- | --- |
| `publish.config.json` | 发布必需 | 模型 ID、修订、CAD 路径、根装配、部件映射、关节、发布目录和碰撞设置 |
| `sourceCad` 指向的 `.FCStd` | 发布必需 | 几何、`App::Part` 物理层级、刚体名称、对象属性和颜色的事实源 |
| `sourceSysml` 指向的文件 | 当前发布器必需 | 只用于稳定 ID 的追溯检查；三维浏览器不读取它，也不得用它生成或覆盖 CAD 物理树 |
| `pose-presets.json` | 当前开发服务必需 | 与 `modelId`、`revision` 绑定的命名关节姿态；不定义关节本身 |
| `assembly.sequence.json` | 可选 | 装配工艺或历史序列；当前浏览器已移除装配模拟 |
| `expected/`、`simulation/` | 可选但建议 | 自动验收的姿态、变换和仿真数据基线 |

`publish.config.json` 中的 `joints` 定义可动关节、父子刚体、局部轴、零位、上下限和速度；`componentMap` 只负责稳定部件 ID 与 CAD/GLB 节点的映射。物理包含层级必须来自 FCStd 内的嵌套 `App::Part`。

### 2. `generated/` 发布物

浏览器开发服务启动前至少必须存在：

- `model3d.manifest.json`：模型包主清单和运行时契约；
- `visual.glb`：带稳定节点名称、层级和材质的视觉模型。
- `kinematics.json`：当清单声明该资产时必须存在；它是运动学运行时定义，优先于旧清单中的兼容副本。

完整发布流程还应生成发布报告和验证报告。`collision.glb` 可以作为发布契约中的碰撞资产保留，但不得声称当前浏览器已使用它，除非运行时代码已明确加载该资产。精确干涉校核仍需要清单所引用的受控 CAD 源文件和本地 FreeCAD/OpenCascade 服务。

`generated/` 中的文件必须由同一版本的上游模型和配置生成。不得单独替换 GLB 或运动学文件而不更新模型修订与资产哈希。

### 3. `release/` 发布归档

发布归档用于交付、校验哈希和离线留档，通常包含：

```text
model3d.manifest.json
visual.glb
kinematics.json
collision.glb                 # 若该模型发布了碰撞资产
pose-presets.json
assembly.sequence.json        # 若项目仍保留该数据
simulation/*                  # 若交付仿真样例
reports/publish-report.json
reports/validation-report.json
schemas/*
```

归档内容必须与清单中的模型 ID、修订、资产哈希和字节数一致。当前浏览器的“打开模型”不会解压 `.robot3d.zip`；需要手工查看时，先解压并同时选择清单、视觉 GLB 和所需姿态文件。

## 新建或切换模型工程

1. 在独立 `model3d/` 目录中创建 `publish.config.json` 和与模型版本匹配的 `pose-presets.json`。
2. 让配置引用受控 CAD 源文件；确认 FCStd 只有一个物理根装配，所有非根节点只有一个父 `App::Part`。
3. 配置根刚体、稳定部件映射和关节契约。不要在配置中另造 CAD 父子树。
4. 运行发布器，将结果只写入 `generated/`，再运行 Schema、节点映射、关节和重新打开检查。
5. 开发服务通过 `AIDC_MODEL3D_PROJECT_DIR=<model3d目录>` 切换工程；该目录必须已有 `generated/model3d.manifest.json`、清单引用的视觉资产和 `pose-presets.json`。
6. 需要对外交付时再生成 `release/` 归档和 SHA-256；不要用归档替代源工程。

正常产品入口应把上述第 4、5 步封装为一次“打开模型工程”：本地服务读取工程配置、校验缓存新鲜度、按需发布并启动浏览。命令行发布和环境变量切换保留给自动化、持续集成和故障排查。

## 验收要点

- 删除或移动任一必需文件时，发布或启动应明确失败，不能静默回退到另一个模型。
- 清单中的视觉资产 URI、哈希、字节数与实际文件一致。
- `modelId`、`revision` 在清单和姿态文件中一致。
- 运动学文件中的刚体和关节引用、清单中的产品树都能解析到 GLB 的稳定节点名称。
- 单独打开 GLB 时应明确属于“通用浏览模式”，不得显示不存在的运动、精确校核或工程姿态能力。
- 换模型只切换模型工程目录或打开另一组结构化文件，不修改浏览器源代码，也不复用上一模型的姿态数据。
