#include "PortGraphicsItem.h"
#include "GraphicViewWidget.h"
#include "ArchitectureScene.h"
#include "PartGraphicsItem.h"
#include "PortBasicItem.h"
#include "GraphicsItemTypes.h"
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsSceneContextMenuEvent>
#include <QGraphicsSceneMouseEvent>
#include <QMenu>
#include <QAction>
#include <QDebug>

SYSML_NAMESPACE_BEGIN

PortGraphicsItem::PortGraphicsItem(Element* port, PortGraphicMetadata* metadata, QGraphicsItem* parent)
    : QGraphicsItem(parent)
    , m_port(port)
    , m_metadata(metadata)
{
    qDebug() << "[PortGraphicsItem] Creating port item:"
             << (port ? port->declaredName() : "null")
             << "parent:" << (parent ? "set" : "null")
             << "metadata:" << (metadata ? "set" : "null");
    
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    
    // 确保端口能接收鼠标事件
    setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
    
    // 设置标志，确保端口能接收鼠标事件，即使父项也在处理事件
    setFlag(QGraphicsItem::ItemIgnoresParentOpacity, false);
    setFlag(QGraphicsItem::ItemIsFocusable, true);
    
    // 根据元数据决定是否可拖动；PortGraphicMetadata 默认 portMovable = true
    bool canMove = true;
    if (m_metadata) {
        canMove = m_metadata->portMovable();
    }
    if (canMove) {
        setFlag(QGraphicsItem::ItemIsMovable, true);
    }
    
    // 设置较高的 z-order，确保端口在父 Part 之上，可以被点击
    setZValue(100);
    
    // 计算边界矩形
    updateBoundingRect();
    
    qDebug() << "[PortGraphicsItem] Created port item:"
             << (port ? port->declaredName() : "null")
             << "boundingRect:" << m_boundingRect
             << "zValue:" << zValue()
             << "flags: selectable=" << (flags() & QGraphicsItem::ItemIsSelectable)
             << "movable=" << (flags() & QGraphicsItem::ItemIsMovable);
}

QRectF PortGraphicsItem::boundingRect() const {
    return m_boundingRect;
}

QPainterPath PortGraphicsItem::shape() const {
    QPainterPath path;
    QRectF rect = boundingRect();
    
    // 获取端口形状
    QString shapeType = m_metadata ? m_metadata->portShape() : "Circle";
    
    if (shapeType == "Square") {
        path.addRect(rect);
    } else {
        // 默认圆形：使用椭圆路径，让点击检测更准确
        path.addEllipse(rect);
    }
    
    return path;
}

void PortGraphicsItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    Q_UNUSED(option)
    Q_UNUSED(widget)
    
    QRectF rect = boundingRect();
    
    // 获取端口样式
    QString shape = m_metadata ? m_metadata->portShape() : "Circle";
    QString fillColor = m_metadata ? m_metadata->portFillColor() : "#FFFFFF";
    QString borderColor = m_metadata ? m_metadata->portBorderColor() : "#000000";
    double borderWidth = m_metadata ? m_metadata->portBorderWidth() : 1.0;
    
    // 设置画笔和画刷
    QPen pen(QColor(borderColor), borderWidth);
    painter->setPen(pen);
    painter->setBrush(QBrush(QColor(fillColor)));
    
    // 绘制端口形状
    if (shape == "Square") {
        painter->drawRect(rect);
    } else {
        // 默认圆形
        painter->drawEllipse(rect);
    }
}

QPointF PortGraphicsItem::connectionPoint() const {
    // 连接点位于端口中心（相对于端口项）
    return QPointF(0, 0);
}

