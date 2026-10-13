#include "BasicScene.h"
#include <QGraphicsSceneContextMenuEvent>
#include <QGraphicsItem>
#include <QDebug>

SYSML_NAMESPACE_BEGIN

BasicScene::BasicScene(QObject* parent)
    : QGraphicsScene(parent)
{
    // 设置场景背景色
    setBackgroundBrush(QBrush(Qt::white));
}

void BasicScene::contextMenuEvent(QGraphicsSceneContextMenuEvent* event)
{
    // 先记录一下点击信息，便于调试
    QGraphicsItem* item = itemAt(event->scenePos(), QTransform());
    qDebug() << "[BasicScene] contextMenuEvent triggered"
             << "scenePos =" << event->scenePos()
             << "item =" << (item ? item->type() : -1)
             << "itemPtr =" << item;

    // 先调用基类实现，让图形项有机会处理右键菜单
    QGraphicsScene::contextMenuEvent(event);

    if (event->isAccepted()) {
        // 某个图形项已经处理了事件（例如 PartBasicItem / AttributeItem 等）
        qDebug() << "[BasicScene] contextMenuEvent: handled by item, return";
        return;
    }

    // 没有图形项处理（通常是点击空白区域），改为显示视图切换菜单
    qDebug() << "[BasicScene] contextMenuEvent: not handled by item, emit blankAreaContextMenuRequested at"
             << event->screenPos();
    emit blankAreaContextMenuRequested(event->screenPos());
    event->accept();
}

SYSML_NAMESPACE_END
