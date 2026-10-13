#include "ArchitectureScene.h"
#include "PartGraphicsItem.h"
#include "PortGraphicsItem.h"
#include "ConnectionGraphicsItem.h"
#include <QGraphicsSceneMouseEvent>

SYSML_NAMESPACE_BEGIN

ArchitectureScene::ArchitectureScene(QObject* parent)
    : QGraphicsScene(parent)
{
    setBackgroundBrush(QBrush(Qt::white));
    setSceneRect(0, 0, 2000, 2000);
}

ArchitectureScene::~ArchitectureScene() {
    clearAllItems();
}

PartGraphicsItem* ArchitectureScene::findPartItem(Element* part) const {
    return m_partItems.value(part, nullptr);
}

PortGraphicsItem* ArchitectureScene::findPortItem(Element* port) const {
    return m_portItems.value(port, nullptr);
}

ConnectionGraphicsItem* ArchitectureScene::findConnectionItem(Element* connection) const {
    return m_connectionItems.value(connection, nullptr);
}

void ArchitectureScene::registerPartItem(Element* part, PartGraphicsItem* item) {
    if (part && item) {
        m_partItems[part] = item;
    }
}

void ArchitectureScene::registerPortItem(Element* port, PortGraphicsItem* item) {
    if (port && item) {
        m_portItems[port] = item;
    }
}

void ArchitectureScene::registerConnectionItem(Element* connection, ConnectionGraphicsItem* item) {
    if (connection && item) {
        m_connectionItems[connection] = item;
    }
}

void ArchitectureScene::unregisterPartItem(Element* part) {
    if (part) {
        m_partItems.remove(part);
    }
}

void ArchitectureScene::unregisterPortItem(Element* port) {
    if (port) {
        m_portItems.remove(port);
    }
}

void ArchitectureScene::unregisterConnectionItem(Element* connection) {
    if (connection) {
        m_connectionItems.remove(connection);
    }
}

void ArchitectureScene::clearAllItems() {
    // 清除所有图形项
    clear();
    
    // 清除映射
    m_partItems.clear();
    m_portItems.clear();
    m_connectionItems.clear();
}

void ArchitectureScene::updateConnectionsForPort(QGraphicsItem* portItem) {
    if (!portItem) {
        return;
    }
    
    // 遍历所有连接线，找到使用该端口作为端点的连接线，并更新其路径
    auto it = m_connectionItems.constBegin();
    while (it != m_connectionItems.constEnd()) {
        ConnectionGraphicsItem* connItem = it.value();
        if (connItem) {
            connItem->onPortMoved(portItem);
        }
        ++it;
    }
}

void ArchitectureScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) {
    QGraphicsScene::mouseDoubleClickEvent(event);
    
    // 查找被双击的item
    QGraphicsItem* item = itemAt(event->scenePos(), QTransform());
    if (!item) {
        return;
    }
    
    // 检查是否是PartGraphicsItem
    PartGraphicsItem* partItem = qgraphicsitem_cast<PartGraphicsItem*>(item);
    if (partItem && partItem->part()) {
        emit partDoubleClicked(partItem->part());
    }
}

SYSML_NAMESPACE_END
