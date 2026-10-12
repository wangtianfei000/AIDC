# AIDC Model3D Viewer

独立的机器人三维模型浏览器软件。它以 GLB 为主显示模型，使用独立 JSON 契约描述物理结构、运动学、姿态与仿真帧，提供便捷浏览、运动驱动、干涉预警与爆炸图，不依赖 SysML 浏览器运行。

## 命令

在 `AIDC` 目录执行：

```powershell
npm install
npm run publish:model3d:jingang
npm run dev:model3d
npm run verify:model3d
npm run verify:model3d:e5
```

`publish:model3d:jingang` 生成 `visual.glb`、`collision.glb`、`kinematics.json` 和 `model3d.manifest.json`。`npm run dev:model3d` 在 `http://127.0.0.1:4174/` 启动独立软件。`verify:model3d` 会重新发布、验证显示节点与运动学映射、运行 TypeScript 测试并构建应用。

`verify:model3d:e5` 对机器人、同轴轴系、箱体和无 AIDC 扩展配置的第三方 STEP 执行通用爆炸图验收。任意 STEP 可用 `model3d-viewer/tools/freecad-publisher/convert-step.ps1 -InputPath <assembly.step> -OutputPath <assembly.glb>` 转为浏览发布物；浏览器会从 GLB 场景层级自动建立集合/部件产品树。

独立应用不从 SysML 浏览器的 `public` 目录读取任何文件。

命名姿态保存在当前模型工程的 `pose-presets.json`，不会写入浏览器本地存储。开发服务默认打开金刚样例；切换模型工程时设置 `AIDC_MODEL3D_PROJECT_DIR` 后再启动：

```powershell
$env:AIDC_MODEL3D_PROJECT_DIR = 'D:\path\to\model3d'
npm run dev:model3d
```

## 边界

- `packages/contracts` 不依赖渲染框架；
- `packages/three-runtime` 不读取 SysML 或调用应用导航；
- `apps/browser` 是独立三维模型浏览器应用；
- SysML 浏览器与三维模型浏览器分别构建、分别启动；两者仅通过稳定工程 ID 和模型包契约关联；
- `visual.glb` 是浏览器的主显示资产；模型包不要求用户打开或管理 FCStd/STEP；
- `kinematics.json` 通过稳定节点名把关节、刚体和舵机通道映射到 GLB；仿真帧只传递关节值和机体位姿；
- `collision.glb` 是低复杂度实时碰撞资产；STL 仅用于外部工具兼容，不作为浏览器主模型；
- V3 的 `collision.glb` 是实时预警代理；运行 `npm run serve:model3d:exact` 可启动本地 FreeCAD/OpenCascade 精确校核服务。
- V4 支持可逆爆炸图、BOM 标注和引出线；装配步骤播放功能已从浏览器移除。
- 姿态预设由模型工程的 `pose-presets.json` 管理，浏览器只通过模型工程接口读取和保存。