void PortGraphicsItem::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    qDebug() << "[PortGraphicsItem] mousePressEvent: CALLED"
             << "port:" << (m_port ? m_port->declaredName() : "null")
             << "pos:" << event->pos()
             << "scenePos:" << event->scenePos()
             << "button:" << event->button()
             << "modifiers:" << event->modifiers()
             << "flags: selectable=" << (flags() & QGraphicsItem::ItemIsSelectable)
             << "movable=" << (flags() & QGraphicsItem::ItemIsMovable)
             << "zValue:" << zValue();
    
    // 立即接受事件，阻止传播到父项或 view
    event->accept();
    
    if (event->button() == Qt::LeftButton) {
        // 记录按下位置
        m_pressPos = event->scenePos();
        // 进入连接创建模式（如果端口不可移动，或者用户按住 Ctrl 键）
        // 如果端口可移动，需要按住 Ctrl 键才能创建连接，避免与移动冲突
        bool ctrlPressed = (event->modifiers() & Qt::ControlModifier) != 0;
        bool isMovable = (flags() & QGraphicsItem::ItemIsMovable) != 0;
        
        qDebug() << "[PortGraphicsItem] mousePressEvent: Checking connection creation mode"
                 << "isMovable:" << isMovable
                 << "ctrlPressed:" << ctrlPressed
                 << "willEnterConnectionMode:" << (!isMovable || ctrlPressed);
        
        if (!isMovable || ctrlPressed) {
            m_creatingConnection = true;
            qDebug() << "[PortGraphicsItem] mousePressEvent: ✓ Entering connection creation mode"
                     << "pressPos:" << m_pressPos;
            
            // 开始连接预览
            QList<QGraphicsView*> views = scene()->views();
            for (QGraphicsView* v : views) {
                GraphicViewWidget* view = qobject_cast<GraphicViewWidget*>(v);
                if (view) {
                    view->startConnectionPreview(this, m_pressPos);
                    break;
                }
            }
            
            // 事件已在函数开头接受，直接返回
            return;
        } else {
            qDebug() << "[PortGraphicsItem] mousePressEvent: ✗ Not entering connection creation mode (port is movable and Ctrl not pressed)";
            // 对于可移动端口，如果不按 Ctrl，允许正常移动
            // 但事件已被接受，不会传播到父项
        }
    }
    
    // 如果事件未被处理，调用基类处理（但通常不会到达这里，因为事件已被接受）
    QGraphicsItem::mousePressEvent(event);
}

void PortGraphicsItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
    qDebug() << "[PortGraphicsItem] mouseMoveEvent:"
             << "port:" << (m_port ? m_port->declaredName() : "null")
             << "pos:" << event->pos()
             << "scenePos:" << event->scenePos()
             << "creatingConnection:" << m_creatingConnection;
    
    // 如果正在创建连接，保持连接创建模式，不取消
    // 实际的判断在 mouseReleaseEvent 中进行
    if (m_creatingConnection) {
        // 更新连接预览
        QList<QGraphicsView*> views = scene()->views();
        for (QGraphicsView* v : views) {
            GraphicViewWidget* view = qobject_cast<GraphicViewWidget*>(v);
            if (view) {
                view->updateConnectionPreview(event->scenePos());
                break;
            }
        }
        // 接受事件，阻止传播，保持连接创建模式
        event->accept();
        return;
    }
    
    QGraphicsItem::mouseMoveEvent(event);
}

void PortGraphicsItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
    qDebug() << "[PortGraphicsItem] mouseReleaseEvent:"
             << "port:" << (m_port ? m_port->declaredName() : "null")
             << "pos:" << event->pos()
             << "scenePos:" << event->scenePos()
             << "button:" << event->button()
             << "creatingConnection:" << m_creatingConnection;
    
    if (m_creatingConnection && event->button() == Qt::LeftButton) {
        qDebug() << "[PortGraphicsItem] mouseReleaseEvent: In connection creation mode, checking release conditions";
        
        // 检查是否移动了足够距离（如果移动了，说明用户想要移动端口而不是创建连接）
        QPointF delta = event->scenePos() - m_pressPos;
        double distance = qSqrt(delta.x() * delta.x() + delta.y() * delta.y());
        
        qDebug() << "[PortGraphicsItem] mouseReleaseEvent: Movement check"
                 << "pressPos:" << m_pressPos
                 << "releasePos:" << event->scenePos()
                 << "delta:" << delta
                 << "distance:" << distance
                 << "threshold: 10.0";
        
        // 如果移动距离很小（小于 10 像素），可能是误点击，不创建连接
        // 如果移动距离较大，说明用户想要创建连接，检查是否释放到另一个端口上
        if (distance < 10.0) {
            m_creatingConnection = false;
            qDebug() << "[PortGraphicsItem] mouseReleaseEvent: ✗ Moved too little, not creating connection"
                     << "distance:" << distance << "< threshold: 10.0";
            QGraphicsItem::mouseReleaseEvent(event);
            return;
        }
        
        qDebug() << "[PortGraphicsItem] mouseReleaseEvent: ✓ Moved enough distance, checking for target port";
        
        // 检查是否释放到另一个端口上
        // 使用 items() 查找所有在释放位置的项，然后筛选出端口项
        QList<QGraphicsItem*> itemsAtPos = scene()->items(event->scenePos());
        qDebug() << "[PortGraphicsItem] mouseReleaseEvent: Found" << itemsAtPos.size() << "items at release position";
        
        QGraphicsItem* targetItem = nullptr;
        
        // 优先查找 PortGraphicsItem 或 PortBasicItem
        // 使用 type() 方法判断类型，比 qgraphicsitem_cast 更安全
        for (QGraphicsItem* item : itemsAtPos) {
            if (!item) {
                continue;
            }
            
            // 检查 item 是否仍然在场景中（防止访问已删除的项）
            if (!item->scene()) {
                continue;
            }
            
            // 使用 type() 方法判断类型
            int itemType = item->type();
            if (itemType == GraphicsItemTypes::PortGraphicsItem && item != this) {
                targetItem = item;
                break;
            } else if (itemType == GraphicsItemTypes::PortBasicItem) {
                targetItem = item;
                break;
            }
        }
        
        if (!targetItem) {
            qDebug() << "[PortGraphicsItem] mouseReleaseEvent: ✗ No port item found at release position";
            m_creatingConnection = false;
            QGraphicsItem::mouseReleaseEvent(event);
            return;
        }
        
        // 使用 type() 方法判断类型，比 qgraphicsitem_cast 更安全
        int targetItemType = targetItem->type();
        bool isPortGraphics = (targetItemType == GraphicsItemTypes::PortGraphicsItem);
        bool isPortBasic = (targetItemType == GraphicsItemTypes::PortBasicItem);
        
        qDebug() << "[PortGraphicsItem] mouseReleaseEvent: Target port item found"
                 << "targetItemType:" << targetItemType
                 << "isPortGraphics:" << isPortGraphics
                 << "isPortBasic:" << isPortBasic
                 << "isSelf:" << (targetItem == this);
        
        // 如果找到了目标端口且不是自己（比较图形项本身，而不是端口元素，因为相同类型不同实例的同一端口可能共享同一个端口元素）
        if ((isPortGraphics && targetItem != this) || 
            (isPortBasic && targetItem != this)) {
            
            qDebug() << "[PortGraphicsItem] mouseReleaseEvent: ✓ Valid target port found, getting GraphicViewWidget";
            
            // 获取 GraphicViewWidget
            QList<QGraphicsView*> views = scene()->views();
            qDebug() << "[PortGraphicsItem] mouseReleaseEvent: Scene has" << views.size() << "views";
            
            GraphicViewWidget* view = nullptr;
            for (QGraphicsView* v : views) {
                view = qobject_cast<GraphicViewWidget*>(v);
                if (view) {
                    qDebug() << "[PortGraphicsItem] mouseReleaseEvent: ✓ Found GraphicViewWidget";
                    break;
                }
            }
            
            if (!view) {
                qDebug() << "[PortGraphicsItem] mouseReleaseEvent: ✗ GraphicViewWidget not found";
                m_creatingConnection = false;
                QGraphicsItem::mouseReleaseEvent(event);
                return;
            }
            
            // 安全地获取目标端口元素
            Element* targetPort = nullptr;
            if (isPortGraphics) {
                PortGraphicsItem* portGraphics = static_cast<PortGraphicsItem*>(targetItem);
                if (portGraphics) {
                    targetPort = portGraphics->port();
                }
            } else if (isPortBasic) {
                PortBasicItem* portBasic = static_cast<PortBasicItem*>(targetItem);
                if (portBasic) {
                    targetPort = portBasic->port();
                }
            }
            
            if (targetPort && m_port) {
                qDebug() << "[PortGraphicsItem] mouseReleaseEvent: ✓ Calling createConnection"
                         << "sourcePort:" << m_port->declaredName()
                         << "targetPort:" << targetPort->declaredName();
                // 传递图形项，以便找到正确的 PartUsage 实例
                QGraphicsItem* sourceItem = this;
                QGraphicsItem* targetPortItem = targetItem;
                Element* connection = view->createConnection(m_port, targetPort, sourceItem, targetPortItem);
                if (connection) {
                    qDebug() << "[PortGraphicsItem] mouseReleaseEvent: ✓ Connection created successfully"
                             << "connection:" << connection->declaredName();
                } else {
                    qDebug() << "[PortGraphicsItem] mouseReleaseEvent: ✗ Connection creation failed";
                }
            } else {
                qDebug() << "[PortGraphicsItem] mouseReleaseEvent: ✗ Invalid ports"
                         << "sourcePort:" << (m_port ? m_port->declaredName() : "null")
                         << "targetPort:" << (targetPort ? targetPort->declaredName() : "null");
            }
        } else {
            qDebug() << "[PortGraphicsItem] mouseReleaseEvent: ✗ Target port is self or invalid";
        }
        
        // 退出连接创建模式
        m_creatingConnection = false;
    } else {
        qDebug() << "[PortGraphicsItem] mouseReleaseEvent: Not in connection creation mode or wrong button"
                 << "creatingConnection:" << m_creatingConnection
                 << "button:" << event->button();
    }
    
    // 结束连接预览
    QList<QGraphicsView*> views = scene()->views();
    for (QGraphicsView* v : views) {
        GraphicViewWidget* view = qobject_cast<GraphicViewWidget*>(v);
        if (view) {
            view->endConnectionPreview();
            break;
        }
    }
    
    QGraphicsItem::mouseReleaseEvent(event);
}

