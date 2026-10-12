# AIDCRobotics 0.3.0 API

`AIDCRobotics` 是机器人模型的通用 Modelica 标准库。公共坐标约定为 `+X` 前、`+Y` 左、`+Z` 上，SI 单位，FMI 四元数顺序为 `wxyz`。

| 包 | 稳定接口 | 用途 |
|---|---|---|
| `Core` | 版本和标准约定 | 模型与工具链握手 |
| `Interfaces` | 命令、状态、诊断连接器 | 控制器与机器人解耦 |
| `Mechanics` | `RigidBody3D`、固定/浮动基座、固定/转动/移动关节、关节损耗 | 树形多刚体系统 |
| `Actuators` | `IdealTorque` | 可替换关节执行器基线 |
| `Electrical` | 电机和电池母线 | 电气能量链路 |
| `Sensors` | 关节、IMU 和力矩传感器 | 状态与观测 |
| `Contacts` | 点、球、盒、胶囊、足底代理与平面接触 | 连续接触动力学 |
| `NativeGeometry` | `MeshHandle` 和确定性几何查询 | 缓存 STL/OBJ 网格和 BVH |
| `DeviceData` | 参数记录 | 设备数据与方程分离 |
| `Profiles` | 机器人能力组合 | 兼容性声明 |
| `Conformance` | 可执行契约模型 | 发布验收 |
| `Examples` | 标准组件示例 | 独立建模参考 |

生成器只实例化这些稳定组件并写入参数和连接，不复制通用方程。新设备优先新增 `DeviceData` 记录；行为不同的设备实现既有标准连接器。
