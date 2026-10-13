#include "PartNameItem.h"

SYSML_NAMESPACE_BEGIN

PartNameItem::PartNameItem(const QString& name, QGraphicsItem* parent)
    : ElidedTextItem(name, parent)
{
    // 设置 part 名称的样式
    QFont font("Arial", 12);
    font.setBold(true);
    setFont(font);
    
    // 使用中间省略模式，保留前后部分
    setElideMode(Qt::ElideMiddle);
    
    // 居中对齐
    setAlignment(Qt::AlignCenter);
    
    // 设置文本颜色
    setDefaultTextColor(Qt::black);
}

SYSML_NAMESPACE_END
