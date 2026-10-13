#ifndef CONNECTIONGRAPHICSITEM_H
#define CONNECTIONGRAPHICSITEM_H

#include <QGraphicsItem>
#include "../../../core/model/Element.h"
#include "../../../core/common/Global.h"
#include "ConnectionGraphicMetadata.h"
#include "GraphicsItemTypes.h"

SYSML_NAMESPACE_BEGIN

// 前向声明
class PortGraphicsItem;
class PortBasicItem;

/**
 * @brief ConnectionGraphicsItem 连接的图形项
 * 
 * 继承自QGraphicsItem，表示一个连接的图形表示
 * 作为Scene的直接子项
 */
class ConnectionGraphicsItem : public QGraphicsItem {
public:
    explicit ConnectionGraphicsItem(Element* connection, ConnectionGraphicMetadata* metadata = nullptr, QGraphicsItem* parent = nullptr);
    ~ConnectionGraphicsItem() override = default;
    
    // QGraphicsItem接口
    int type() const override { return GraphicsItemTypes::ConnectionGraphicsItem; }
    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;
    
    // 端点设置（支持PortGraphicsItem和PortBasicItem）
    void setSourcePort(PortGraphicsItem* portItem);
    void setTargetPort(PortGraphicsItem* portItem);
    void setSourcePortBasic(PortBasicItem* portItem);  // 支持PortBasicItem
    void setTargetPortBasic(PortBasicItem* portItem);  // 支持PortBasicItem
    
    // 更新路径（当端口位置改变时）
    void updatePath();
    // 端口移动时通知连接线更新路径
    void onPortMoved(QGraphicsItem* portItem);
    
    Element* connection() const { return m_connection; }
    ConnectionGraphicMetadata* metadata() const { return m_metadata; }

private:
    void createOrthogonalPath(const QPointF& source, const QPointF& target);
    QPointF getPortConnectionPoint(QGraphicsItem* portItem) const;  // 获取端口的连接点
    
    Element* m_connection;
    ConnectionGraphicMetadata* m_metadata;
    QGraphicsItem* m_sourcePort;  // 可以是PortGraphicsItem或PortBasicItem
    QGraphicsItem* m_targetPort;  // 可以是PortGraphicsItem或PortBasicItem
    QPainterPath m_path;
    QRectF m_boundingRect;
};

SYSML_NAMESPACE_END

#endif // CONNECTIONGRAPHICSITEM_H
