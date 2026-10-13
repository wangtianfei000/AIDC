#include "PartBasicItem.h"
#include "PortBasicItem.h"
#include "PartNameItem.h"
#include "DocItem.h"
#include "AttributeItem.h"
#include "ItemItem.h"
#include "BasicViewWidget.h"
#include "AttributeEditDialog.h"
#include "PortEditDialog.h"
#include "ItemEditDialog.h"
#include "../../../core/model/Feature.h"
#include "../../../core/model/DocStatement.h"
#include "../../../core/model/AttributeDefinition.h"
#include "../../../parser/src/annotation/AnnotationParser.h"
#include "BaseGraphicMetadata.h"
#include "PortGraphicMetadata.h"
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsTextItem>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsSceneContextMenuEvent>
#include <QMenu>
#include <QMessageBox>
#include <QObject>
#include <QToolTip>
#include <QFont>
#include <QFontMetrics>
#include <QTextDocument>
#include <QDebug>

SYSML_NAMESPACE_BEGIN

PartBasicItem::PartBasicItem(Element* part, PartBasicViewMetadata* metadata, QGraphicsItem* parent)
    : QGraphicsItem(parent)
    , m_part(part)
    , m_metadata(metadata)
{
    // 设置标志
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsMovable, false);  // 基础视图中part不可移动
    setAcceptHoverEvents(true);
    
    // 先设置名称、doc、属性、items（用于计算边界矩形）
    // 注意：显示顺序为：Header -> Doc -> Attributes -> Items
    setupName();
    setupDoc();
    setupAttributes();
    setupItems();
    
    // 计算边界矩形（在设置端口之前，因为端口位置需要用到宽度）
    updateBoundingRect();
    
    // 然后设置端口（端口位置需要用到 boundingRect）
    setupPorts();
}

PartBasicItem::~PartBasicItem() {
    // 清理子项（Qt会自动清理，但显式清理更安全）
    if (m_nameItem) {
        delete m_nameItem;
    }
    if (m_docItem) {
        delete m_docItem;
    }
    qDeleteAll(m_ports);
    qDeleteAll(m_attributeItems);
    qDeleteAll(m_itemItems);
}

QRectF PartBasicItem::boundingRect() const {
    return m_boundingRect;
}

bool PartBasicItem::contains(const QPointF& point) const {
    return m_boundingRect.contains(point);
}

void PartBasicItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    Q_UNUSED(option)
    Q_UNUSED(widget)
    
    // 根据SysML规范绘制part外形
    QRectF rect = boundingRect();
    
    // 绘制外形（遵循SysML规范：圆角矩形）
    painter->setPen(QPen(Qt::black, 2));
    painter->setBrush(QBrush(Qt::white));
    painter->drawRoundedRect(rect, 5, 5);
    
    // 绘制标题区域分割线（part名称下方）
    QRectF headerRect = QRectF(rect.x(), rect.y(), rect.width(), HEADER_HEIGHT);
    painter->setPen(QPen(Qt::black, 1));
    painter->drawLine(headerRect.bottomLeft(), headerRect.bottomRight());
    
    // 注意：part 名称、doc、属性等文本由各自的 Item 绘制，不需要在这里绘制
    
    // 绘制 doc 和 attributes 之间的分割线（如果有doc且有属性/items）
    if (m_docItem && (!m_attributeItems.isEmpty() || !m_itemItems.isEmpty())) {
        // 使用 doc Item 的实际位置计算分割线位置
        QRectF docRect = m_docItem->boundingRect();
        QPointF docPos = m_docItem->pos();
        double separatorY = docPos.y() + docRect.height() + PADDING / 2;
        // 分割线在 doc 和 attributes 之间的间距中间
        painter->setPen(QPen(Qt::gray, 1, Qt::DashLine));
        painter->drawLine(QPointF(PADDING, separatorY), 
                         QPointF(rect.width() - PADDING, separatorY));
    }
    
    // 调试：绘制边界矩形（可选，用于调试）
    // painter->setPen(QPen(Qt::red, 1, Qt::DashLine));
    // painter->drawRect(rect);
}

