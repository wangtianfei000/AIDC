#ifndef ARCHITECTURESCENE_H
#define ARCHITECTURESCENE_H

#include <QGraphicsScene>
#include <QMap>
#include "../../../core/model/Element.h"
#include "../../../core/common/Global.h"

SYSML_NAMESPACE_BEGIN

// 前向声明
class PartGraphicsItem;
class PortGraphicsItem;
class ConnectionGraphicsItem;

/**
 * @brief ArchitectureScene 架构视图场景
 * 
 * 继承自QGraphicsScene，管理所有图形项
 * 提供元素到图形项的映射和查找功能
 */
class ArchitectureScene : public QGraphicsScene {
    Q_OBJECT

public:
    explicit ArchitectureScene(QObject* parent = nullptr);
    ~ArchitectureScene() override;
    
    // 图形项管理
    PartGraphicsItem* findPartItem(Element* part) const;
    PortGraphicsItem* findPortItem(Element* port) const;
    ConnectionGraphicsItem* findConnectionItem(Element* connection) const;
    
    // 注册图形项
    void registerPartItem(Element* part, PartGraphicsItem* item);
    void registerPortItem(Element* port, PortGraphicsItem* item);
    void registerConnectionItem(Element* connection, ConnectionGraphicsItem* item);
    
    // 取消注册图形项
    void unregisterPartItem(Element* part);
    void unregisterPortItem(Element* port);
    void unregisterConnectionItem(Element* connection);
    
    // 清除所有图形项
    void clearAllItems();
    
    // 当端口图元移动时，更新与之相连的所有连接线
    void updateConnectionsForPort(QGraphicsItem* portItem);
    
signals:
    void partDoubleClicked(Element* part);
    void partSelected(Element* part);

protected:
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;

private:
    // 元素到图形项的映射
    QMap<Element*, PartGraphicsItem*> m_partItems;
    QMap<Element*, PortGraphicsItem*> m_portItems;
    QMap<Element*, ConnectionGraphicsItem*> m_connectionItems;
};

SYSML_NAMESPACE_END

#endif // ARCHITECTURESCENE_H
