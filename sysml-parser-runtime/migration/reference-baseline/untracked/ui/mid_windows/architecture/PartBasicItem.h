#ifndef PARTBASICITEM_H
#define PARTBASICITEM_H

#include <QGraphicsItem>
#include <QList>
#include <QGraphicsTextItem>
#include "../../../core/model/Element.h"
#include "../../../core/common/Global.h"
#include "PartBasicViewMetadata.h"
#include "PortBasicItem.h"
#include "GraphicsItemTypes.h"

SYSML_NAMESPACE_BEGIN

// 前向声明
class PartNameItem;
class DocItem;
class AttributeItem;
class ItemItem;

/**
 * @brief PartBasicItem 基础视图中part的图形项
 * 
 * 继承自QGraphicsItem，显示part的基础信息
 * 包括：外形、端口（两侧布局）、属性、item、doc
 */
class PartBasicItem : public QGraphicsItem {
public:
    explicit PartBasicItem(Element* part, PartBasicViewMetadata* metadata = nullptr, QGraphicsItem* parent = nullptr);
    ~PartBasicItem() override;
    
    // QGraphicsItem接口
    int type() const override { return GraphicsItemTypes::PartBasicItem; }
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
    bool contains(const QPointF& point) const override;
    
    // 元素访问
    Element* part() const { return m_part; }
    PartBasicViewMetadata* metadata() const { return m_metadata; }
    
    // 更新显示
    void updateDisplay();
    
    // 设置BasicViewWidget（用于编辑功能）
    void setBasicViewWidget(class BasicViewWidget* widget) { m_basicViewWidget = widget; }
    
protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;

private:
    void setupPorts();      // 设置端口（两侧布局）
    void setupAttributes(); // 设置属性列表
    void setupItems();      // 设置item列表
    void setupDoc();        // 设置doc
    void setupName();       // 设置part名称
    void updateBoundingRect(); // 更新边界矩形
    
    Element* m_part;
    PartBasicViewMetadata* m_metadata;
    PartNameItem* m_nameItem = nullptr;  // Part 名称项
    DocItem* m_docItem = nullptr;        // Doc 内容项
    QList<AttributeItem*> m_attributeItems;  // 属性项列表
    QList<ItemItem*> m_itemItems;   // Item 项列表
    QList<PortBasicItem*> m_ports;
    QRectF m_boundingRect;
    class BasicViewWidget* m_basicViewWidget = nullptr;  // 用于编辑功能
    
    // 最大宽度限制
    static constexpr double MAX_WIDTH = 600.0;  // Part 最大宽度
    
    // 布局常量
    static constexpr double PADDING = 10.0;        // 内边距
    static constexpr double PORT_SPACING = 20.0;    // 端口间距
    static constexpr double PORT_OFFSET = 15.0;    // 端口距离边缘的偏移
    static constexpr double TEXT_LINE_HEIGHT = 16.0; // 文本行高
    static constexpr double MIN_WIDTH = 200.0;      // 最小宽度
    static constexpr double MIN_HEIGHT = 100.0;     // 最小高度
    static constexpr double HEADER_HEIGHT = 30.0;   // 标题区域高度
};

SYSML_NAMESPACE_END

#endif // PARTBASICITEM_H
