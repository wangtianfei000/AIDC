# 兼容性矩阵

| AIDCRobotics | Modelica | MSL | OpenModelica | Robot IR | 生成器 | 原生几何 ABI |
|---|---|---|---|---|---|---|
| 0.3.0 | 3.6 | 4.1.0 | 1.27.1 | 0.1 | 0.3.x | 0.1.0 |

Windows x86-64 发布包包含 `modelica-native-geometry` 动态库。其他平台可使用解析接触组件，或按 `native/modelica-native-geometry` 的 CMake 工程构建同一稳定 C ABI。

兼容性规则：补丁版本不移除公共类或连接器；次版本可增加组件和参数，但保持已有模型可加载；Robot IR 的未知 schema 必须先迁移，禁止静默降级。
