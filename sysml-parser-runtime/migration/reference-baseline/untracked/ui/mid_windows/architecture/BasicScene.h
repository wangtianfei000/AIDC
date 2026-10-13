#ifndef BASICSCENE_H
#define BASICSCENE_H

#include <QGraphicsScene>
#include "../../../core/common/Global.h"

SYSML_NAMESPACE_BEGIN

/**
 * @brief BasicScene 基础视图场景
 * 
 * 继承自QGraphicsScene，管理基础视图中的所有图形项
 */
class BasicScene : public QGraphicsScene {
    Q_OBJECT

public:
    explicit BasicScene(QObject* parent = nullptr);
    ~BasicScene() override = default;

signals:
    // 当点击空白区域时发出此信号，用于显示切换视图菜单
    void blankAreaContextMenuRequested(const QPoint& globalPos);

protected:
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;
};

SYSML_NAMESPACE_END

#endif // BASICSCENE_H
