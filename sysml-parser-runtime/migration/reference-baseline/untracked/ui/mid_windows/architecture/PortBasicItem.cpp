#include "PortBasicItem.h"
#include "PortLabelItem.h"
#include "BasicViewWidget.h"
#include "GraphicViewWidget.h"
#include "PortGraphicsItem.h"
#include "GraphicsItemTypes.h"
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsSceneContextMenuEvent>
#include <QGraphicsSceneMouseEvent>
#include <QKeyEvent>
#include <QMenu>
#include <QMessageBox>
#include <QObject>
#include <QtMath>
#include <QDebug>

SYSML_NAMESPACE_BEGIN

PortBasicItem::PortBasicItem(Element* port, PortGraphicMetadata* metadata, QGraphicsItem* parent)
    : QGraphicsItem(parent)
    , m_port(port)
    , m_metadata(metadata)
{
    // 设置端口大小（根据元数据或默认值）
    double portSize = m_metadata ? m_metadata->portSize() : PORT_SIZE;
    m_boundingRect = QRectF(-portSize/2, -portSize/2, portSize, portSize);
    
    // 设置标志
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsFocusable, true);
    
    // 确保端口能接收鼠标事件
    setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
    
    // 设置标志，确保端口能接收鼠标事件，即使父项也在处理事件
    setFlag(QGraphicsItem::ItemIgnoresParentOpacity, false);
    
    // 基础 part 的端口通常不可移动，所以不设置 ItemIsMovable
    // 这样可以支持连接创建
    
    // 设置较高的 z-order，确保端口在父 Part 之上，可以被点击
    setZValue(100);
    
    // 设置标签
    setupLabel();
}

PortBasicItem::~PortBasicItem() {
    if (m_labelItem) {
        delete m_labelItem;
    }
}

QRectF PortBasicItem::boundingRect() const {
    return m_boundingRect;
}

void PortBasicItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    Q_UNUSED(option)
    Q_UNUSED(widget)
    
    // 检查端口是否可见
    if (m_metadata && !m_metadata->portVisible()) {
        return;
    }
    
    // 根据SysML规范绘制端口
    QRectF rect = boundingRect();
    
    // 获取样式信息
    QColor fillColor = m_metadata ? m_metadata->portFillColorAsQColor() : Qt::white;
    QColor borderColor = m_metadata ? m_metadata->portBorderColorAsQColor() : Qt::black;
    double borderWidth = m_metadata ? m_metadata->portBorderWidth() : 1.0;
    QString shape = m_metadata ? m_metadata->portShape() : "Circle";
    
    // 绘制端口
    painter->setPen(QPen(borderColor, borderWidth));
    painter->setBrush(QBrush(fillColor));
    
    if (shape == "Square" || shape == "Rectangle") {
        painter->drawRect(rect);
    } else {
        // 默认圆形
        painter->drawEllipse(rect);
    }
    
    // 注意：端口标签由 PortLabelItem 绘制，不需要在这里绘制
}

void PortBasicItem::setupLabel() {
    // 检查标签是否可见
    if (m_metadata && !m_metadata->labelVisible()) {
        if (m_labelItem) {
            delete m_labelItem;
            m_labelItem = nullptr;
        }
        return;
    }
    
    // 获取标签文本
    QString labelText;
    if (m_metadata && !m_metadata->portLabel().isEmpty()) {
        labelText = m_metadata->portLabel();
    } else if (m_port && !m_port->declaredName().isEmpty()) {
        labelText = m_port->declaredName();
    }
    
    if (labelText.isEmpty()) {
        if (m_labelItem) {
            delete m_labelItem;
            m_labelItem = nullptr;
        }
        return;
    }
    
    // 清除现有标签项
    if (m_labelItem) {
        delete m_labelItem;
        m_labelItem = nullptr;
    }
    
    // 获取标签方向
    // 如果用户指定了 labelDirection，使用用户指定的；否则根据 portPosition 设置默认值
    QString portPosition = m_metadata ? m_metadata->portPosition() : "left";
    QString labelDirection = m_metadata ? m_metadata->labelDirection() : QString();
    
    // 如果用户没有指定 labelDirection，根据 portPosition 设置默认值
    // 右侧端口：标签在右上方（标签在端口右侧，端口顶部上方）
    // 左侧端口：标签在左上方（标签在端口左侧，端口顶部上方）
    if (labelDirection.isEmpty()) {
        if (portPosition == "right") {
            labelDirection = "right";  // 右侧端口，标签在右上方
        } else if (portPosition == "left") {
            labelDirection = "left";   // 左侧端口，标签在左上方
        } else if (portPosition == "top") {
            labelDirection = "top";    // 顶部端口，标签在上方
        } else {
            labelDirection = "bottom";  // 底部端口，标签在下方
        }
        qDebug() << "[PortBasicItem] setupLabel: no labelDirection specified, using default based on portPosition:"
                 << portPosition << "->" << labelDirection;
    } else {
        qDebug() << "[PortBasicItem] setupLabel: using user-specified labelDirection:" << labelDirection;
    }
    
    double labelOffsetX = m_metadata ? m_metadata->labelOffsetX() : 0.0;
    double labelOffsetY = m_metadata ? m_metadata->labelOffsetY() : 0.0;
    
    // 创建标签项
    m_labelItem = new PortLabelItem(labelText, labelDirection, this);
    m_labelItem->setOffset(labelOffsetX, labelOffsetY);
    
    qDebug() << "[PortBasicItem] setupLabel: created label:" << labelText.left(50)
             << "portPosition:" << portPosition
             << "labelDirection:" << labelDirection
             << "maxWidth:" << m_labelItem->maxWidth();
    
    // 更新标签位置
    updateLabelPosition();
}

