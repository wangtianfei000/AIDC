#include "ItemItem.h"
#include "BasicViewWidget.h"
#include "../../../core/model/Element.h"
#include <QGraphicsSceneContextMenuEvent>
#include <QKeyEvent>
#include <QMenu>
#include <QMessageBox>
#include <QObject>

SYSML_NAMESPACE_BEGIN

ItemItem::ItemItem(Element* item, const QString& name, QGraphicsItem* parent)
    : ElidedTextItem(QString(), parent)
    , m_item(item)
    , m_name(name)
{
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    
    // 设置item的样式
    QFont font("Arial", 10);
    setFont(font);
    
    // 使用右侧省略模式
    setElideMode(Qt::ElideRight);
    
    // 左对齐
    setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    
    // 设置文本颜色
    setDefaultTextColor(Qt::black);
    
    // 更新显示文本
    updateItemText();
}

void ItemItem::setName(const QString& name) {
    if (m_name != name) {
        m_name = name;
        updateItemText();
    }
}

QString ItemItem::fullText() const {
    return m_name;
}

void ItemItem::updateItemText() {
    // 调用基类方法设置文本
    ElidedTextItem::setText(m_name);
}

void ItemItem::contextMenuEvent(QGraphicsSceneContextMenuEvent* event) {
    qDebug() << "[ItemItem] contextMenuEvent triggered"
             << "name =" << m_name
             << "hasWidget =" << (m_basicViewWidget != nullptr)
             << "hasItemElement =" << (m_item != nullptr);

    if (!m_basicViewWidget || !m_item) {
        return;
    }
    
    QMenu menu;
    QAction* deleteAction = menu.addAction("删除Item");
    
    QObject::connect(deleteAction, &QAction::triggered, [this]() {
        if (!m_basicViewWidget || !m_item) {
            return;
        }
        
        int ret = QMessageBox::question(
            m_basicViewWidget,
            "确认删除",
            QString("确定要删除Item \"%1\" 吗？").arg(m_name),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
        );
        
        if (ret == QMessageBox::Yes) {
            m_basicViewWidget->deleteItem(m_item);
        }
    });
    
    qDebug() << "[ItemItem] contextMenuEvent: show menu at" << event->screenPos();
    menu.exec(event->screenPos());
    event->accept();  // 标记事件已处理
}

void ItemItem::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Delete && m_basicViewWidget && m_item) {
        int ret = QMessageBox::question(
            scene()->views().first(),
            "确认删除",
            QString("确定要删除Item \"%1\" 吗？").arg(m_name),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
        );
        
        if (ret == QMessageBox::Yes) {
            m_basicViewWidget->deleteItem(m_item);
        }
        event->accept();
    } else {
        ElidedTextItem::keyPressEvent(event);
    }
}

SYSML_NAMESPACE_END
