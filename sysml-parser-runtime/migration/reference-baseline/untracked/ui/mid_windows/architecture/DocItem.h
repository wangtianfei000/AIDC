#ifndef DOCITEM_H
#define DOCITEM_H

#include "ElidedTextItem.h"

SYSML_NAMESPACE_BEGIN

/**
 * @brief DocItem Doc 内容显示项
 * 
 * 用于在基础视图中显示 doc 内容
 * 支持文本省略和 tooltip
 */
class DocItem : public ElidedTextItem {
public:
    explicit DocItem(const QString& content, QGraphicsItem* parent = nullptr);
    
    // 设置 doc 内容
    void setContent(const QString& content) { setText(content); }
    QString content() const { return text(); }
};

SYSML_NAMESPACE_END

#endif // DOCITEM_H
