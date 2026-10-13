#include "PartGraphicsItem.h"
#include "PortGraphicsItem.h"
#include "PortBasicItem.h"
#include "GraphicViewWidget.h"
#include "ArchitectureScene.h"
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsSceneContextMenuEvent>
#include <QGraphicsSceneMouseEvent>
#include <QMenu>
#include <QAction>
#include <QFont>
#include <QFontMetrics>
#include <QCursor>
#include <QDebug>
#include <QMap>

SYSML_NAMESPACE_BEGIN

PartGraphicsItem::PartGraphicsItem(Element* part, PartGraphicMetadata* metadata, QGraphicsItem* parent)
    : QGraphicsItem(parent)
    , m_part(part)
    , m_metadata(metadata)
{
    // 选择/几何变化始终开启
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);

    // 仅允许 PartUsage（实例）拖动；PartDefinition 的外形不允许通过拖动改变位置
    bool canMove = false;
    if (m_part) {
        QString type = m_part->elementType();
        if (type == "PartUsage") {
            // 对 Part 实例，默认允许拖动；如需更细粒度控制，可后续结合 metadata->movable()
            canMove = true;
        } else if (type == "PartDefinition") {
            // PartDefinition：位置由视图控制，仅允许调整大小，不允许拖动
            canMove = false;
        }
    }
    if (canMove) {
        setFlag(QGraphicsItem::ItemIsMovable, true);
    }
    
    // 计算边界矩形
    updateBoundingRect();
    
    // 注意：调整手柄在 Part 被选中时才创建（在 itemChange 中处理）
}

PartGraphicsItem::~PartGraphicsItem() {
    // Qt会自动清理子项，但可以显式清理
    // 注意：不要删除子项，因为Qt会管理它们的生命周期
}

QRectF PartGraphicsItem::boundingRect() const {
    return m_boundingRect;
}

void PartGraphicsItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    Q_UNUSED(widget)
    
    QRectF rect = boundingRect();
    
    // 如果没有元数据，使用默认样式
    if (!m_metadata) {
        painter->setPen(QPen(Qt::black, 1));
        painter->setBrush(QBrush(Qt::white));
        painter->drawRect(rect);
        
        // 绘制part名称
        QString partName = m_part ? m_part->declaredName() : "Part";
        painter->drawText(rect, Qt::AlignCenter, partName);
        return;
    }
    
    // 根据GraphicMetadata绘制part
    QString shape = m_metadata->shape();
    QString fillColor = m_metadata->fillColor();
    QString borderColor = m_metadata->borderColor();
    double borderWidth = m_metadata->borderWidth();
    double fillOpacity = m_metadata->fillOpacity();
    
    // 设置画笔和画刷
    QPen pen(QColor(borderColor), borderWidth);
    painter->setPen(pen);
    
    QColor fillColorObj = m_metadata->fillColorAsQColor();
    fillColorObj.setAlphaF(fillOpacity);
    painter->setBrush(QBrush(fillColorObj));
    
    // 绘制形状
    QPainterPath shapePath = createShapePath(rect);
    painter->drawPath(shapePath);
    
    // 绘制标签
    if (m_metadata->labelVisible()) {
        QString labelText = m_metadata->labelText();
        if (labelText.isEmpty() && m_part) {
            labelText = m_part->declaredName();
        }
        
        if (!labelText.isEmpty()) {
            QFont font = painter->font();
            if (!m_metadata->labelFont().isEmpty()) {
                font.fromString(m_metadata->labelFont());
            }
            painter->setFont(font);
            painter->setPen(QPen(QColor(m_metadata->labelColor())));
            
            // 根据labelPosition绘制标签
            QString labelPos = m_metadata->labelPosition();
            Qt::Alignment alignment = Qt::AlignCenter;
            if (labelPos == "Top") {
                alignment = Qt::AlignTop | Qt::AlignHCenter;
            } else if (labelPos == "Bottom") {
                alignment = Qt::AlignBottom | Qt::AlignHCenter;
            } else if (labelPos == "Left") {
                alignment = Qt::AlignLeft | Qt::AlignVCenter;
            } else if (labelPos == "Right") {
                alignment = Qt::AlignRight | Qt::AlignVCenter;
            }
            
            painter->drawText(rect, alignment, labelText);
        }
    }
    
    // 绘制选中状态
    if (option->state & QStyle::State_Selected) {
        QPen selectionPen(Qt::blue, 2, Qt::DashLine);
        painter->setPen(selectionPen);
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(rect.adjusted(-2, -2, 2, 2));
    }
    
    // 调整手柄由 ResizeHandleItem 自己绘制，不需要在这里绘制
}

