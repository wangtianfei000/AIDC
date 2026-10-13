#ifndef ITEMITEM_H
#define ITEMITEM_H

#include "ElidedTextItem.h"

SYSML_NAMESPACE_BEGIN

// 前向声明
class Element;

/**
 * @brief ItemItem Item显示项
 * 
 * 用于在基础视图中显示item
 * 支持文本省略、tooltip、右键菜单和删除
 */
class ItemItem : public ElidedTextItem {
public:
    explicit ItemItem(Element* item, const QString& name, QGraphicsItem* parent = nullptr);
    
    // 设置item名称
    void setName(const QString& name);
    
    QString name() const { return m_name; }
    Element* item() const { return m_item; }
    
    // 获取完整文本（用于 tooltip）
    QString fullText() const;
    
    // 设置BasicViewWidget（用于编辑功能）
    void setBasicViewWidget(class BasicViewWidget* widget) { m_basicViewWidget = widget; }
    
protected:
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    
private:
    void updateItemText();
    
    Element* m_item = nullptr;
    QString m_name;
    class BasicViewWidget* m_basicViewWidget = nullptr;
};

SYSML_NAMESPACE_END

#endif // ITEMITEM_H
