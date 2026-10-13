#ifndef PORTBASICITEM_H
#define PORTBASICITEM_H

#include <QGraphicsItem>
#include "../../../core/model/Element.h"
#include "../../../core/common/Global.h"
#include "PortGraphicMetadata.h"
#include "GraphicsItemTypes.h"

SYSML_NAMESPACE_BEGIN

// 前向声明
class PortLabelItem;

/**
 * @brief PortBasicItem 基础视图中端口的图形项
 * 
 * 继承自QGraphicsItem，显示端口的基础信息
 * 作为PartBasicItem的子项，显示在part的两侧
 */
class PortBasicItem : public QGraphicsItem {
public:
    explicit PortBasicItem(Element* port, PortGraphicMetadata* metadata = nullptr, QGraphicsItem* parent = nullptr);
    ~PortBasicItem() override;
    
    // QGraphicsItem接口
    int type() const override { return GraphicsItemTypes::PortBasicItem; }
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
    
    // 元素访问
    Element* port() const { return m_port; }
    PortGraphicMetadata* metadata() const { return m_metadata; }
    
    // 更新标签位置（当端口位置改变时调用）
    void updateLabelPosition();
    
    // 连接点（用于连接线）
    QPointF connectionPoint() const;
    
    // 设置BasicViewWidget（用于编辑功能）
    void setBasicViewWidget(class BasicViewWidget* widget) { m_basicViewWidget = widget; }

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    void setupLabel();  // 设置端口标签
    
    Element* m_port;
    PortGraphicMetadata* m_metadata;
    PortLabelItem* m_labelItem = nullptr;  // 端口标签项
    QRectF m_boundingRect;
    class BasicViewWidget* m_basicViewWidget = nullptr;  // 用于编辑功能
    bool m_creatingConnection = false;  // 是否正在创建连接
    QPointF m_pressPos;  // 鼠标按下的位置
    static constexpr double PORT_SIZE = 8.0;  // 端口大小（像素）
};

SYSML_NAMESPACE_END

#endif // PORTBASICITEM_H