QVariant PartGraphicsItem::itemChange(GraphicsItemChange change, const QVariant& value) {
    // 处理选中状态变化，显示/隐藏调整手柄
    if (change == ItemSelectedHasChanged) {
        bool isSelected = value.toBool();
        // 嵌套 part 实例的子 part 不允许手动缩放，只能跟随父 part 缩放
        // 检查是否有父项：如果有父项，说明是嵌套 part 实例的子 part，不应该显示缩放手柄
        bool isNestedChildPart = (parentItem() != nullptr);
        if (isSelected && m_metadata && m_metadata->resizable() && !isNestedChildPart) {
            createResizeHandles();
        } else {
            removeResizeHandles();
        }
    }
    
    if (change == ItemPositionHasChanged && m_metadata && m_part) {
        // 如果正在缩放，则不保存（子元素信息应该保存在 PartDefinition 类型中，而不是 PartUsage 实例中）
        // 但是，如果这是 PartDefinition 本身的位置改变，也不应该保存（PartDefinition 位置固定）
        qDebug() << "[PartGraphicsItem] itemChange: ItemPositionHasChanged"
                 << "part:" << (m_part ? m_part->declaredName() : "null")
                 << "m_isResizing:" << m_isResizing;
        
        if (m_isResizing) {
            // 正在缩放，只更新元数据用于视觉反馈，但不保存到模型
            QPointF newPos = value.toPointF();
            // 判断 Part 是否属于 PartDefinition
            bool isPartDefinitionChild = false;
            if (m_part) {
                Element* owner = m_part->owner();
                if (owner && owner->elementType() == "PartDefinition") {
                    isPartDefinitionChild = true;
                }
            }
            
            if (isPartDefinitionChild && !parentItem() && scene()) {
                // PartDefinition 的子 Part，需要转换坐标
                ArchitectureScene* archScene = qobject_cast<ArchitectureScene*>(scene());
                if (archScene && m_part) {
                    Element* owner = m_part->owner();
                    PartGraphicsItem* partDefItem = archScene->findPartItem(owner);
                    if (partDefItem) {
                        QPointF partDefLocalPos = partDefItem->mapFromScene(newPos);
                        m_metadata->setGraphicX(partDefLocalPos.x());
                        m_metadata->setGraphicY(partDefLocalPos.y());
                    }
                }
            } else if (parentItem()) {
                // 有父项，使用本地坐标
                m_metadata->setGraphicX(newPos.x());
                m_metadata->setGraphicY(newPos.y());
            } else {
                // 没有父项，使用场景坐标（但这种情况不应该发生在缩放时）
                m_metadata->setGraphicX(newPos.x());
                m_metadata->setGraphicY(newPos.y());
            }
            
            qDebug() << "[PartGraphicsItem] itemChange: Part is resizing, skipping save (child elements should be saved in PartDefinition, not PartUsage instance)";
            return QGraphicsItem::itemChange(change, value);
        }
        
        // 位置改变时更新元数据
        // 坐标类型规则：
        // - PartDefinition 本身：保存场景坐标（但 PartDefinition 不允许拖动，此情况不应发生）
        // - PartDefinition 的子 Part 实例：保存相对于 PartDefinition 的本地坐标
        // - PartUsage 的子 Part 实例：保存相对于父 PartUsage 的本地坐标
        
        // 获取位置
        // 注意：在 itemChange 中，value 是新位置
        // - 对于有父项的项目，value 是相对于父项的本地坐标
        // - 对于没有父项的项目，value 是场景坐标
        QPointF newPos = value.toPointF();
        
        // 判断 Part 是否属于 PartDefinition
        // 方法：检查 Part 的 owner 是否是 PartDefinition
        bool isPartDefinitionChild = false;
        if (m_part) {
            Element* owner = m_part->owner();
            if (owner && owner->elementType() == "PartDefinition") {
                isPartDefinitionChild = true;
            }
        }
        
        // 如果 Part 属于 PartDefinition，需要将场景坐标转换为相对于 PartDefinition 的本地坐标
        if (isPartDefinitionChild) {
            // 对于 PartDefinition 的子 Part，如果没有父项，newPos 是场景坐标
            // 需要转换为相对于 PartDefinition 的本地坐标
            if (!parentItem() && scene()) {
                ArchitectureScene* archScene = qobject_cast<ArchitectureScene*>(scene());
                if (archScene && m_part) {
                    Element* owner = m_part->owner();
                    PartGraphicsItem* partDefItem = archScene->findPartItem(owner);
                    if (partDefItem) {
                        // newPos 是场景坐标（因为没有父项），转换为相对于 PartDefinition 的本地坐标
                        QPointF partDefLocalPos = partDefItem->mapFromScene(newPos);
                        m_metadata->setGraphicX(partDefLocalPos.x());
                        m_metadata->setGraphicY(partDefLocalPos.y());
                        qDebug() << "[PartGraphicsItem] itemChange: Converting scene to local for PartDefinition child"
                                 << "scenePos:" << newPos
                                 << "partDefScenePos:" << partDefItem->scenePos()
                                 << "localPos:" << partDefLocalPos;
                    } else {
                        // 如果找不到 PartDefinition 的 PartGraphicsItem，延迟处理
                        qDebug() << "[PartGraphicsItem] itemChange: PartDefinition not found, skipping save"
                                 << "scenePos:" << newPos;
                        // 不保存，直接返回
                        if (!m_isResizing) {
                            QString elementType = m_part->elementType();
                            if (elementType != "PartDefinition") {
                                // 更新连接线
                                if (scene()) {
                                    ArchitectureScene* archScene = qobject_cast<ArchitectureScene*>(scene());
                                    if (archScene) {
                                        for (PortGraphicsItem* portItem : m_ports) {
                                            if (portItem) {
                                                archScene->updateConnectionsForPort(portItem);
                                            }
                                        }
                                        const auto children = childItems();
                                        for (QGraphicsItem* child : children) {
                                            PortBasicItem* basicPort = qgraphicsitem_cast<PortBasicItem*>(child);
                                            if (basicPort) {
                                                archScene->updateConnectionsForPort(basicPort);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        return QGraphicsItem::itemChange(change, value);
                    }
                } else {
                    // 场景不可用，延迟处理
                    qDebug() << "[PartGraphicsItem] itemChange: Scene not available, skipping save"
                             << "scenePos:" << newPos;
                    // 不保存，直接返回
                    if (!m_isResizing) {
                        QString elementType = m_part->elementType();
                        if (elementType != "PartDefinition") {
                            // 更新连接线
                            if (scene()) {
                                ArchitectureScene* archScene = qobject_cast<ArchitectureScene*>(scene());
                                if (archScene) {
                                    for (PortGraphicsItem* portItem : m_ports) {
                                        if (portItem) {
                                            archScene->updateConnectionsForPort(portItem);
                                        }
                                    }
                                    const auto children = childItems();
                                    for (QGraphicsItem* child : children) {
                                        PortBasicItem* basicPort = qgraphicsitem_cast<PortBasicItem*>(child);
                                        if (basicPort) {
                                            archScene->updateConnectionsForPort(basicPort);
                                        }
                                    }
                                }
                            }
                        }
                    }
                    return QGraphicsItem::itemChange(change, value);
                }
            } else if (parentItem()) {
                // 有父项，使用本地坐标（相对于父项）
                m_metadata->setGraphicX(newPos.x());
                m_metadata->setGraphicY(newPos.y());
                qDebug() << "[PartGraphicsItem] itemChange: PartDefinition child with parent, using local coordinates"
                         << "localPos:" << newPos;
            } else {
                // 没有父项但场景不可用，延迟处理
                qDebug() << "[PartGraphicsItem] itemChange: PartDefinition child, no parent and no scene, skipping save"
                         << "newPos:" << newPos;
                // 不保存，直接返回
                if (!m_isResizing) {
                    QString elementType = m_part->elementType();
                    if (elementType != "PartDefinition") {
                        // 更新连接线
                        if (scene()) {
                            ArchitectureScene* archScene = qobject_cast<ArchitectureScene*>(scene());
                            if (archScene) {
                                for (PortGraphicsItem* portItem : m_ports) {
                                    if (portItem) {
                                        archScene->updateConnectionsForPort(portItem);
                                    }
                                }
                                const auto children = childItems();
                                for (QGraphicsItem* child : children) {
                                    PortBasicItem* basicPort = qgraphicsitem_cast<PortBasicItem*>(child);
                                    if (basicPort) {
                                        archScene->updateConnectionsForPort(basicPort);
                                    }
                                }
                            }
                        }
                    }
                }
                return QGraphicsItem::itemChange(change, value);
            }
        } else {
            // PartUsage 的子 Part，使用本地坐标（相对于父项）
            m_metadata->setGraphicX(newPos.x());
            m_metadata->setGraphicY(newPos.y());
            qDebug() << "[PartGraphicsItem] itemChange: PartUsage child, using local coordinates"
                     << "localPos:" << newPos
                     << "parent:" << (parentItem() ? "set" : "null");
        }
        
        // 只有在非调整大小状态下才自动保存（调整大小期间在 mouseReleaseEvent 中统一保存）
        // 注意：对于 PartDefinition，位置不应该通过拖动改变，所以这里不应该保存
        // 注意：对于嵌套 part 实例的子 part（有父项且父项是 PartUsage），位置信息应该保存在 PartDefinition 类型中，而不是实例中
        if (!m_isResizing) {
            QString elementType = m_part->elementType();
            if (elementType != "PartDefinition") {
                // 检查是否是嵌套 part 实例的子 part
                // 如果有父项，且父项是 PartUsage，说明这是嵌套 part 实例的子 part
                // 嵌套 part 实例的子 part 的位置信息应该保存在 PartDefinition 类型中，而不是实例中
                bool isNestedChildPart = false;
                if (parentItem()) {
                    PartGraphicsItem* parentPart = qgraphicsitem_cast<PartGraphicsItem*>(parentItem());
                    if (parentPart && parentPart->part() && parentPart->part()->elementType() == "PartUsage") {
                        isNestedChildPart = true;
                        qDebug() << "[PartGraphicsItem] itemChange: Skipping save for nested part instance's child part"
                                 << "part:" << (m_part ? m_part->declaredName() : "null")
                                 << "parent:" << (parentPart->part() ? parentPart->part()->declaredName() : "null")
                                 << "child elements should be saved in PartDefinition (type), not PartUsage (instance)";
                    }
                }
                
                if (!isNestedChildPart) {
                    // 只有非嵌套 part 实例的子 part 才在位置改变时自动保存
                    m_metadata->saveToModel(m_part);
                }
            } else {
                qDebug() << "[PartGraphicsItem] itemChange: Skipping save for PartDefinition position change";
            }
        }

        // 当 Part 移动时，其子端口（包括 PortGraphicsItem 和 PortBasicItem）的场景位置也会改变，
        // 需要更新所有相关连接线
        if (scene()) {
            ArchitectureScene* archScene = qobject_cast<ArchitectureScene*>(scene());
            if (archScene) {
                // 1) 更新所有 PortGraphicsItem 端口对应的连接线
                for (PortGraphicsItem* portItem : m_ports) {
                    if (portItem) {
                        archScene->updateConnectionsForPort(portItem);
                    }
                }

                // 2) 更新挂在该 Part 上的所有 PortBasicItem 端口对应的连接线
                const auto children = childItems();
                for (QGraphicsItem* child : children) {
                    PortBasicItem* basicPort = qgraphicsitem_cast<PortBasicItem*>(child);
                    if (basicPort) {
                        archScene->updateConnectionsForPort(basicPort);
                    }
                }
            }
        }
    }
    return QGraphicsItem::itemChange(change, value);
}

QPainterPath PartGraphicsItem::createShapePath(const QRectF& rect) const {
    QPainterPath path;
    
    if (!m_metadata) {
        path.addRect(rect);
        return path;
    }
    
    QString shape = m_metadata->shape();
    double radius = m_metadata->shapeRadius();
    
    if (shape == "RoundedRectangle") {
        path.addRoundedRect(rect, radius, radius);
    } else if (shape == "Ellipse") {
        path.addEllipse(rect);
    } else {
        // 默认矩形
        path.addRect(rect);
    }
    
    return path;
}

void PartGraphicsItem::addChildPart(PartGraphicsItem* childPart) {
    if (childPart && !m_childParts.contains(childPart)) {
        // 检查是否是 PartDefinition，如果是，不应该添加子项（子项应该直接添加到场景）
        bool isPartDefinition = false;
        if (m_part) {
            isPartDefinition = (m_part->elementType() == "PartDefinition");
        }
        
        if (isPartDefinition) {
            qDebug() << "[PartGraphicsItem] addChildPart: Skipping for PartDefinition - child should be added to scene directly";
            return;
        }
        
        m_childParts.append(childPart);
        childPart->setParentItem(this);  // 设置为子项（实现嵌套）
        updateBoundingRect();  // 更新边界矩形
    }
}

void PartGraphicsItem::removeChildPart(PartGraphicsItem* childPart) {
    if (m_childParts.removeOne(childPart)) {
        updateBoundingRect();
    }
}

void PartGraphicsItem::addPort(PortGraphicsItem* portItem) {
    if (portItem && !m_ports.contains(portItem)) {
        // 检查是否是 PartDefinition，如果是，不应该添加端口（端口应该直接添加到场景）
        bool isPartDefinition = false;
        if (m_part) {
            isPartDefinition = (m_part->elementType() == "PartDefinition");
        }
        
        if (isPartDefinition) {
            qDebug() << "[PartGraphicsItem] addPort: Skipping for PartDefinition - port should be added to scene directly";
            return;
        }
        
        m_ports.append(portItem);
        portItem->setParentItem(this);  // 设置为子项
        updateBoundingRect();  // 更新边界矩形
    }
}

void PartGraphicsItem::removePort(PortGraphicsItem* portItem) {
    if (m_ports.removeOne(portItem)) {
        updateBoundingRect();
    }
}

void PartGraphicsItem::updateBoundingRect() {
    // 计算包含所有子项和端口的边界矩形
    QRectF rect = QRectF(0, 0, 100, 80);  // 默认大小
    
    if (m_metadata) {
        if (m_metadata->graphicWidth() > 0) {
            rect.setWidth(m_metadata->graphicWidth());
        }
        if (m_metadata->graphicHeight() > 0) {
            rect.setHeight(m_metadata->graphicHeight());
        }
    }
    
    // 包含所有子项
    for (PartGraphicsItem* child : m_childParts) {
        QRectF childRect = child->boundingRect();
        QPointF childPos = child->pos();
        rect = rect.united(QRectF(childPos, childRect.size()));
    }
    
    // 包含所有端口
    for (PortGraphicsItem* port : m_ports) {
        QRectF portRect = port->boundingRect();
        QPointF portPos = port->pos();
        rect = rect.united(QRectF(portPos, portRect.size()));
    }
    
    m_boundingRect = rect;
    prepareGeometryChange();  // 通知Qt边界矩形已改变
    
    // 更新调整手柄位置
    if (!m_resizeHandles.isEmpty()) {
        updateResizeHandlesPosition();
    }
}

void PartGraphicsItem::updateGraphics() {
    updateBoundingRect();
    update();  // 触发重绘
}

void PartGraphicsItem::contextMenuEvent(QGraphicsSceneContextMenuEvent* event) {
    if (!m_part || m_part->elementType() != "PartUsage") {
        // 只允许删除 PartUsage，不允许删除 PartDefinition
        QGraphicsItem::contextMenuEvent(event);
        return;
    }
    
    QMenu menu;
    QAction* deleteAction = menu.addAction("删除 Part 实例");
    
    QObject::connect(deleteAction, &QAction::triggered, [this]() {
        // 通过 GraphicViewWidget 删除
        QGraphicsScene* scene = this->scene();
        if (!scene) {
            return;
        }
        
        QList<QGraphicsView*> views = scene->views();
        if (views.isEmpty()) {
            return;
        }
        
        GraphicViewWidget* view = qobject_cast<GraphicViewWidget*>(views.first());
        if (view) {
            view->deletePart(m_part);
        }
    });
    
    menu.exec(event->screenPos());
}

// ============================================================================
// ResizeHandleItem 内部类定义（必须在 createResizeHandles 之前定义）
// ============================================================================

class PartGraphicsItem::ResizeHandleItem : public QGraphicsItem {
public:
    enum HandlePosition {
        TopLeft,
        TopRight,
        BottomLeft,
        BottomRight
    };
    
    explicit ResizeHandleItem(HandlePosition pos, PartGraphicsItem* parent)
        : QGraphicsItem(parent)
        , m_position(pos)
        , m_parentPart(parent)
        , m_startPos(0, 0)
        , m_startSize(0, 0)
    {
        setFlag(QGraphicsItem::ItemIsSelectable, false);
        setFlag(QGraphicsItem::ItemIsMovable, false);
        setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
        setZValue(200);  // 确保在手柄之上
        
        // 设置光标
        updateCursor();
        
        // 设置边界矩形
        const double size = 8.0;
        m_boundingRect = QRectF(-size/2, -size/2, size, size);
    }
    
    HandlePosition position() const { return m_position; }
    
    QRectF boundingRect() const override {
        return m_boundingRect;
    }
    
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override {
        Q_UNUSED(option)
        Q_UNUSED(widget)
        
        // 绘制小矩形手柄
        painter->setPen(QPen(Qt::black, 1));
        painter->setBrush(QBrush(Qt::white));
        painter->drawRect(m_boundingRect);
        
        // 绘制内部填充
        painter->setBrush(QBrush(Qt::gray));
        painter->drawRect(m_boundingRect.adjusted(1, 1, -1, -1));
    }
    
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override {
        if (event->button() == Qt::LeftButton && m_parentPart && m_parentPart->metadata()) {
            // 标记开始调整大小
            m_parentPart->m_isResizing = true;
            
            m_startPos = event->scenePos();
            m_startSize = QSizeF(m_parentPart->boundingRect().width(), m_parentPart->boundingRect().height());
            m_startPartPos = m_parentPart->scenePos();
            
            // 保存所有子项的初始位置（用于缩放时跟随）
            m_startChildPositions.clear();
            
            // 保存所有 PortGraphicsItem 的位置
            for (PortGraphicsItem* portItem : m_parentPart->ports()) {
                if (portItem && portItem->metadata()) {
                    QPointF portPos = portItem->pos();
                    m_startChildPositions[portItem] = portPos;
                }
            }
            
            // 保存所有 PortBasicItem 的位置
            // 注意：需要排除 ResizeHandleItem（zValue=200）
            const auto children = m_parentPart->childItems();
            // 获取 ResizeHandleItem 列表，用于排除
            QList<QGraphicsItem*> resizeHandles;
            for (ResizeHandleItem* handle : m_parentPart->m_resizeHandles) {
                if (handle) {
                    resizeHandles.append(handle);
                }
            }
            
            for (QGraphicsItem* child : children) {
                // 跳过 ResizeHandleItem
                if (resizeHandles.contains(child)) {
                    continue;
                }
                
                PortBasicItem* basicPort = qgraphicsitem_cast<PortBasicItem*>(child);
                if (basicPort) {
                    QPointF portPos = basicPort->pos();
                    m_startChildPositions[basicPort] = portPos;
                }
            }
            
            // 保存所有子 PartGraphicsItem 的位置和大小
            for (PartGraphicsItem* childPart : m_parentPart->childParts()) {
                if (childPart && childPart->metadata()) {
                    QPointF childPos = childPart->pos();
                    QRectF childRect = childPart->boundingRect();
                    m_startChildPositions[childPart] = childPos;
                    m_startChildSizes[childPart] = QSizeF(childRect.width(), childRect.height());
                    qDebug() << "[ResizeHandleItem] mousePressEvent: Saved child Part initial state"
                             << "childPart:" << (childPart->part() ? childPart->part()->declaredName() : "null")
                             << "childPos:" << childPos
                             << "childRect:" << childRect
                             << "parentPart:" << (m_parentPart->part() ? m_parentPart->part()->declaredName() : "null")
                             << "parentPos:" << m_parentPart->pos()
                             << "parentRect:" << m_parentPart->boundingRect();
                }
            }
            
            event->accept();
        } else {
            QGraphicsItem::mousePressEvent(event);
        }
    }
    
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override {
        if (event->buttons() & Qt::LeftButton && m_parentPart && m_parentPart->metadata()) {
            QPointF currentPos = event->scenePos();
            QPointF delta = currentPos - m_startPos;
            
            QRectF partRect = m_parentPart->boundingRect();
            QPointF partScenePos = m_parentPart->scenePos();
            
            double newWidth = m_startSize.width();
            double newHeight = m_startSize.height();
            QPointF newPartPos = m_startPartPos;
            
            // 根据手柄位置计算新的宽高和位置
            switch (m_position) {
                case TopLeft:
                    newWidth = m_startSize.width() - delta.x();
                    newHeight = m_startSize.height() - delta.y();
                    newPartPos = m_startPartPos + delta;
                    break;
                case TopRight:
                    newWidth = m_startSize.width() + delta.x();
                    newHeight = m_startSize.height() - delta.y();
                    newPartPos.setY(m_startPartPos.y() + delta.y());
                    break;
                case BottomLeft:
                    newWidth = m_startSize.width() - delta.x();
                    newHeight = m_startSize.height() + delta.y();
                    newPartPos.setX(m_startPartPos.x() + delta.x());
                    break;
                case BottomRight:
                    newWidth = m_startSize.width() + delta.x();
                    newHeight = m_startSize.height() + delta.y();
                    break;
            }
            
            // 最小尺寸约束
            const double minWidth = 40.0;
            const double minHeight = 30.0;
            if (newWidth < minWidth) {
                newWidth = minWidth;
                if (m_position == TopLeft || m_position == BottomLeft) {
                    newPartPos.setX(m_startPartPos.x() + m_startSize.width() - minWidth);
                }
            }
            if (newHeight < minHeight) {
                newHeight = minHeight;
                if (m_position == TopLeft || m_position == TopRight) {
                    newPartPos.setY(m_startPartPos.y() + m_startSize.height() - minHeight);
                }
            }
            
            // 检查是否是 PartDefinition（画布模式，不缩放子项）
            bool isPartDefinition = false;
            if (m_parentPart->part()) {
                QString partType = m_parentPart->part()->elementType();
                isPartDefinition = (partType == "PartDefinition");
            }
            
            // 在更新父 Part 位置之前，先设置所有子 Part 的 m_isResizing 标志
            // 这样可以防止父 Part 位置改变时触发子 Part 的 itemChange 保存
            if (!isPartDefinition) {
                for (PartGraphicsItem* childPart : m_parentPart->childParts()) {
                    if (childPart) {
                        childPart->m_isResizing = true;
                    }
                }
            }
            
            // 更新元数据
            m_parentPart->metadata()->setGraphicWidth(newWidth);
            m_parentPart->metadata()->setGraphicHeight(newHeight);
            
            // 更新 Part 位置（如果是左上角或左边的手柄）
            // 注意：如果 Part 有父项，需要将场景坐标转换为相对于父项的本地坐标
            // 对于 PartDefinition 的子 Part，需要转换为相对于 PartDefinition 的本地坐标
            QPointF finalPartPos;
            
            // 检查是否是 PartDefinition 的子 Part（没有父项但属于 PartDefinition）
            bool isPartDefinitionChild = false;
            PartGraphicsItem* partDefItem = nullptr;
            if (!m_parentPart->parentItem() && m_parentPart->part() && m_parentPart->scene()) {
                Element* owner = m_parentPart->part()->owner();
                if (owner && owner->elementType() == "PartDefinition") {
                    ArchitectureScene* archScene = qobject_cast<ArchitectureScene*>(m_parentPart->scene());
                    if (archScene) {
                        partDefItem = archScene->findPartItem(owner);
                        if (partDefItem) {
                            isPartDefinitionChild = true;
                        }
                    }
                }
            }
            
            if (m_position == TopLeft || m_position == BottomLeft) {
                QGraphicsItem* parent = m_parentPart->parentItem();
                if (parent) {
                    // 有父项（PartUsage 的子 Part），转换为相对于父项的本地坐标
                    QPointF localPos = parent->mapFromScene(newPartPos);
                    m_parentPart->setPos(localPos);
                    finalPartPos = localPos;
                } else if (isPartDefinitionChild && partDefItem) {
                    // PartDefinition 的子 Part，转换为相对于 PartDefinition 的本地坐标
                    QPointF localPos = partDefItem->mapFromScene(newPartPos);
                    m_parentPart->setPos(newPartPos);  // 设置场景坐标
                    finalPartPos = localPos;  // 保存本地坐标
                } else {
                    // 没有父项且不属于 PartDefinition，使用场景坐标
                    m_parentPart->setPos(newPartPos);
                    finalPartPos = newPartPos;
                }
            } else if (m_position == TopRight) {
                QGraphicsItem* parent = m_parentPart->parentItem();
                if (parent) {
                    // 有父项（PartUsage 的子 Part），转换为相对于父项的本地坐标
                    QPointF currentLocalPos = m_parentPart->pos();
                    QPointF newLocalPos = parent->mapFromScene(newPartPos);
                    m_parentPart->setPos(currentLocalPos.x(), newLocalPos.y());
                    finalPartPos = QPointF(currentLocalPos.x(), newLocalPos.y());
                } else if (isPartDefinitionChild && partDefItem) {
                    // PartDefinition 的子 Part，转换为相对于 PartDefinition 的本地坐标
                    QPointF currentLocalPos = m_parentPart->pos();
                    QPointF newLocalPos = partDefItem->mapFromScene(newPartPos);
                    m_parentPart->setPos(m_parentPart->pos().x(), newPartPos.y());  // 设置场景坐标（只更新 Y）
                    finalPartPos = QPointF(currentLocalPos.x(), newLocalPos.y());  // 保存本地坐标
                } else {
                    // 没有父项且不属于 PartDefinition，使用场景坐标
                    QPointF currentPos = m_parentPart->pos();
                    m_parentPart->setPos(currentPos.x(), newPartPos.y());
                    finalPartPos = QPointF(currentPos.x(), newPartPos.y());
                }
            } else {
                // BottomRight: 位置不变，使用当前位置
                finalPartPos = m_parentPart->pos();
                
                // 如果是 PartDefinition 的子 Part，需要将当前位置转换为本地坐标
                if (isPartDefinitionChild && partDefItem) {
                    QPointF currentScenePos = m_parentPart->scenePos();
                    finalPartPos = partDefItem->mapFromScene(currentScenePos);
                }
            }
            
            // 更新位置元数据
            // 注意：对于 PartDefinition，位置应该固定为 (0,0)，不随调整大小而改变
            // 因为 PartDefinition 的外边框相当于画布，其位置是固定的
            if (!isPartDefinition) {
                // 只有 PartUsage 才保存位置
                m_parentPart->metadata()->setGraphicX(finalPartPos.x());
                m_parentPart->metadata()->setGraphicY(finalPartPos.y());
                qDebug() << "[ResizeHandleItem] mouseMoveEvent: Saving PartUsage position"
                         << "finalPartPos:" << finalPartPos
                         << "isPartDefinitionChild:" << isPartDefinitionChild
                         << "scenePos:" << m_parentPart->scenePos();
            } else {
                // PartDefinition 的位置固定为 (0,0)，不保存实际场景位置
                m_parentPart->metadata()->setGraphicX(0.0);
                m_parentPart->metadata()->setGraphicY(0.0);
            }
            
            // 获取子项列表（用于后续更新连接线）
            const auto children = m_parentPart->childItems();
            
            // 只有 PartUsage（实例）才缩放子项；PartDefinition 的外边框相当于画布，不缩放内容
            // 对于 PartDefinition，无论使用哪个角，都不缩放子项
            if (!isPartDefinition) {
                // 计算缩放比例（相对于初始大小）
                double scaleX = m_startSize.width() > 0 ? newWidth / m_startSize.width() : 1.0;
                double scaleY = m_startSize.height() > 0 ? newHeight / m_startSize.height() : 1.0;
                
                qDebug() << "[ResizeHandleItem] mouseMoveEvent: Scaling children"
                         << "parentPart:" << (m_parentPart->part() ? m_parentPart->part()->declaredName() : "null")
                         << "startSize:" << m_startSize
                         << "newSize:" << QSizeF(newWidth, newHeight)
                         << "scaleX:" << scaleX << "scaleY:" << scaleY
                         << "childParts count:" << m_parentPart->childParts().size()
                         << "m_startChildPositions size:" << m_startChildPositions.size();
                
                // 缩放所有子项的位置
                // 注意：子元素的位置是相对于父 Part 的本地坐标，所以缩放计算应该基于初始位置
                // 1. 缩放 PortGraphicsItem 的位置（父 part 的直接端口）
                for (PortGraphicsItem* portItem : m_parentPart->ports()) {
                    if (portItem && portItem->metadata() && m_startChildPositions.contains(portItem)) {
                        QPointF startPos = m_startChildPositions[portItem];
                        QPointF newPos(startPos.x() * scaleX, startPos.y() * scaleY);
                        portItem->setPos(newPos);
                        // 更新元数据
                        portItem->metadata()->setGraphicX(newPos.x());
                        portItem->metadata()->setGraphicY(newPos.y());
                    }
                }
                
                // 2. 缩放 PortBasicItem 的位置（只缩放父 part 的直接 PortBasicItem，不包括子 part 的 PortBasicItem）
                // 子 part 的 PortBasicItem 会随着子 part 的缩放自动跟随（Qt 的父子关系），不需要手动处理
                QList<PartGraphicsItem*> childPartsList = m_parentPart->childParts();
                for (QGraphicsItem* child : children) {
                    PortBasicItem* basicPort = qgraphicsitem_cast<PortBasicItem*>(child);
                    if (basicPort && m_startChildPositions.contains(basicPort)) {
                        // 检查这个 PortBasicItem 是否是父 part 的直接子项（不是子 part 的子项）
                        bool isDirectChild = true;
                        for (PartGraphicsItem* childPart : childPartsList) {
                            if (childPart && childPart->isAncestorOf(basicPort)) {
                                // 这个 PortBasicItem 是子 part 的子项，不需要手动缩放
                                isDirectChild = false;
                                break;
                            }
                        }
                        
                        if (isDirectChild) {
                            // 只缩放父 part 的直接 PortBasicItem
                            QPointF startPos = m_startChildPositions[basicPort];
                            QPointF newPos(startPos.x() * scaleX, startPos.y() * scaleY);
                            basicPort->setPos(newPos);
                            // PortBasicItem 可能没有独立的 metadata，但位置会通过 setPos 更新
                            qDebug() << "[ResizeHandleItem] mouseMoveEvent: Scaled parent part's PortBasicItem"
                                     << "port:" << (basicPort->port() ? basicPort->port()->declaredName() : "null")
                                     << "startPos:" << startPos
                                     << "newPos:" << newPos
                                     << "scaleX:" << scaleX << "scaleY:" << scaleY;
                        } else {
                            qDebug() << "[ResizeHandleItem] mouseMoveEvent: Skipping child part's PortBasicItem (will follow child part automatically)"
                                     << "port:" << (basicPort->port() ? basicPort->port()->declaredName() : "null");
                        }
                    }
                }
                
                // 3. 缩放子 PartGraphicsItem 的位置和大小
                for (PartGraphicsItem* childPart : m_parentPart->childParts()) {
                    if (childPart && childPart->metadata()) {
                        // 临时设置子 Part 的 m_isResizing 标志，防止在缩放过程中触发 itemChange 保存
                        bool wasResizing = childPart->m_isResizing;
                        childPart->m_isResizing = true;
                        qDebug() << "[ResizeHandleItem] mouseMoveEvent: Set childPart m_isResizing = true"
                                 << "childPart:" << (childPart->part() ? childPart->part()->declaredName() : "null")
                                 << "wasResizing:" << wasResizing;
                        
                        if (m_startChildPositions.contains(childPart)) {
                            QPointF startPos = m_startChildPositions[childPart];
                            // 计算新的位置：基于初始位置和缩放比例
                            QPointF newPos(startPos.x() * scaleX, startPos.y() * scaleY);
                            
                            // 获取当前实际位置，用于调试
                            QPointF currentPos = childPart->pos();
                            
                            qDebug() << "[ResizeHandleItem] mouseMoveEvent: About to setPos for childPart"
                                     << "childPart:" << (childPart->part() ? childPart->part()->declaredName() : "null")
                                     << "m_isResizing:" << childPart->m_isResizing
                                     << "startPos:" << startPos
                                     << "currentPos:" << currentPos
                                     << "newPos:" << newPos
                                     << "scaleX:" << scaleX << "scaleY:" << scaleY;
                            
                            childPart->setPos(newPos);
                            // 更新元数据（但不保存，在 mouseReleaseEvent 中统一保存）
                            childPart->metadata()->setGraphicX(newPos.x());
                            childPart->metadata()->setGraphicY(newPos.y());
                            qDebug() << "[ResizeHandleItem] mouseMoveEvent: Scaled child Part"
                                     << "childPart:" << (childPart->part() ? childPart->part()->declaredName() : "null")
                                     << "startPos:" << startPos
                                     << "newPos:" << newPos
                                     << "scaleX:" << scaleX << "scaleY:" << scaleY;
                        }
                        
                        if (m_startChildSizes.contains(childPart)) {
                            QSizeF startSize = m_startChildSizes[childPart];
                            double newChildWidth = startSize.width() * scaleX;
                            double newChildHeight = startSize.height() * scaleY;
                            childPart->metadata()->setGraphicWidth(newChildWidth);
                            childPart->metadata()->setGraphicHeight(newChildHeight);
                            childPart->updateBoundingRect();
                            qDebug() << "[ResizeHandleItem] mouseMoveEvent: Scaled child Part size"
                                     << "childPart:" << (childPart->part() ? childPart->part()->declaredName() : "null")
                                     << "startSize:" << startSize
                                     << "newSize:" << QSizeF(newChildWidth, newChildHeight)
                                     << "scaleX:" << scaleX << "scaleY:" << scaleY;
                        }
                        
                        // 恢复子 Part 的 m_isResizing 标志
                        childPart->m_isResizing = wasResizing;
                        qDebug() << "[ResizeHandleItem] mouseMoveEvent: Restored childPart m_isResizing = " << wasResizing
                                 << "childPart:" << (childPart->part() ? childPart->part()->declaredName() : "null");
                        
                        // 更新子 part 的 PortBasicItem 的连接线（因为子 part 的位置和大小改变了）
                        if (childPart->scene()) {
                            ArchitectureScene* archScene = qobject_cast<ArchitectureScene*>(childPart->scene());
                            if (archScene) {
                                const auto childPartChildren = childPart->childItems();
                                for (QGraphicsItem* childPartChild : childPartChildren) {
                                    PortBasicItem* childPort = qgraphicsitem_cast<PortBasicItem*>(childPartChild);
                                    if (childPort) {
                                        archScene->updateConnectionsForPort(childPort);
                                    }
                                }
                            }
                        }
                    }
                }
                
                // 恢复所有子 Part 的 m_isResizing 标志（在更新父 Part 位置之前设置的）
                for (PartGraphicsItem* childPart : m_parentPart->childParts()) {
                    if (childPart) {
                        childPart->m_isResizing = false;
                    }
                }
            } else {
                // 对于 PartDefinition，其内部的 PartUsage 和 Port 已经直接添加到场景（不是子项），
                // 所以不需要任何补偿操作
            }
            
            // 更新 Part 的边界矩形
            m_parentPart->updateBoundingRect();
            
            // 更新手柄位置
            m_parentPart->updateResizeHandlesPosition();
            
            // 更新连接线（当端口位置改变时）
            if (m_parentPart->scene()) {
                ArchitectureScene* archScene = qobject_cast<ArchitectureScene*>(m_parentPart->scene());
                if (archScene) {
                    // 更新所有端口的连接线
                    for (PortGraphicsItem* portItem : m_parentPart->ports()) {
                        if (portItem) {
                            archScene->updateConnectionsForPort(portItem);
                        }
                    }
                    // 更新所有 PortBasicItem 的连接线
                    for (QGraphicsItem* child : children) {
                        PortBasicItem* basicPort = qgraphicsitem_cast<PortBasicItem*>(child);
                        if (basicPort) {
                            archScene->updateConnectionsForPort(basicPort);
                        }
                    }
                }
            }
            
            event->accept();
        } else {
            QGraphicsItem::mouseMoveEvent(event);
        }
    }
    
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override {
        qDebug() << "[ResizeHandleItem] mouseReleaseEvent: START";
        qDebug() << "[ResizeHandleItem] event->button():" << event->button();
        qDebug() << "[ResizeHandleItem] m_parentPart:" << (m_parentPart ? "valid" : "null");
        
        if (event->button() == Qt::LeftButton) {
            qDebug() << "[ResizeHandleItem] Left button pressed";
            
            if (!m_parentPart) {
                qDebug() << "[ResizeHandleItem] ERROR: m_parentPart is null";
                QGraphicsItem::mouseReleaseEvent(event);
                return;
            }
            qDebug() << "[ResizeHandleItem] m_parentPart is valid";
            
            if (!m_parentPart->metadata()) {
                qDebug() << "[ResizeHandleItem] ERROR: m_parentPart->metadata() is null";
                QGraphicsItem::mouseReleaseEvent(event);
                return;
            }
            qDebug() << "[ResizeHandleItem] m_parentPart->metadata() is valid";
            
            if (!m_parentPart->part()) {
                qDebug() << "[ResizeHandleItem] ERROR: m_parentPart->part() is null";
                QGraphicsItem::mouseReleaseEvent(event);
                return;
            }
            qDebug() << "[ResizeHandleItem] m_parentPart->part() is valid";
            
            // 检查是否是 PartDefinition（画布模式，不保存子项）
            bool isPartDefinition = false;
            Element* partElement = m_parentPart->part();
            if (partElement) {
                QString partType = partElement->elementType();
                isPartDefinition = (partType == "PartDefinition");
                qDebug() << "[ResizeHandleItem] Part type:" << partType << "isPartDefinition:" << isPartDefinition;
            }
            
            // 拖动结束后保存到模型
            qDebug() << "[ResizeHandleItem] Starting to save metadata...";
            
            // 1. 保存 Part 的元数据（包括 Part def）
            qDebug() << "[ResizeHandleItem] Step 1: Saving Part metadata";
            PartGraphicMetadata* partMeta = m_parentPart->metadata();
            qDebug() << "[ResizeHandleItem] partElement:" << (partElement ? "valid" : "null");
            qDebug() << "[ResizeHandleItem] partMeta:" << (partMeta ? "valid" : "null");
            
            if (partElement && partMeta) {
                // 在保存前，再次确认 metadata 的值
                // 对于 PartDefinition 的子 Part，需要确保保存的是本地坐标
                bool isPartDefinitionChild = false;
                if (!isPartDefinition && !m_parentPart->parentItem() && m_parentPart->part()) {
                    Element* owner = m_parentPart->part()->owner();
                    if (owner && owner->elementType() == "PartDefinition") {
                        isPartDefinitionChild = true;
                        // 如果当前保存的是场景坐标，需要转换为本地坐标
                        if (m_parentPart->scene()) {
                            ArchitectureScene* archScene = qobject_cast<ArchitectureScene*>(m_parentPart->scene());
                            if (archScene) {
                                PartGraphicsItem* partDefItem = archScene->findPartItem(owner);
                                if (partDefItem) {
                                    QPointF currentScenePos = m_parentPart->scenePos();
                                    QPointF localPos = partDefItem->mapFromScene(currentScenePos);
                                    partMeta->setGraphicX(localPos.x());
                                    partMeta->setGraphicY(localPos.y());
                                    qDebug() << "[ResizeHandleItem] mouseReleaseEvent: Converted PartDefinition child position"
                                             << "scenePos:" << currentScenePos
                                             << "localPos:" << localPos;
                                }
                            }
                        }
                    }
                }
                
                qDebug() << "[ResizeHandleItem] Before save - metadata width:" << partMeta->graphicWidth() << "height:" << partMeta->graphicHeight();
                qDebug() << "[ResizeHandleItem] Before save - metadata x:" << partMeta->graphicX() << "y:" << partMeta->graphicY();
                qDebug() << "[ResizeHandleItem] Before save - part pos:" << m_parentPart->pos() << "scenePos:" << m_parentPart->scenePos();
                qDebug() << "[ResizeHandleItem] Before save - isPartDefinitionChild:" << isPartDefinitionChild;
                qDebug() << "[ResizeHandleItem] Calling partMeta->saveToModel(partElement)";
                partMeta->saveToModel(partElement);
                qDebug() << "[ResizeHandleItem] partMeta->saveToModel() completed";
            } else {
                qDebug() << "[ResizeHandleItem] Skipping Part save: partElement or partMeta is null";
            }
            
            // 注意：缩放 PartUsage 实例时，只保存该实例本身的信息（大小和位置）
            // 不保存子元素（Port、子 Part）的信息，子元素的信息应该保存在 PartDefinition（类型）中
            // 在 mouseMoveEvent 中，子元素的位置和大小会被缩放（用于视觉反馈），但不会保存到模型
            qDebug() << "[ResizeHandleItem] Skipping child metadata save - child elements should be saved in PartDefinition (type), not PartUsage (instance)";
            
            // 清理临时数据
            qDebug() << "[ResizeHandleItem] Clearing temporary data";
            m_startChildPositions.clear();
            m_startChildSizes.clear();
            qDebug() << "[ResizeHandleItem] Temporary data cleared";
            
            // 标记调整大小结束
            if (m_parentPart) {
                m_parentPart->m_isResizing = false;
            }
            
            qDebug() << "[ResizeHandleItem] mouseReleaseEvent: END - SUCCESS";
            event->accept();
        } else {
            qDebug() << "[ResizeHandleItem] Not left button, calling base class";
            QGraphicsItem::mouseReleaseEvent(event);
        }
    }
    
private:
    void updateCursor() {
        switch (m_position) {
            case TopLeft:
            case BottomRight:
                setCursor(Qt::SizeFDiagCursor);
                break;
            case TopRight:
            case BottomLeft:
                setCursor(Qt::SizeBDiagCursor);
                break;
        }
    }
    
    HandlePosition m_position;
    PartGraphicsItem* m_parentPart;
    QRectF m_boundingRect;
    QPointF m_startPos;
    QSizeF m_startSize;
    QPointF m_startPartPos;
    QMap<QGraphicsItem*, QPointF> m_startChildPositions;  // 子项的初始位置（本地坐标）
    QMap<PartGraphicsItem*, QSizeF> m_startChildSizes;    // 子 Part 的初始大小
};

void PartGraphicsItem::createResizeHandles() {
    // 先清理现有的手柄
    removeResizeHandles();
    
    if (!m_metadata || !m_metadata->resizable()) {
        return;
    }
    
    // 嵌套 part 实例的子 part 不允许手动缩放，只能跟随父 part 缩放
    // 检查是否有父项：如果有父项，说明是嵌套 part 实例的子 part，不应该显示缩放手柄
    if (parentItem() != nullptr) {
        qDebug() << "[PartGraphicsItem] createResizeHandles: Skipping resize handles for nested child part"
                 << "part:" << (m_part ? m_part->declaredName() : "null");
        return;
    }
    
    // 创建四个角的手柄
    m_resizeHandles.append(new ResizeHandleItem(ResizeHandleItem::TopLeft, this));
    m_resizeHandles.append(new ResizeHandleItem(ResizeHandleItem::TopRight, this));
    m_resizeHandles.append(new ResizeHandleItem(ResizeHandleItem::BottomLeft, this));
    m_resizeHandles.append(new ResizeHandleItem(ResizeHandleItem::BottomRight, this));
    
    // 更新手柄位置
    updateResizeHandlesPosition();
}

void PartGraphicsItem::removeResizeHandles() {
    for (ResizeHandleItem* handle : m_resizeHandles) {
        if (handle) {
            delete handle;
        }
    }
    m_resizeHandles.clear();
}

void PartGraphicsItem::updateResizeHandlesPosition() {
    QRectF rect = boundingRect();
    const double handleSize = 8.0;
    const double offset = handleSize / 2.0;
    
    for (ResizeHandleItem* handle : m_resizeHandles) {
        if (!handle) continue;
        
        QPointF pos;
        switch (handle->position()) {
            case ResizeHandleItem::TopLeft:
                pos = QPointF(-offset, -offset);
                break;
            case ResizeHandleItem::TopRight:
                pos = QPointF(rect.width() - offset, -offset);
                break;
            case ResizeHandleItem::BottomLeft:
                pos = QPointF(-offset, rect.height() - offset);
                break;
            case ResizeHandleItem::BottomRight:
                pos = QPointF(rect.width() - offset, rect.height() - offset);
                break;
        }
        handle->setPos(pos);
    }
}

SYSML_NAMESPACE_END
