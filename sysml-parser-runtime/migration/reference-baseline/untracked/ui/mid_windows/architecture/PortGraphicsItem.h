#ifndef PORTGRAPHICSITEM_H
#define PORTGRAPHICSITEM_H

#include <QGraphicsItem>
#include "../../../core/model/Element.h"
#include "../../../core/common/Global.h"
#include "PortGraphicMetadata.h"
#include "GraphicsItemTypes.h"

SYSML_NAMESPACE_BEGIN

/**
 * @brief PortGraphicsItem 端口的图形项
 * 
 * 继承自QGraphicsItem，表示一个端口的图形表示
 * 作为PartGraphicsItem的子项
 */
class PortGraphicsItem : public QGraphicsItem {
public:
    explicit PortGraphicsItem(Element* port, PortGraphicMetadata* metadata = nullptr, QGraphicsItem* parent = nullptr);
    ~PortGraphicsItem() override = default;
    
    // QGraphicsItem接口
    int type() const override { return GraphicsItemTypes::PortGraphicsItem; }
    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    
    // 连接创建相关
    void setCreatingConnection(bool creating) { m_creatingConnection = creating; }
    bool isCreatingConnection() const { return m_creatingConnection; }
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;
    
    // 元素访问
    Element* port() const { return m_port; }
    PortGraphicMetadata* metadata() const { return m_metadata; }
    
    // 连接点（用于连接线）
    QPointF connectionPoint() const;

private:
    void updateBoundingRect();
    
    Element* m_port;
    PortGraphicMetadata* m_metadata;
    QRectF m_boundingRect;
    bool m_creatingConnection = false;  // 是否正在创建连接
    QPointF m_pressPos;  // 鼠标按下的位置
};

SYSML_NAMESPACE_END

#endif // PORTGRAPHICSITEM_H
