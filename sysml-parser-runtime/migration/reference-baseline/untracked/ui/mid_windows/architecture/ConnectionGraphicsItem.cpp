#include "ConnectionGraphicsItem.h"
#include "PortGraphicsItem.h"
#include "PortBasicItem.h"
#include "GraphicViewWidget.h"
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsSceneContextMenuEvent>
#include <QMenu>
#include <QAction>
#include <QPainterPathStroker>

SYSML_NAMESPACE_BEGIN

ConnectionGraphicsItem::ConnectionGraphicsItem(Element* connection, ConnectionGraphicMetadata* metadata, QGraphicsItem* parent)
    : QGraphicsItem(parent)
    , m_connection(connection)
    , m_metadata(metadata)
    , m_sourcePort(nullptr)
    , m_targetPort(nullptr)
{
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    // 连接线的 z-order 应该低于端口，避免遮挡端口
    // 端口 z-order = 100，连接线设置为 5，确保端口在连接线之上
    setZValue(5);
    
    // 初始化路径
    updatePath();
}

QRectF ConnectionGraphicsItem::boundingRect() const {
    return m_boundingRect;
}

QPainterPath ConnectionGraphicsItem::shape() const {
    // 使用线宽附近的实际路径作为可点击区域，而不是整个 boundingRect
    // 这样可以减少在端口附近被连接线"盖住"的情况
    if (m_path.isEmpty()) {
        return QGraphicsItem::shape();
    }

    QPainterPathStroker stroker;
    double width = 5.0;
    if (m_metadata) {
        // 在线宽基础上增加一点余量，既方便点击连线，又不会过大
        // 但不要太大，避免覆盖端口
        width = m_metadata->lineWidth() + 3.0;  // 从 4.0 减少到 3.0
    }
    stroker.setWidth(width);
    
    QPainterPath strokePath = stroker.createStroke(m_path);
    
    // 在端口位置附近排除点击区域，避免遮挡端口
    // 如果连接线两端有端口，在端口中心附近排除一个圆形区域
    if (m_sourcePort || m_targetPort) {
        const double portExclusionRadius = 8.0;  // 端口排除半径
        
        // 获取源端口位置
        if (m_sourcePort) {
            QPointF sourcePos = m_sourcePort->scenePos();
            QPointF localSourcePos = mapFromScene(sourcePos);
            QPainterPath exclusionCircle;
            exclusionCircle.addEllipse(localSourcePos, portExclusionRadius, portExclusionRadius);
            strokePath = strokePath.subtracted(exclusionCircle);
        }
        
        // 获取目标端口位置
        if (m_targetPort) {
            QPointF targetPos = m_targetPort->scenePos();
            QPointF localTargetPos = mapFromScene(targetPos);
            QPainterPath exclusionCircle;
            exclusionCircle.addEllipse(localTargetPos, portExclusionRadius, portExclusionRadius);
            strokePath = strokePath.subtracted(exclusionCircle);
        }
    }
    
    return strokePath;
}

void ConnectionGraphicsItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    Q_UNUSED(option)
    Q_UNUSED(widget)
    
    if (m_path.isEmpty()) {
        return;
    }
    
    // 获取线型样式
    QString lineType = m_metadata ? m_metadata->lineType() : "Solid";
    QString lineColor = m_metadata ? m_metadata->lineColor() : "#000000";
    double lineWidth = m_metadata ? m_metadata->lineWidth() : 1.0;
    
    // 设置画笔
    QPen pen(QColor(lineColor), lineWidth);
    if (lineType == "Dashed") {
        pen.setStyle(Qt::DashLine);
    } else if (lineType == "Dotted") {
        pen.setStyle(Qt::DotLine);
    } else {
        pen.setStyle(Qt::SolidLine);
    }
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);
    
    // 绘制路径
    painter->drawPath(m_path);
    
    // 绘制箭头（简化版，只绘制目标箭头）
    if (m_metadata && m_metadata->targetArrow() != "None" && !m_path.isEmpty()) {
        // TODO: 实现箭头绘制
    }
}

void ConnectionGraphicsItem::setSourcePort(PortGraphicsItem* portItem) {
    if (m_sourcePort != portItem) {
        m_sourcePort = portItem;
        updatePath();
    }
}

