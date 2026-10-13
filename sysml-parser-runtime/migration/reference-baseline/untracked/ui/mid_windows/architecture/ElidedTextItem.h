#ifndef ELIDEDTEXTITEM_H
#define ELIDEDTEXTITEM_H

#include <QGraphicsTextItem>
#include <QFont>
#include "../../../core/common/Global.h"

SYSML_NAMESPACE_BEGIN

/**
 * @brief ElidedTextItem 通用文本省略显示项
 * 
 * 继承自 QGraphicsTextItem，提供文本省略和 tooltip 支持
 * 所有需要显示文本并支持省略号的 Item 都可以继承此类
 */
class ElidedTextItem : public QGraphicsTextItem {
public:
    explicit ElidedTextItem(const QString& text, QGraphicsItem* parent = nullptr);
    
    // QGraphicsItem接口（重写以支持正确的边界矩形）
    QRectF boundingRect() const override;
    
    // 文本设置（重写以支持省略）
    void setText(const QString& text);
    QString text() const { return m_fullText; }
    
    // 宽度设置
    void setMaxWidth(double width);
    double maxWidth() const { return m_maxWidth; }
    
    // 省略模式设置
    void setElideMode(Qt::TextElideMode mode);
    Qt::TextElideMode elideMode() const { return m_elideMode; }
    
    // 对齐方式设置（通过 QTextDocument 实现）
    void setAlignment(Qt::Alignment alignment);
    Qt::Alignment alignment() const { return m_alignment; }
    
    // 工具方法
    QString fullText() const { return m_fullText; }
    QString displayText() const { return toPlainText(); }
    bool isElided() const { return toPlainText() != m_fullText; }
    
protected:
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;
    
    // 更新显示文本（根据最大宽度和省略模式）
    void updateDisplayText();
    
private:
    QString m_fullText;           // 完整文本
    double m_maxWidth = 200.0;     // 最大显示宽度
    Qt::TextElideMode m_elideMode = Qt::ElideRight;  // 省略模式
    Qt::Alignment m_alignment = Qt::AlignLeft | Qt::AlignVCenter;  // 对齐方式
    QRectF m_cachedBoundingRect;  // 缓存的边界矩形
};

SYSML_NAMESPACE_END

#endif // ELIDEDTEXTITEM_H
