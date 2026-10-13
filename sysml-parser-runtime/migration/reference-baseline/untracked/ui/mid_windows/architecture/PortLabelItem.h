#ifndef PORTLABELITEM_H
#define PORTLABELITEM_H

#include "ElidedTextItem.h"

SYSML_NAMESPACE_BEGIN

/**
 * @brief PortLabelItem 端口标签显示项
 * 
 * 用于在基础视图中显示端口标签
 * 支持不同方向的标签显示、文本省略和 tooltip
 */
class PortLabelItem : public ElidedTextItem {
public:
    explicit PortLabelItem(const QString& label, const QString& direction = "right", QGraphicsItem* parent = nullptr);
    
    // 设置标签文本
    void setLabel(const QString& label) { setText(label); }
    QString label() const { return text(); }
    
    // 设置标签方向
    void setDirection(const QString& direction);
    QString direction() const { return m_direction; }
    
    // 设置偏移量
    void setOffset(double offsetX, double offsetY);
    double offsetX() const { return m_offsetX; }
    double offsetY() const { return m_offsetY; }
    
    // 根据方向和对齐方式更新位置（由 PortBasicItem 调用）
    void updatePosition(const QRectF& portRect);
    
private:
    QString m_direction;  // "left", "right", "top", "bottom"
    double m_offsetX = 0.0;
    double m_offsetY = 0.0;
    static constexpr double DEFAULT_MAX_WIDTH = 100.0;  // 默认最大宽度
};

SYSML_NAMESPACE_END

#endif // PORTLABELITEM_H
