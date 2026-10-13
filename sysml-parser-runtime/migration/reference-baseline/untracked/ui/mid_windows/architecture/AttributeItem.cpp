#include "AttributeItem.h"
#include "BasicViewWidget.h"
#include "../../../core/model/Element.h"
#include <QGraphicsSceneContextMenuEvent>
#include <QKeyEvent>
#include <QMenu>
#include <QMessageBox>
#include <QObject>

SYSML_NAMESPACE_BEGIN

AttributeItem::AttributeItem(Element* attribute, const QString& name, const QString& value, QGraphicsItem* parent)
    : ElidedTextItem(QString(), parent)
    , m_attribute(attribute)
    , m_name(name)
    , m_value(value)
{
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    // 设置属性的样式
    QFont font("Arial", 10);
    setFont(font);
    
    // 使用右侧省略模式
    setElideMode(Qt::ElideRight);
    
    // 左对齐
    setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    
    // 设置文本颜色
    setDefaultTextColor(Qt::black);
    
    // 更新显示文本
    updateAttributeText();
}

void AttributeItem::setName(const QString& name) {
    if (m_name != name) {
        m_name = name;
        updateAttributeText();
    }
}

void AttributeItem::setValue(const QString& value) {
    if (m_value != value) {
        m_value = value;
        updateAttributeText();
    }
}

QString AttributeItem::fullText() const {
    if (m_value.isEmpty()) {
        return m_name;
    } else {
        return QString("%1 = %2").arg(m_name, m_value);
    }
}

void AttributeItem::updateAttributeText() {
    // 构建完整文本
    QString attrText;
    if (m_value.isEmpty()) {
        attrText = m_name;
    } else {
        attrText = QString("%1 = %2").arg(m_name, m_value);
    }
    
    // 调用基类方法设置文本
    ElidedTextItem::setText(attrText);
}

void AttributeItem::contextMenuEvent(QGraphicsSceneContextMenuEvent* event) {
    qDebug() << "[AttributeItem] contextMenuEvent triggered"
             << "name =" << m_name
             << "hasWidget =" << (m_basicViewWidget != nullptr)
             << "hasAttributeElement =" << (m_attribute != nullptr);

    if (!m_basicViewWidget || !m_attribute) {
        return;
    }
    
    QMenu menu;
    QAction* deleteAction = menu.addAction("删除属性");
    
    QObject::connect(deleteAction, &QAction::triggered, [this]() {
        if (!m_basicViewWidget || !m_attribute) {
            return;
        }
        
        int ret = QMessageBox::question(
            m_basicViewWidget,
            "确认删除",
            QString("确定要删除属性 \"%1\" 吗？").arg(m_name),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
        );
        
        if (ret == QMessageBox::Yes) {
            m_basicViewWidget->deleteAttribute(m_attribute);
        }
    });
    
    qDebug() << "[AttributeItem] contextMenuEvent: show menu at" << event->screenPos();
    menu.exec(event->screenPos());
    event->accept();  // 标记事件已处理
}

void AttributeItem::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Delete && m_basicViewWidget && m_attribute) {
        int ret = QMessageBox::question(
            scene()->views().first(),
            "确认删除",
            QString("确定要删除属性 \"%1\" 吗？").arg(m_name),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
        );
        
        if (ret == QMessageBox::Yes) {
            m_basicViewWidget->deleteAttribute(m_attribute);
        }
        event->accept();
    } else {
        ElidedTextItem::keyPressEvent(event);
    }
}

SYSML_NAMESPACE_END