void PartBasicItem::setupPorts() {
    if (!m_part) {
        return;
    }
    
    // 检查是否显示端口
    bool showPorts = m_metadata ? m_metadata->showPorts() : true;
    if (!showPorts) {
        return;
    }
    
    // 获取端口列表
    QList<Element*> ports = m_part->getPorts();
    if (ports.isEmpty()) {
        return;
    }
    
    // 清除现有端口
    qDeleteAll(m_ports);
    m_ports.clear();
    
    // 根据 portPosition 分类端口
    QList<Element*> leftPorts, rightPorts, topPorts, bottomPorts;
    
    for (Element* port : ports) {
        // 读取端口元数据
        BaseGraphicMetadata* metadata = AnnotationParser::parseGraphicMetadata(port);
        PortGraphicMetadata* portMeta = qobject_cast<PortGraphicMetadata*>(metadata);
        
        QString position = portMeta ? portMeta->portPosition() : "left";
        
        // 根据 portPosition 分类
        if (position == "right") {
            rightPorts.append(port);
        } else if (position == "top") {
            topPorts.append(port);
        } else if (position == "bottom") {
            bottomPorts.append(port);
        } else {
            leftPorts.append(port);  // 默认左侧
        }
        
        // 注意：metadata 不需要在这里删除，因为 parseGraphicMetadata 返回的可能是
        // 从 Annotation 中解析的元数据，或者是新创建的。如果是新创建的，会在后续使用后处理
    }
    
    // 处理左侧端口（端口中心应该在矩形左边缘，端口完全在外部）
    double leftY = HEADER_HEIGHT + PORT_OFFSET;
    for (Element* port : leftPorts) {
        BaseGraphicMetadata* metadata = AnnotationParser::parseGraphicMetadata(port);
        PortGraphicMetadata* portMeta = qobject_cast<PortGraphicMetadata*>(metadata);
        double portSize = portMeta ? portMeta->portSize() : 8.0;
        PortBasicItem* portItem = new PortBasicItem(port, portMeta, this);
        // 设置BasicViewWidget（用于编辑功能）
        portItem->setBasicViewWidget(m_basicViewWidget);
        // 端口中心在左边缘（x=0），端口完全在矩形外部
        portItem->setPos(-portSize/2, leftY);
        // 更新标签位置（在端口位置设置后）
        portItem->updateLabelPosition();
        m_ports.append(portItem);
        leftY += PORT_SPACING;
        // 注意：如果 metadata 是从 Annotation 解析的，不需要删除，因为它是 Annotation 的一部分
    }
    
    // 处理右侧端口（端口中心应该在矩形右边缘，端口完全在外部）
    double rightY = HEADER_HEIGHT + PORT_OFFSET;
    for (Element* port : rightPorts) {
        BaseGraphicMetadata* metadata = AnnotationParser::parseGraphicMetadata(port);
        PortGraphicMetadata* portMeta = qobject_cast<PortGraphicMetadata*>(metadata);
        double portSize = portMeta ? portMeta->portSize() : 8.0;
        PortBasicItem* portItem = new PortBasicItem(port, portMeta, this);
        // 设置BasicViewWidget（用于编辑功能）
        portItem->setBasicViewWidget(m_basicViewWidget);
        // 端口中心在右边缘（x=width），端口完全在矩形外部
        portItem->setPos(m_boundingRect.width() + portSize/2, rightY);
        // 更新标签位置（在端口位置设置后）
        portItem->updateLabelPosition();
        m_ports.append(portItem);
        rightY += PORT_SPACING;
    }
    
    // 处理顶部端口（端口中心应该在矩形上边缘，端口完全在外部）
    double topX = PADDING;
    for (Element* port : topPorts) {
        BaseGraphicMetadata* metadata = AnnotationParser::parseGraphicMetadata(port);
        PortGraphicMetadata* portMeta = qobject_cast<PortGraphicMetadata*>(metadata);
        double portSize = portMeta ? portMeta->portSize() : 8.0;
        PortBasicItem* portItem = new PortBasicItem(port, portMeta, this);
        // 设置BasicViewWidget（用于编辑功能）
        portItem->setBasicViewWidget(m_basicViewWidget);
        // 端口中心在上边缘（y=0），端口完全在矩形外部
        portItem->setPos(topX, -portSize/2);
        // 更新标签位置（在端口位置设置后）
        portItem->updateLabelPosition();
        m_ports.append(portItem);
        topX += PORT_SPACING;
    }
    
    // 处理底部端口（端口中心应该在矩形下边缘，端口完全在外部）
    double bottomX = PADDING;
    for (Element* port : bottomPorts) {
        BaseGraphicMetadata* metadata = AnnotationParser::parseGraphicMetadata(port);
        PortGraphicMetadata* portMeta = qobject_cast<PortGraphicMetadata*>(metadata);
        double portSize = portMeta ? portMeta->portSize() : 8.0;
        PortBasicItem* portItem = new PortBasicItem(port, portMeta, this);
        // 设置BasicViewWidget（用于编辑功能）
        portItem->setBasicViewWidget(m_basicViewWidget);
        // 端口中心在下边缘（y=height），端口完全在矩形外部
        portItem->setPos(bottomX, m_boundingRect.height() + portSize/2);
        // 更新标签位置（在端口位置设置后）
        portItem->updateLabelPosition();
        m_ports.append(portItem);
        bottomX += PORT_SPACING;
    }
}

