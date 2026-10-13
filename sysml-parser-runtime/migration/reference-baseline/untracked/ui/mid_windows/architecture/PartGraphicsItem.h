#ifndef PARTGRAPHICSITEM_H
#define PARTGRAPHICSITEM_H

#include <QGraphicsItem>
#include <QList>
#include "../../../core/model/Element.h"
#include "../../../core/common/Global.h"
#include "PartGraphicMetadata.h"
#include "GraphicsItemTypes.h"

SYSML_NAMESPACE_BEGIN

// 前向声明
class PortGraphicsItem;

/**
 * @brief PartGraphicsItem Part的图形项
 * 
 * 继承自QGraphicsItem，表示一个part的图形表示
 * 支持嵌套：子part作为子item
 */
class PartGraphicsItem : public QGraphicsItem {
public:
    explicit PartGraphicsItem(Element* part, PartGraphicMetadata* metadata = nullptr, QGraphicsItem* parent = nullptr);
    ~PartGraphicsItem() override;
    
    // QGraphicsItem接口
    int type() const override { return GraphicsItemTypes::PartGraphicsItem; }
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;
    
    // 元素访问
    Element* part() const { return m_part; }
    PartGraphicMetadata* metadata() const { return m_metadata; }
    
    // 检查是否正在调整大小
    bool isResizing() const { return m_isResizing; }
    
    // 子part管理
    void addChildPart(PartGraphicsItem* childPart);
    void removeChildPart(PartGraphicsItem* childPart);
    QList<PartGraphicsItem*> childParts() const { return m_childParts; }
    
    // 端口管理
    void addPort(PortGraphicsItem* portItem);
    void removePort(PortGraphicsItem* portItem);
    QList<PortGraphicsItem*> ports() const { return m_ports; }
    
    // 更新图形
    void updateGraphics();
    
    // 大小调整相关
    void updateBoundingRect();  // 公开，供 ResizeHandleItem 调用
    void updateResizeHandlesPosition();  // 更新调整手柄位置

private:
    QPainterPath createShapePath(const QRectF& rect) const;
    void createResizeHandles();
    void removeResizeHandles();
    
    // 调整大小的手柄内部类
    class ResizeHandleItem;
    
    Element* m_part;
    PartGraphicMetadata* m_metadata;
    QList<PartGraphicsItem*> m_childParts;
    QList<PortGraphicsItem*> m_ports;
    QRectF m_boundingRect;
    QList<ResizeHandleItem*> m_resizeHandles;  // 调整手柄列表
    bool m_isResizing = false;  // 是否正在调整大小（用于阻止 itemChange 中的自动保存）
};

SYSML_NAMESPACE_END

#endif // PARTGRAPHICSITEM_H