void ConnectionGraphicsItem::setTargetPort(PortGraphicsItem* portItem) {
    if (m_targetPort != portItem) {
        m_targetPort = portItem;
        updatePath();
    }
}

void ConnectionGraphicsItem::setSourcePortBasic(PortBasicItem* portItem) {
    if (m_sourcePort != portItem) {
        m_sourcePort = portItem;
        updatePath();
    }
}

void ConnectionGraphicsItem::setTargetPortBasic(PortBasicItem* portItem) {
    if (m_targetPort != portItem) {
        m_targetPort = portItem;
        updatePath();
    }
}

void ConnectionGraphicsItem::onPortMoved(QGraphicsItem* portItem) {
    // 如果移动的端口是本连接线的任一端点，则更新路径
    if (!portItem) {
        return;
    }
    if (portItem == m_sourcePort || portItem == m_targetPort) {
        updatePath();
    }
}

QPointF ConnectionGraphicsItem::getPortConnectionPoint(QGraphicsItem* portItem) const {
    if (!portItem) {
        return QPointF();
    }
    
    // 尝试转换为PortGraphicsItem
    PortGraphicsItem* portGraphicsItem = qgraphicsitem_cast<PortGraphicsItem*>(portItem);
    if (portGraphicsItem) {
        return portGraphicsItem->connectionPoint();
    }
    
    // 尝试转换为PortBasicItem
    PortBasicItem* portBasicItem = qgraphicsitem_cast<PortBasicItem*>(portItem);
    if (portBasicItem) {
        return portBasicItem->connectionPoint();
    }
    
    // 默认返回端口中心
    return QPointF(0, 0);
}

void ConnectionGraphicsItem::updatePath() {
    m_path = QPainterPath();
    
    if (!m_sourcePort || !m_targetPort) {
        m_boundingRect = QRectF();
        return;
    }
    
    // 计算源端口和目标端口的连接点（相对于端口自身的本地坐标）
    QPointF sourceLocalPoint = getPortConnectionPoint(m_sourcePort);
    QPointF targetLocalPoint = getPortConnectionPoint(m_targetPort);
    
    // 将端口连接点转换为场景坐标
    QPointF sourceScenePos = m_sourcePort->mapToScene(sourceLocalPoint);
    QPointF targetScenePos = m_targetPort->mapToScene(targetLocalPoint);
    
    // 如果 ConnectionGraphicsItem 有 parent，需要将场景坐标转换为相对于 parent 的本地坐标
    QGraphicsItem* parent = parentItem();
    QPointF sourcePos, targetPos;
    
    if (parent) {
        // 有 parent：转换为相对于 parent 的本地坐标
        sourcePos = parent->mapFromScene(sourceScenePos);
        targetPos = parent->mapFromScene(targetScenePos);
    } else {
        // 没有 parent：使用场景坐标（但需要相对于 item 自身的位置）
        // 如果 item 在 scene 中，场景坐标就是本地坐标（因为 item 的 pos 是 (0,0)）
        sourcePos = sourceScenePos;
        targetPos = targetScenePos;
    }
    
    // 根据GraphicMetadata的pathType创建路径
    if (m_metadata && m_metadata->pathType() == "Orthogonal") {
        // 正交路径
        createOrthogonalPath(sourcePos, targetPos);
    } else {
        // 直线路径
        m_path.moveTo(sourcePos);
        m_path.lineTo(targetPos);
    }
    
    // 更新边界矩形
    m_boundingRect = m_path.boundingRect().adjusted(-5, -5, 5, 5);
    prepareGeometryChange();
    update();  // 触发重绘
}

void ConnectionGraphicsItem::createOrthogonalPath(const QPointF& source, const QPointF& target) {
    // 简化的正交路径（Manhattan路由）
    m_path = QPainterPath();
    m_path.moveTo(source);
    
    // 计算中间点
    QPointF mid1(source.x(), target.y());
    m_path.lineTo(mid1);
    m_path.lineTo(target);
}

void ConnectionGraphicsItem::contextMenuEvent(QGraphicsSceneContextMenuEvent* event) {
    if (!m_connection) {
        QGraphicsItem::contextMenuEvent(event);
        return;
    }
    
    QMenu menu;
    QAction* deleteAction = menu.addAction("删除连线");
    
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
            view->deleteConnection(m_connection);
        }
    });
    
    menu.exec(event->screenPos());
}

SYSML_NAMESPACE_END