void PartBasicItem::setupAttributes() {
    if (!m_part) {
        return;
    }
    
    // 检查是否显示属性
    bool showAttributes = m_metadata ? m_metadata->showAttributes() : true;
    if (!showAttributes) {
        return;
    }
    
    // 获取属性列表
    QList<Element*> attributes = m_part->getAttributes();
    if (attributes.isEmpty()) {
        return;
    }
    
    // 清除现有属性项
    qDeleteAll(m_attributeItems);
    m_attributeItems.clear();
    
    // 创建属性项
    // 属性显示在 doc 下方
    double yPos = HEADER_HEIGHT + PADDING;
    if (m_docItem) {
        // 如果有 doc，属性显示在 doc 下方
        yPos += m_docItem->boundingRect().height() + PADDING;
    }
    
    for (Element* attr : attributes) {
        QString attrName = attr->declaredName();
        QString attrValue;
        
        // 尝试获取值
        if (Feature* feature = qobject_cast<Feature*>(attr)) {
            attrValue = feature->valueExpression();
        }
        
        AttributeItem* attrItem = new AttributeItem(attr, attrName, attrValue, this);
        // 设置BasicViewWidget（用于编辑功能）
        attrItem->setBasicViewWidget(m_basicViewWidget);
        // 设置初始最大宽度（使用 MIN_WIDTH，后续在 updateBoundingRect() 中会更新）
        attrItem->setMaxWidth(MIN_WIDTH - 2 * PADDING);
        attrItem->setPos(PADDING, yPos);
        m_attributeItems.append(attrItem);
        
        yPos += TEXT_LINE_HEIGHT;
    }
}

void PartBasicItem::setupItems() {
    if (!m_part) {
        return;
    }
    
    // 检查是否显示items
    bool showItems = m_metadata ? m_metadata->showItems() : true;
    if (!showItems) {
        return;
    }
    
    // 获取item列表
    QList<Element*> items = m_part->getItems();
    if (items.isEmpty()) {
        return;
    }
    
    // 清除现有item项
    qDeleteAll(m_itemItems);
    m_itemItems.clear();
    
    // 创建item文本项
    // Items 显示在 attributes 下方
    double yPos = HEADER_HEIGHT + PADDING;
    if (m_docItem) {
        // 如果有 doc，先加上 doc 的高度
        yPos += m_docItem->boundingRect().height() + PADDING;
    }
    if (!m_attributeItems.isEmpty()) {
        // 如果有属性，items显示在属性下方
        yPos += m_attributeItems.size() * TEXT_LINE_HEIGHT + PADDING;
    }
    
    for (Element* item : items) {
        QString itemName = item->declaredName();
        ItemItem* itemItem = new ItemItem(item, itemName, this);
        // 设置BasicViewWidget（用于编辑功能）
        itemItem->setBasicViewWidget(m_basicViewWidget);
        // 设置初始最大宽度（使用 MIN_WIDTH，后续在 updateBoundingRect() 中会更新）
        itemItem->setMaxWidth(MIN_WIDTH - 2 * PADDING);
        itemItem->setPos(PADDING, yPos);
        m_itemItems.append(itemItem);
        
        yPos += TEXT_LINE_HEIGHT;
    }
}

