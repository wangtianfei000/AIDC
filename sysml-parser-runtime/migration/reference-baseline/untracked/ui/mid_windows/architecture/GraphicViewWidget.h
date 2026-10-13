#ifndef GRAPHICVIEWWIDGET_H
#define GRAPHICVIEWWIDGET_H

#include <QGraphicsView>
#include "../../../core/model/Element.h"
#include "../../../core/common/Global.h"
#include "ArchitectureScene.h"

SYSML_NAMESPACE_BEGIN

// 前向声明
class PartGraphicsItem;
class PortGraphicsItem;
class ConnectionGraphicsItem;

/**
 * @brief GraphicViewWidget 图形视图Widget
 * 
 * 继承自QGraphicsView，显示ArchitectureScene
 * 用于显示part的图形视图（端口、子part、连接）
 */
class GraphicViewWidget : public QGraphicsView {
    Q_OBJECT

public:
    explicit GraphicViewWidget(QWidget* parent = nullptr);
    ~GraphicViewWidget() override;
    
    void setPart(Element* part);
    Element* part() const { return m_part; }
    
    void refresh();
    
    // 访问场景
    ArchitectureScene* scene() const { return m_scene; }
    
    // 编辑接口
    bool deletePart(Element* partUsage);
    bool deletePort(Element* portElement);
    bool deleteConnection(Element* connection);
    Element* createPartInstance(Element* partDef, const QPointF& scenePos, Element* parentPartUsage = nullptr);  // 创建 PartUsage 实例
    Element* createConnection(Element* sourcePort, Element* targetPort, QGraphicsItem* sourcePortItem = nullptr, QGraphicsItem* targetPortItem = nullptr);  // 创建连接
    
    // 连接预览相关
    void startConnectionPreview(QGraphicsItem* sourcePortItem, const QPointF& startPos);  // 开始连接预览
    void updateConnectionPreview(const QPointF& currentPos);  // 更新连接预览
    void endConnectionPreview();  // 结束连接预览

protected:
    void mousePressEvent(QMouseEvent* event) override;  // 鼠标按下事件（检查是否点击了端口项）
    void wheelEvent(QWheelEvent* event) override;  // 缩放
    void resizeEvent(QResizeEvent* event) override;  // 调整大小时居中显示
    void keyPressEvent(QKeyEvent* event) override;  // 键盘事件（Delete键删除）
    void dragEnterEvent(QDragEnterEvent* event) override;  // 拖放进入事件
    void dragMoveEvent(QDragMoveEvent* event) override;  // 拖放移动事件
    void dropEvent(QDropEvent* event) override;  // 拖放事件

private:
    void setupScene();
    PartGraphicsItem* renderPart(Element* part, PartGraphicsItem* parentItem = nullptr, double scaleX = 1.0, double scaleY = 1.0);
    void renderPorts(Element* part, PartGraphicsItem* partItem);
    void renderBasicPartPorts(Element* part, PartGraphicsItem* partItem);
    // connectionOwner: 拥有 ConnectStatement 的元素（例如 PartDefinition）
    // visualRoot: 用于查找端口图元的根 part（例如对应的 PartUsage 或根 PartDefinition）
    // parentItem: 连接线应该作为哪个图元的子项（如果为 nullptr，则添加到 scene）
    void renderConnections(Element* connectionOwner, Element* visualRoot, QGraphicsItem* parentItem = nullptr);
    void centerOnPart();
    
    QList<Element*> getChildParts(Element* part) const;
    QList<Element*> getPorts(Element* part) const;
    QList<Element*> getPortsForPart(Element* part) const;  // 获取part的端口（处理PartUsage的情况）
    Element* getPartDefinition(Element* partUsage) const;  // 从PartUsage获取PartDefinition
    QList<Element*> getConnections(Element* part) const;
    
    // 连接解析和端口查找
    QString getConnectionSourceEnd(Element* connection) const;  // 获取连接的源端点字符串
    QString getConnectionTargetEnd(Element* connection) const;  // 获取连接的目标端点字符串
    // endString: 端点字符串（可能是 \"partName.portName\" 或 \"portName\"）
    // connectionOwner: 拥有该端点语义结构的元素（通常是 PartDefinition）
    // visualRoot: 对应当前视图中实际绘制图元的根 part（例如 PartUsage 或根 PartDefinition）
    QGraphicsItem* findPortByEndString(const QString& endString,
                                       Element* connectionOwner,
                                       Element* visualRoot);  // 返回QGraphicsItem以支持PortBasicItem
    Element* findPartByName(Element* parentPart, const QString& partName) const;
    Element* findPortByName(Element* part, const QString& portName) const;
    QString getPortEndString(Element* port, Element* partUsage, Element* connectionOwner) const;  // 从端口元素生成端点字符串（partUsage 是端口所属的 PartUsage 实例）
    Element* findPartUsageForPort(Element* port) const;  // 从场景中找到端口所属的 PartUsage 实例
    Element* findPartUsageForPortFromItem(Element* port, QGraphicsItem* portItem) const;  // 从图形项找到端口所属的 PartUsage 实例
    QString getPortEndStringRecursive(Element* port, Element* currentPart, Element* rootPart) const;  // 递归查找端口路径
    QString findPathToPart(Element* rootPart, Element* targetPart) const;  // 查找从 rootPart 到 targetPart 的路径
    
    // 拖放创建相关
    Element* findPartDefinitionByQualifiedName(const QString& qualifiedName);
    Element* findElementByQualifiedNameRecursive(Element* parent, const QString& qualifiedName);
    Element* findPartDefinitionByQualifiedNameHelper(Element* parent, const QString& qualifiedName) const;  // 辅助方法
    QString generateUniqueName(const QString& baseName, Element* parent);
    QString generateUniqueNameByPrefix(const QString& prefix, Element* parent);  // 通过前缀生成唯一名称（如 w1, w2）
    
    Element* m_part;
    ArchitectureScene* m_scene;
    QGraphicsItem* m_connectionPreviewItem = nullptr;  // 连接预览项
    QGraphicsItem* m_previewSourcePortItem = nullptr;  // 预览的源端口项
};

SYSML_NAMESPACE_END

#endif // GRAPHICVIEWWIDGET_H
