#include "PortLabelItem.h"
#include <QFontMetrics>

SYSML_NAMESPACE_BEGIN

PortLabelItem::PortLabelItem(const QString& label, const QString& direction, QGraphicsItem* parent)
    : ElidedTextItem(label, parent)
    , m_direction(direction)
{
    // 设置端口标签的样式
    QFont font("Arial", 8);
    setFont(font);
    
    // 使用右侧省略模式
    setElideMode(Qt::ElideRight);
    
    // 设置默认最大宽度
    setMaxWidth(DEFAULT_MAX_WIDTH);
    
    // 设置文本颜色
    setDefaultTextColor(Qt::black);
    
    // 根据方向设置对齐方式
    // 注意：对齐方式影响文本在 boundingRect 内的位置
    // left: 标签在端口左侧，文本右对齐（文本右边缘对齐 boundingRect 右边缘），顶部对齐
    // right: 标签在端口右侧，文本左对齐（文本左边缘对齐 boundingRect 左边缘），顶部对齐
    if (direction == "left") {
        setAlignment(Qt::AlignRight | Qt::AlignTop);  // 右对齐，顶部对齐
    } else if (direction == "right") {
        setAlignment(Qt::AlignLeft | Qt::AlignTop);   // 左对齐，顶部对齐
    } else {
        setAlignment(Qt::AlignCenter);
    }
}

void PortLabelItem::setDirection(const QString& direction) {
    if (m_direction != direction) {
        m_direction = direction;
        
        // 更新对齐方式
        if (direction == "left") {
            setAlignment(Qt::AlignRight | Qt::AlignTop);  // 右对齐，顶部对齐
        } else if (direction == "right") {
            setAlignment(Qt::AlignLeft | Qt::AlignTop);   // 左对齐，顶部对齐
        } else {
            setAlignment(Qt::AlignCenter);
        }
    }
}

void PortLabelItem::setOffset(double offsetX, double offsetY) {
    m_offsetX = offsetX;
    m_offsetY = offsetY;
}

void PortLabelItem::updatePosition(const QRectF& portRect) {
    // 确保 boundingRect 已更新
    QRectF labelRect = boundingRect();
    QPointF newPos;
    
    // 计算端口边界
    double portTop = portRect.top();
    double portBottom = portRect.bottom();
    double portLeft = portRect.left();
    double portRight = portRect.right();
    QPointF portCenter = portRect.center();
    
    // 标签与端口的间距
    const double LABEL_SPACING = 5.0;
    
    if (m_direction == "left") {
        // 标签在端口左侧，默认位置：左上方
        // 标签右对齐，所以需要计算标签右边缘的位置
        // 标签右边缘应该在端口左边缘左侧 LABEL_SPACING 距离
        // 标签顶部应该在端口顶部上方（标签完全在端口上方，不覆盖端口）
        // 注意：由于标签右对齐，标签的右边缘在 boundingRect 的右边缘
        // 所以标签的左上角位置 = 标签右边缘位置 - labelRect.width()
        newPos = QPointF(portLeft - LABEL_SPACING - labelRect.width() + m_offsetX,  // 标签右边缘在端口左侧
                        portTop - labelRect.height() + m_offsetY);  // 标签顶部对齐端口顶部（标签在端口上方）
    } else if (m_direction == "right") {
        // 标签在端口右侧，默认位置：右上方
        // 标签左对齐，所以标签的左边缘在端口右边缘右侧 LABEL_SPACING 距离
        // 标签顶部应该在端口顶部上方（标签完全在端口上方，不覆盖端口）
        // 注意：由于标签左对齐，标签的左边缘在 boundingRect 的左边缘（即 pos()）
        newPos = QPointF(portRight + LABEL_SPACING + m_offsetX,  // 标签左边缘在端口右侧
                        portTop - labelRect.height() + m_offsetY);  // 标签顶部对齐端口顶部（标签在端口上方）
    } else if (m_direction == "top") {
        // 标签在端口上方，居中对齐
        newPos = QPointF(portCenter.x() - labelRect.width() / 2 + m_offsetX,
                        portTop - labelRect.height() - LABEL_SPACING + m_offsetY);
    } else {  // bottom
        // 标签在端口下方，居中对齐
        newPos = QPointF(portCenter.x() - labelRect.width() / 2 + m_offsetX,
                        portBottom + LABEL_SPACING + m_offsetY);
    }
    
    setPos(newPos);
}

SYSML_NAMESPACE_END