void PartBasicItem::setupName() {
    if (!m_part) {
        return;
    }
    
    // 清除现有名称项
    if (m_nameItem) {
        delete m_nameItem;
        m_nameItem = nullptr;
    }
    
    // 创建名称项
    QString partName = m_part->declaredName();
    if (partName.isEmpty()) {
        partName = "Part";
    }
    
    m_nameItem = new PartNameItem(partName, this);
    // 设置初始最大宽度（使用 MIN_WIDTH，后续在 updateBoundingRect() 中会更新）
    double initialWidth = MIN_WIDTH - 2 * PADDING;
    qDebug() << "[PartBasicItem] setupName: creating PartNameItem with name:" << partName.left(50) << "initialWidth:" << initialWidth;
    m_nameItem->setMaxWidth(initialWidth);
    // 名称项的位置在 updateBoundingRect() 后设置
}

void PartBasicItem::setupDoc() {
    if (!m_part) {
        return;
    }
    
    // 检查是否显示doc
    bool showDoc = m_metadata ? m_metadata->showDoc() : true;
    if (!showDoc) {
        if (m_docItem) {
            delete m_docItem;
            m_docItem = nullptr;
        }
        return;
    }
    
    // 获取doc
    Element* docElement = m_part->getDoc();
    if (!docElement) {
        if (m_docItem) {
            delete m_docItem;
            m_docItem = nullptr;
        }
        return;
    }
    
    // 获取doc内容
    QString docContent;
    if (DocStatement* doc = qobject_cast<DocStatement*>(docElement)) {
        docContent = doc->content();
    }
    
    if (docContent.isEmpty()) {
        if (m_docItem) {
            delete m_docItem;
            m_docItem = nullptr;
        }
        return;
    }
    
    // 清除现有doc项
    if (m_docItem) {
        delete m_docItem;
        m_docItem = nullptr;
    }
    
    // 创建doc项
    m_docItem = new DocItem(docContent, this);
    // 设置初始最大宽度（使用 MIN_WIDTH，后续在 updateBoundingRect() 中会更新）
    double initialWidth = MIN_WIDTH - 2 * PADDING;
    qDebug() << "[PartBasicItem] setupDoc: creating DocItem with content:" << docContent.left(50) << "initialWidth:" << initialWidth;
    m_docItem->setMaxWidth(initialWidth);
    // Doc 项的位置在 updateBoundingRect() 后设置
}

