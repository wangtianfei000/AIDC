# AIDCRobotics Profiles

| Profile | 必选能力 | 适用对象 |
|---|---|---|
| `SingleAxis` | 固定基座、单转动关节、扭矩命令、位置/速度状态 | 单关节台架 |
| `ArticulatedRobot` | 固定基座、树形刚体、固定/转动/移动关节、数组命令与状态 | 机械臂、云台 |
| `FloatingLeggedRobot` | 浮动基座、树形刚体、关节驱动、足底接触 | 双足和多足机器人 |
| `NativeMeshContact` | `Contacts` 与 `NativeGeometry` | 需要 STL/OBJ 查询的接触模型 |

Profile 约束能力集合，不包含机器人型号、关节数量或 CAD 文件名。