void PortBasicItem::updateLabelPosition() {
    if (m_labelItem) {
        m_labelItem->updatePosition(boundingRect());
    }
}

QPointF PortBasicItem::connectionPoint() const {
    // 连接点位于端口中心（相对于端口项）
    return QPointF(0, 0);
}

void PortBasicItem::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    qDebug() << "[PortBasicItem] mousePressEvent: CALLED"
             << "port:" << (m_port ? m_port->declaredName() : "null")
             << "pos:" << event->pos()
             << "scenePos:" << event->scenePos()
             << "button:" << event->button()
             << "modifiers:" << event->modifiers()
             << "zValue:" << zValue();
    
    // 立即接受事件，阻止传播到父项或 view
    event->accept();
    
    if (event->button() == Qt::LeftButton) {
        // 记录按下位置
        m_pressPos = event->scenePos();
        // 基础 part 的端口通常不可移动，直接进入连接创建模式
        m_creatingConnection = true;
        qDebug() << "[PortBasicItem] mousePressEvent: ✓ Entering connection creation mode"
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
    }
    
    // 如果事件未被处理，调用基类处理（但通常不会到达这里，因为事件已被接受）
    QGraphicsItem::mousePressEvent(event);
}

void PortBasicItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
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

void PortBasicItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
    qDebug() << "[PortBasicItem] mouseReleaseEvent:"
             << "port:" << (m_port ? m_port->declaredName() : "null")
             << "pos:" << event->pos()
             << "scenePos:" << event->scenePos()
             << "button:" << event->button()
             << "creatingConnection:" << m_creatingConnection;
    
    if (m_creatingConnection && event->button() == Qt::LeftButton) {
        qDebug() << "[PortBasicItem] mouseReleaseEvent: In connection creation mode, checking release conditions";
        
        // 检查是否移动了足够距离
        QPointF delta = event->scenePos() - m_pressPos;
        double distance = qSqrt(delta.x() * delta.x() + delta.y() * delta.y());
        
        qDebug() << "[PortBasicItem] mouseReleaseEvent: Movement check"
                 << "pressPos:" << m_pressPos
                 << "releasePos:" << event->scenePos()
                 << "delta:" << delta
                 << "distance:" << distance
                 << "threshold: 10.0";
        
        // 如果移动距离很小（小于 10 像素），可能是误点击，不创建连接
        // 如果移动距离较大，说明用户想要创建连接，检查是否释放到另一个端口上
        if (distance < 10.0) {
            m_creatingConnection = false;
            qDebug() << "[PortBasicItem] mouseReleaseEvent: ✗ Moved too little, not creating connection"
                     << "distance:" << distance << "< threshold: 10.0";
            QGraphicsItem::mouseReleaseEvent(event);
            return;
        }
        
        qDebug() << "[PortBasicItem] mouseReleaseEvent: ✓ Moved enough distance, checking for target port";
        
        // 检查是否释放到另一个端口上
        QList<QGraphicsItem*> itemsAtPos = scene()->items(event->scenePos());
        qDebug() << "[PortBasicItem] mouseReleaseEvent: Found" << itemsAtPos.size() << "items at release position";
        
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
            if (itemType == GraphicsItemTypes::PortGraphicsItem) {
                targetItem = item;
                break;
            } else if (itemType == GraphicsItemTypes::PortBasicItem && item != this) {
                targetItem = item;
                break;
            }
        }
        
        if (!targetItem) {
            qDebug() << "[PortBasicItem] mouseReleaseEvent: ✗ No port item found at release position";
            m_creatingConnection = false;
            QGraphicsItem::mouseReleaseEvent(event);
            return;
        }
        
        // 使用 type() 方法判断类型，比 qgraphicsitem_cast 更安全
        int targetItemType = targetItem->type();
        bool isPortGraphics = (targetItemType == GraphicsItemTypes::PortGraphicsItem);
        bool isPortBasic = (targetItemType == GraphicsItemTypes::PortBasicItem);
        
        qDebug() << "[PortBasicItem] mouseReleaseEvent: Target port item found"
                 << "targetItemType:" << targetItemType
                 << "isPortGraphics:" << isPortGraphics
                 << "isPortBasic:" << isPortBasic
                 << "isSelf:" << (targetItem == this);
        
        // 如果找到了目标端口且不是自己（比较图形项本身，而不是端口元素，因为相同类型不同实例的同一端口可能共享同一个端口元素）
        if ((isPortGraphics && targetItem != this) || 
            (isPortBasic && targetItem != this)) {
            
            qDebug() << "[PortBasicItem] mouseReleaseEvent: ✓ Valid target port found, getting GraphicViewWidget";
            
            // 获取 GraphicViewWidget
            QList<QGraphicsView*> views = scene()->views();
            qDebug() << "[PortBasicItem] mouseReleaseEvent: Scene has" << views.size() << "views";
            
            GraphicViewWidget* view = nullptr;
            for (QGraphicsView* v : views) {
                view = qobject_cast<GraphicViewWidget*>(v);
                if (view) {
                    qDebug() << "[PortBasicItem] mouseReleaseEvent: ✓ Found GraphicViewWidget";
                    break;
                }
            }
            
            if (!view) {
                qDebug() << "[PortBasicItem] mouseReleaseEvent: ✗ GraphicViewWidget not found";
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
                qDebug() << "[PortBasicItem] mouseReleaseEvent: ✓ Calling createConnection"
                         << "sourcePort:" << m_port->declaredName()
                         << "targetPort:" << targetPort->declaredName();
                // 传递图形项，以便找到正确的 PartUsage 实例
                QGraphicsItem* sourceItem = this;
                QGraphicsItem* targetPortItem = targetItem;
                Element* connection = view->createConnection(m_port, targetPort, sourceItem, targetPortItem);
                if (connection) {
                    qDebug() << "[PortBasicItem] mouseReleaseEvent: ✓ Connection created successfully"
                             << "connection:" << connection->declaredName();
                } else {
                    qDebug() << "[PortBasicItem] mouseReleaseEvent: ✗ Connection creation failed";
                }
            } else {
                qDebug() << "[PortBasicItem] mouseReleaseEvent: ✗ Invalid ports"
                         << "sourcePort:" << (m_port ? m_port->declaredName() : "null")
                         << "targetPort:" << (targetPort ? targetPort->declaredName() : "null");
            }
        } else {
            qDebug() << "[PortBasicItem] mouseReleaseEvent: ✗ Target port is self or invalid";
        }
        
        // 退出连接创建模式
        m_creatingConnection = false;
    } else {
        qDebug() << "[PortBasicItem] mouseReleaseEvent: Not in connection creation mode or wrong button"
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

void PortBasicItem::contextMenuEvent(QGraphicsSceneContextMenuEvent* event) {
    qDebug() << "[PortBasicItem] contextMenuEvent triggered"
             << "port =" << (m_port ? m_port->declaredName() : "null")
             << "hasWidget =" << (m_basicViewWidget != nullptr);

    if (!m_basicViewWidget || !m_port) {
        return;
    }
    
    QMenu menu;
    QAction* deleteAction = menu.addAction("删除端口");
    
    QObject::connect(deleteAction, &QAction::triggered, [this]() {
        if (!m_basicViewWidget || !m_port) {
            return;
        }
        
        int ret = QMessageBox::question(
            m_basicViewWidget,
            "确认删除",
            QString("确定要删除端口 \"%1\" 吗？").arg(m_port->declaredName()),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
        );
        
        if (ret == QMessageBox::Yes) {
            m_basicViewWidget->deletePort(m_port);
        }
    });
    
    qDebug() << "[PortBasicItem] contextMenuEvent: show menu at" << event->screenPos();
    menu.exec(event->screenPos());
    event->accept();  // 标记事件已处理
}

void PortBasicItem::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Delete && m_basicViewWidget && m_port) {
        int ret = QMessageBox::question(
            scene()->views().first(),
            "确认删除",
            QString("确定要删除端口 \"%1\" 吗？").arg(m_port->declaredName()),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
        );
        
        if (ret == QMessageBox::Yes) {
            m_basicViewWidget->deletePort(m_port);
        }
        event->accept();
    } else {
        QGraphicsItem::keyPressEvent(event);
    }
}

SYSML_NAMESPACE_END