void PartBasicItem::updateBoundingRect() {
    // 计算所需的高度
    // 显示顺序：Header -> Doc -> Attributes -> Items
    double height = HEADER_HEIGHT + PADDING;
    
    // Doc高度（在最上面）
    if (m_docItem) {
        height += m_docItem->boundingRect().height();
        if (!m_attributeItems.isEmpty() || !m_itemItems.isEmpty()) {
            height += PADDING;  // doc 与下方内容的间距
        }
    }
    
    // 属性高度
    if (!m_attributeItems.isEmpty()) {
        height += m_attributeItems.size() * TEXT_LINE_HEIGHT;
    }
    
    // Items高度
    if (!m_itemItems.isEmpty()) {
        if (!m_attributeItems.isEmpty()) {
            height += PADDING;  // 属性与items之间的间距
        }
        height += m_itemItems.size() * TEXT_LINE_HEIGHT;
    }
    
    // 确保最小高度
    if (height < MIN_HEIGHT) {
        height = MIN_HEIGHT;
    }
    
    // 计算所需宽度（关键：限制在 MAX_WIDTH 内，强制文本省略）
    // 问题：之前根据文本实际宽度计算 width，导致 width 过大（如 383），maxWidth 也过大，文本不需要省略
    // 解决：直接使用 MAX_WIDTH 作为 part 的最大宽度，不再根据名称和 doc 的实际宽度扩展
    
    // 计算最小所需宽度（基于属性、items 等较短的内容，不包括名称和 doc）
    double width = MIN_WIDTH;
    if (!m_ports.isEmpty()) {
        width += PORT_OFFSET * 2;  // 为端口留出空间
    }
    
    QFont attrFont("Arial", 10);
    QFontMetrics attrFm(attrFont);
    
    // 考虑属性项宽度（属性通常不会太长）
    for (AttributeItem* item : m_attributeItems) {
        QString attrText = item->fullText();
        double attrTextWidth = attrFm.horizontalAdvance(attrText) + 2 * PADDING;
        if (attrTextWidth > width) {
            width = qMin(attrTextWidth, MAX_WIDTH);
        }
    }
    
    // 考虑 items 宽度
    QFont itemFont("Arial", 10);
    QFontMetrics itemFm(itemFont);
    for (ItemItem* item : m_itemItems) {
        QString itemText = item->fullText();
        double itemTextWidth = itemFm.horizontalAdvance(itemText) + 2 * PADDING;
        if (itemTextWidth > width) {
            width = qMin(itemTextWidth, MAX_WIDTH);
        }
    }
    
    // 关键：不再根据名称和 doc 的实际宽度扩展 width
    // 直接使用 MAX_WIDTH 作为上限，确保即使文本很长，part 宽度也不会超过 MAX_WIDTH
    width = qMin(width, MAX_WIDTH);
    
    qDebug() << "[PartBasicItem] updateBoundingRect: calculated width:" << width 
             << "MAX_WIDTH:" << MAX_WIDTH;
    
    m_boundingRect = QRectF(0, 0, width, height);
    prepareGeometryChange();
    
    qDebug() << "[PartBasicItem] updateBoundingRect: final width:" << width << "height:" << height;
    
    // 更新名称项和 doc 项的宽度和位置
    // 关键：availableWidth 基于固定的 width（已限制在 MAX_WIDTH 内），而不是文本实际宽度
    if (m_nameItem) {
        double availableWidth = width - 2 * PADDING;
        qDebug() << "[PartBasicItem] updateBoundingRect: setting nameItem maxWidth:" << availableWidth 
                 << "partWidth:" << width 
                 << "MAX_WIDTH:" << MAX_WIDTH
                 << "name:" << m_nameItem->text().left(50);
        m_nameItem->setMaxWidth(availableWidth);
        m_nameItem->setPos(PADDING, 0);
        qDebug() << "[PartBasicItem] nameItem after setMaxWidth - boundingRect:" << m_nameItem->boundingRect();
    }
    
    if (m_docItem) {
        double availableWidth = width - 2 * PADDING;
        qDebug() << "[PartBasicItem] updateBoundingRect: setting docItem maxWidth:" << availableWidth 
                 << "partWidth:" << width 
                 << "MAX_WIDTH:" << MAX_WIDTH
                 << "docContent:" << m_docItem->text().left(50);
        m_docItem->setMaxWidth(availableWidth);
        double docY = HEADER_HEIGHT + PADDING;
        m_docItem->setPos(PADDING, docY);
        qDebug() << "[PartBasicItem] docItem after setMaxWidth - boundingRect:" << m_docItem->boundingRect();
    }
    
    // 更新属性项的最大宽度
    for (AttributeItem* item : m_attributeItems) {
        double availableWidth = width - 2 * PADDING;
        item->setMaxWidth(availableWidth);
    }
    
    // 更新item项的最大宽度
    for (ItemItem* item : m_itemItems) {
        double availableWidth = width - 2 * PADDING;
        item->setMaxWidth(availableWidth);
    }
}