void PortGraphicsItem::updateBoundingRect() {
    double size = m_metadata ? m_metadata->portSize() : 8.0;  // 默认大小从6.0增加到8.0，更容易点击
    // 增大边界矩形，增加点击区域（增加2像素的边距）
    m_boundingRect = QRectF(-size/2 - 2, -size/2 - 2, size + 4, size + 4);
    qDebug() << "[PortGraphicsItem] updateBoundingRect for port:"
             << (m_port ? m_port->declaredName() : "null")
             << "size:" << size
             << "boundingRect:" << m_boundingRect;
    prepareGeometryChange();
}

QVariant PortGraphicsItem::itemChange(GraphicsItemChange change, const QVariant& value) {
    if (change == ItemPositionHasChanged && m_metadata && m_port) {
        // 检查父 Part 是否正在缩放，如果是，则不保存（子元素信息应该保存在 PartDefinition 类型中，而不是 PartUsage 实例中）
        QGraphicsItem* parent = parentItem();
        if (parent) {
            PartGraphicsItem* parentPart = qgraphicsitem_cast<PartGraphicsItem*>(parent);
            if (parentPart && parentPart->isResizing()) {
                // 父 Part 正在缩放，只更新元数据用于视觉反馈，但不保存到模型
                QPointF newPos = value.toPointF();
                m_metadata->setGraphicX(newPos.x());
                m_metadata->setGraphicY(newPos.y());
                qDebug() << "[PortGraphicsItem] itemChange: Parent part is resizing, skipping save (child elements should be saved in PartDefinition, not PartUsage instance)";
                return QGraphicsItem::itemChange(change, value);
            }
        }
        
        // 位置改变时更新元数据
        // 坐标类型规则：
        // - PartDefinition 的端口：保存相对于 PartDefinition 的本地坐标
        // - PartUsage 的端口：保存相对于父 PartUsage 的本地坐标
        
        // 获取位置
        // 注意：在 itemChange 中，value 是新位置
        // - 对于有父项的项目，value 是相对于父项的本地坐标
        // - 对于没有父项的项目，value 是场景坐标
        QPointF newPos = value.toPointF();
        
        // 判断端口是否属于 PartDefinition
        // 方法：检查端口的 owner 是否是 PartDefinition
        bool isPartDefinitionPort = false;
        if (m_port) {
            Element* owner = m_port->owner();
            if (owner && owner->elementType() == "PartDefinition") {
                isPartDefinitionPort = true;
            }
        }
        
        // 如果端口属于 PartDefinition，需要将场景坐标转换为相对于 PartDefinition 的本地坐标
        if (isPartDefinitionPort) {
            // 对于 PartDefinition 的端口，如果没有父项，newPos 是场景坐标
            // 需要转换为相对于 PartDefinition 的本地坐标
            if (!parentItem()) {
                if (scene()) {
                    ArchitectureScene* archScene = qobject_cast<ArchitectureScene*>(scene());
                    if (archScene && m_port) {
                        Element* owner = m_port->owner();
                        PartGraphicsItem* partDefItem = archScene->findPartItem(owner);
                        if (partDefItem) {
                            // newPos 是场景坐标（因为没有父项），转换为相对于 PartDefinition 的本地坐标
                            QPointF partDefLocalPos = partDefItem->mapFromScene(newPos);
                            m_metadata->setGraphicX(partDefLocalPos.x());
                            m_metadata->setGraphicY(partDefLocalPos.y());
                            qDebug() << "[PortGraphicsItem] itemChange: Converting scene to local for PartDefinition port"
                                     << "scenePos:" << newPos
                                     << "partDefScenePos:" << partDefItem->scenePos()
                                     << "localPos:" << partDefLocalPos;
                        } else {
                            // 如果找不到 PartDefinition 的 PartGraphicsItem，跳过保存（可能在初始化阶段）
                            qDebug() << "[PortGraphicsItem] itemChange: PartDefinition not found, skipping save"
                                     << "scenePos:" << newPos
                                     << "owner:" << (owner ? owner->declaredName() : "null");
                            return QGraphicsItem::itemChange(change, value);
                        }
                    } else {
                        // 场景不可用，跳过保存
                        qDebug() << "[PortGraphicsItem] itemChange: Scene not available, skipping save"
                                 << "scenePos:" << newPos;
                        return QGraphicsItem::itemChange(change, value);
                    }
                } else {
                    // 场景不可用，跳过保存（可能在初始化阶段）
                    qDebug() << "[PortGraphicsItem] itemChange: PartDefinition port, no parent and no scene, skipping save"
                             << "newPos:" << newPos;
                    return QGraphicsItem::itemChange(change, value);
                }
            } else {
                // 有父项，使用本地坐标（相对于父项）- 这种情况不应该发生
                m_metadata->setGraphicX(newPos.x());
                m_metadata->setGraphicY(newPos.y());
                qDebug() << "[PortGraphicsItem] itemChange: PartDefinition port with parent (unexpected), using local coordinates"
                         << "localPos:" << newPos;
            }
        } else {
            // PartUsage 的端口，使用本地坐标（相对于父项）
            m_metadata->setGraphicX(newPos.x());
            m_metadata->setGraphicY(newPos.y());
            qDebug() << "[PortGraphicsItem] itemChange: PartUsage port, using local coordinates"
                     << "localPos:" << newPos
                     << "parent:" << (parentItem() ? "set" : "null");
        }
        
        // 保存到模型
        m_metadata->saveToModel(m_port);
        
        // 通知场景更新与该端口相连的连接线
        if (scene()) {
            ArchitectureScene* archScene = qobject_cast<ArchitectureScene*>(scene());
            if (archScene) {
                archScene->updateConnectionsForPort(this);
            }
        }
    }
    return QGraphicsItem::itemChange(change, value);
}

void PortGraphicsItem::contextMenuEvent(QGraphicsSceneContextMenuEvent* event) {
    if (!m_port) {
        QGraphicsItem::contextMenuEvent(event);
        return;
    }
    
    QMenu menu;
    QAction* deleteAction = menu.addAction("删除端口");
    
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
            view->deletePort(m_port);
        }
    });
    
    menu.exec(event->screenPos());
}

SYSML_NAMESPACE_END
