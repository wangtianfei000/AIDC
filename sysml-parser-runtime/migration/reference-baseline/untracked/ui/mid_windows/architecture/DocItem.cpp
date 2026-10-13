#include "DocItem.h"

SYSML_NAMESPACE_BEGIN

DocItem::DocItem(const QString& content, QGraphicsItem* parent)
    : ElidedTextItem(content, parent)
{
    // 设置 doc 的样式
    QFont font("Arial", 9);
    font.setItalic(true);
    setFont(font);
    
    // 使用右侧省略模式
    setElideMode(Qt::ElideRight);
    
    // 左对齐
    setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    
    // 设置文本颜色
    setDefaultTextColor(Qt::black);
}

SYSML_NAMESPACE_END
