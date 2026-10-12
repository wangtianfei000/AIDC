# DiagramModel 0.1 图形契约

## 1. 目的

`DiagramModel` 是 SysML 语义模型和 Web 图形组件之间的稳定边界。Parser Runtime 负责给出语义事实，`DiagramProjection` 负责按查看上下文投影节点、端口和连接，布局器只负责计算坐标和路径。任何一层都不能从源码文本再次猜测模型含义。

首版只定义 `interconnection`（互连图）。需求图、状态图和行为图以后通过新增 `kind` 和专用投影器扩展，不修改现有互连图语义。

## 2. 数据流

```mermaid
flowchart LR
    A[ModelSnapshot] --> B[DiagramProjection]
    V[查看上下文/可选 View] --> B
    B --> C[DiagramModel\n无坐标也合法]
    M[@GraphicMetadata] --> D[布局输入合并]
    C --> D
    D --> E[ELK Layered]
    E --> F[有坐标的 DiagramModel]
    F --> G[React 图形视图]
```

`src/diagram/model.ts` 是机器可检查的 TypeScript 契约，本文解释业务含义。

## 3. 元素规则

### 3.1 节点

- 每张图必须有且只有一个 `context` 节点，其 `semanticElementId` 等于 `contextElementId`。
- 被展示的部件使用 `part` 节点，并通过 `parentId` 形成嵌套层级。
- `semanticElementId` 指向 ModelSnapshot 中的实例元素；`typeElementId` 可指向它的部件定义。
- 自动布局前 `bounds` 可以为空；读取到合法 `@GraphicMetadata` 后可以带初始位置和尺寸。
- 标识必须稳定：`<viewId>:node:<semanticElementId>`。

### 3.2 端口

端口属于图中的部件实例，而不是悬空图元。`nodeId` 指向承载端口的节点，`semanticOwnerId` 指向该实例。

端口有两种来源：

| `projection` | 含义 | 示例 |
|---|---|---|
| `declared` | 端口直接声明在实例/用法上 | `leftLeg.powerIn` |
| `typed` | 端口声明在类型定义上，投影到每一个带类型的实例 | `leftHipRoll.powerIn` 来自 `ServoActuator::powerIn` |

类型端口投影必须遵守以下规则：

1. 从 part usage 的 `typeElementId` 找到 part definition；
2. 遍历定义拥有的端口，为当前实例建立图形端口；
3. `semanticElementId` 保留定义端口 ID，`semanticOwnerId` 使用实例 ID；
4. 因此同一定义端口投影到不同实例时仍有不同 ID：`<viewId>:port:<instanceId>:<definitionPortId>`；
5. 连接端路径 `leftHipRoll.powerIn` 先解析实例，再解析其直接端口或类型端口；
6. 用法上同名的直接端口覆盖类型端口，不能同时生成两个端口。

`side` 和 `order` 是布局提示。没有元数据时 `side` 可以是 `UNDEFINED`，由 ELK 决定；重复生成必须保持同样顺序。

### 3.3 连接

- 每条边指向两个已经投影的端口，不能直接连接节点中心。
- `semanticRelationshipId` 指向 ModelSnapshot 中的 Connection。
- 稳定 ID 为 `<viewId>:edge:<semanticRelationshipId>`。
- 投影阶段只确定语义端点；布局阶段生成 `sections` 和折点。
- 首版保留 SysML 连接的源、目标书写顺序，不据此推断电气方向。

## 4. 布局规则

- 默认算法固定为 `elk.layered`，层级处理为 `INCLUDE_CHILDREN`。
- 整机和供电网络默认从左到右，腿部层级默认从上到下。
- 连接默认正交路由；固定 `randomSeed`，保证同一模型重复打开布局稳定。
- `layoutSource=auto` 表示全部自动布局；`graphic-metadata` 表示全部采用有效元数据；`mixed` 表示固定有元数据的图元，其余图元自动补齐。
- 非法、越界或互相重叠的元数据不能破坏图形生成，应输出诊断并回退自动布局。

## 5. `@GraphicMetadata` 兼容范围

G0 冻结以下最小读取范围：

| 对象 | 字段 | 映射 |
|---|---|---|
| 节点 | `graphicX`, `graphicY`, `graphicWidth`, `graphicHeight` | `bounds` |
| 端口 | `graphicX`, `graphicY`, `portPosition`, `portOffset` | `position`, `side`, 边界偏移 |
| 连接 | `labelText`, `pathType`, `routingStyle` | `label`, `routing` |

元数据只是可选表示信息，不能改变部件、端口和连接的 SysML 语义。兼容样例见 `samples/jingang-10axis-system-design/views/graphic-metadata-compatibility.sysml`。

## 6. G0 验收图

`diagram-expectations.json` 冻结三张图的业务范围：

| 图 | 上下文 | 节点 | 端口 | 连接 | 主要验证点 |
|---|---|---:|---:|---:|---|
| 金刚整机互连图 | `JingangNative10` | 8 | 16 | 11 | 一级组成和整机接口 |
| 左腿内部互连图 | `leftLeg` | 4 | 10 | 8 | `ServoActuator` 类型端口投影 |
| 整机供电网络图 | `JingangNative10` | 7 | 8 | 6 | 按连接关系筛选子图 |

上下文节点属于 DiagramModel，但作为画布/容器处理，不计入上表的业务节点数。

## 7. G1 输入条件

进入投影实现前，Runtime 的 ModelSnapshot 必须提供：

- part usage 的拥有关系和类型定义引用；
- part definition 拥有的端口；
- Connection 原始端路径，或已经解析的端点 ID；
- `@GraphicMetadata` Annotation 的赋值；
- 所有语义元素稳定 ID。

若某个字段缺失，应修正 Runtime 契约或输出诊断，不能在前端读取 SysML 源码正则补值。
