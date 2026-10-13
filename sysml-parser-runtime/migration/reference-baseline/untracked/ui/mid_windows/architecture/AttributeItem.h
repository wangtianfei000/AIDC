#ifndef ATTRIBUTEITEM_H
#define ATTRIBUTEITEM_H

#include "ElidedTextItem.h"

SYSML_NAMESPACE_BEGIN

// 前向声明
class Element;

/**
 * @brief AttributeItem 属性显示项
 * 
 * 用于在基础视图中显示属性
 * 支持文本省略和 tooltip
 */
class AttributeItem : public ElidedTextItem {
public:
    explicit AttributeItem(Element* attribute, const QString& name, const QString& value = QString(), QGraphicsItem* parent = nullptr);
    
    // 设置属性名称和值
    void setName(const QString& name);
    void setValue(const QString& value);
    
    QString name() const { return m_name; }
    QString value() const { return m_value; }
    Element* attribute() const { return m_attribute; }
    
    // 获取完整文本（用于 tooltip）
    QString fullText() const;
    
    // 设置BasicViewWidget（用于编辑功能）
    void setBasicViewWidget(class BasicViewWidget* widget) { m_basicViewWidget = widget; }
    
protected:
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    
private:
    void updateAttributeText();
    
    Element* m_attribute = nullptr;
    QString m_name;
    QString m_value;
    class BasicViewWidget* m_basicViewWidget = nullptr;
};

SYSML_NAMESPACE_END

#endif // ATTRIBUTEITEM_H
