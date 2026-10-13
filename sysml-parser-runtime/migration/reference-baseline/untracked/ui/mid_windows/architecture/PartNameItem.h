#ifndef PARTNAMEITEM_H
#define PARTNAMEITEM_H

#include "ElidedTextItem.h"

SYSML_NAMESPACE_BEGIN

/**
 * @brief PartNameItem Part 名称显示项
 * 
 * 用于在基础视图中显示 part 名称
 * 支持文本省略和 tooltip
 */
class PartNameItem : public ElidedTextItem {
public:
    explicit PartNameItem(const QString& name, QGraphicsItem* parent = nullptr);
    
    // 设置名称
    void setName(const QString& name) { setText(name); }
    QString name() const { return text(); }
};

SYSML_NAMESPACE_END

#endif // PARTNAMEITEM_H