void PartBasicItem::updateDisplay() {
    // 清除现有内容
    if (m_nameItem) {
        delete m_nameItem;
        m_nameItem = nullptr;
    }
    if (m_docItem) {
        delete m_docItem;
        m_docItem = nullptr;
    }
    qDeleteAll(m_ports);
    m_ports.clear();
    qDeleteAll(m_attributeItems);
    m_attributeItems.clear();
    qDeleteAll(m_itemItems);
    m_itemItems.clear();
    
    // 重新设置（注意顺序：name -> doc -> attributes -> items）
    setupName();
    setupDoc();
    setupAttributes();
    setupItems();
    updateBoundingRect();
    setupPorts();
    
    // 更新显示
    update();
}

void PartBasicItem::hoverEnterEvent(QGraphicsSceneHoverEvent* event) {
    // Tooltip 由各个 Item 自己处理，这里不需要额外处理
    QGraphicsItem::hoverEnterEvent(event);
}

void PartBasicItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* event) {
    QGraphicsItem::hoverLeaveEvent(event);
}

void PartBasicItem::contextMenuEvent(QGraphicsSceneContextMenuEvent* event) {
    qDebug() << "[PartBasicItem] contextMenuEvent triggered"
             << "part =" << (m_part ? m_part->declaredName() : "null")
             << "basicViewWidget =" << m_basicViewWidget;

    if (!m_basicViewWidget || !m_part) {
        qDebug() << "[PartBasicItem] contextMenuEvent: missing widget or part, ignore.";
        return;
    }

    QMenu menu;

    // 无论点击在哪个区域，都提供三个添加操作，方便使用
    QAction* addAttrAction = menu.addAction("添加属性");
    QObject::connect(addAttrAction, &QAction::triggered, [this]() {
        if (!m_basicViewWidget || !m_part) {
            return;
        }

        // 获取现有属性名称
        QList<Element*> attributes = m_part->getAttributes();
        QStringList existingNames;
        for (Element* attr : attributes) {
            existingNames << attr->declaredName();
        }

        AttributeEditDialog dialog(m_basicViewWidget);
        dialog.setExistingNames(existingNames);

        if (dialog.exec() == QDialog::Accepted) {
            m_basicViewWidget->createAttribute(dialog.name(), dialog.type(), dialog.value());
        }
    });

    QAction* addPortAction = menu.addAction("添加端口");
    QObject::connect(addPortAction, &QAction::triggered, [this]() {
        if (!m_basicViewWidget || !m_part) {
            return;
        }

        // 获取现有端口名称
        QList<Element*> ports = m_part->getPorts();
        QStringList existingNames;
        for (Element* port : ports) {
            existingNames << port->declaredName();
        }

        // 获取可用的接口类型
        QStringList availableTypes = m_basicViewWidget->getAvailablePortTypes();

        PortEditDialog dialog(m_basicViewWidget);
        dialog.setExistingNames(existingNames);
        dialog.setAvailableTypes(availableTypes);

        if (dialog.exec() == QDialog::Accepted) {
            m_basicViewWidget->createPort(dialog.name(), dialog.position(), dialog.type());
        }
    });

    QAction* addItemAction = menu.addAction("添加Item");
    QObject::connect(addItemAction, &QAction::triggered, [this]() {
        if (!m_basicViewWidget || !m_part) {
            return;
        }

        // 获取现有item名称
        QList<Element*> items = m_part->getItems();
        QStringList existingNames;
        for (Element* item : items) {
            existingNames << item->declaredName();
        }

        ItemEditDialog dialog(m_basicViewWidget);
        dialog.setExistingNames(existingNames);

        if (dialog.exec() == QDialog::Accepted) {
            m_basicViewWidget->createItem(dialog.name(), dialog.type());
        }
    });

    qDebug() << "[PartBasicItem] contextMenuEvent: show menu with actions:"
             << "addAttrAction =" << (addAttrAction != nullptr)
             << "addPortAction =" << (addPortAction != nullptr)
             << "addItemAction =" << (addItemAction != nullptr)
             << "screenPos =" << event->screenPos();

    menu.exec(event->screenPos());
    event->accept();  // 标记事件已处理
}

SYSML_NAMESPACE_END
