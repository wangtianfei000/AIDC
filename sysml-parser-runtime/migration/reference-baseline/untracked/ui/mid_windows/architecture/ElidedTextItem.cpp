#include "ElidedTextItem.h"
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsSceneHoverEvent>
#include <QToolTip>
#include <QFontMetrics>
#include <QTextDocument>

SYSML_NAMESPACE_BEGIN

ElidedTextItem::ElidedTextItem(const QString& text, QGraphicsItem* parent)
    : QGraphicsTextItem(parent)
    , m_fullText(text)
{
    setAcceptHoverEvents(true);  // 启用悬停事件（用于 tooltip）
    
    // 设置默认字体
    setFont(QFont("Arial", 10));
    
    // 禁用文本换行（单行显示）
    document()->setTextWidth(-1);
    
    // 初始化缓存的边界矩形
    m_cachedBoundingRect = QRectF();
    
    updateDisplayText();
}

QRectF ElidedTextItem::boundingRect() const {
    return m_cachedBoundingRect;
}

void ElidedTextItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    Q_UNUSED(option)
    Q_UNUSED(widget)
    
    // 由于需要支持对齐和省略号，使用自定义绘制
    painter->save();
    
    // 设置字体和颜色
    painter->setFont(font());
    painter->setPen(defaultTextColor());
    
    // 获取显示文本（已经经过省略处理）
    QString displayText = toPlainText();
    
    if (displayText.isEmpty()) {
        painter->restore();
        return;
    }
    
    // 使用缓存的边界矩形（已经限制在 maxWidth 内）
    QRectF textRect = m_cachedBoundingRect;
    
    // 设置裁剪区域，确保文本不会超出 maxWidth
    painter->setClipRect(textRect);
    
    // 绘制文本（使用指定的对齐方式，限制在 textRect 内）
    painter->drawText(textRect, m_alignment, displayText);
    
    painter->restore();
}

void ElidedTextItem::setText(const QString& text) {
    if (m_fullText != text) {
        m_fullText = text;
        updateDisplayText();
    }
}

void ElidedTextItem::setMaxWidth(double width) {
    if (m_maxWidth != width) {
        m_maxWidth = width;
        updateDisplayText();
    }
}

void ElidedTextItem::setElideMode(Qt::TextElideMode mode) {
    if (m_elideMode != mode) {
        m_elideMode = mode;
        updateDisplayText();
    }
}

void ElidedTextItem::setAlignment(Qt::Alignment alignment) {
    if (m_alignment != alignment) {
        m_alignment = alignment;
        update();
    }
}

void ElidedTextItem::updateDisplayText() {
    prepareGeometryChange();
    
    if (m_fullText.isEmpty()) {
        setPlainText("");
        setToolTip("");
        document()->setTextWidth(0);
        m_cachedBoundingRect = QRectF(0, 0, 0, 0);
        return;
    }
    
    QFontMetrics fm(font());
    
    // 计算文本宽度
    int textWidth = fm.horizontalAdvance(m_fullText);
    
    QString displayText;
    if (textWidth <= static_cast<int>(m_maxWidth)) {
        // 文本未超出，直接显示
        displayText = m_fullText;
        setToolTip("");  // 不需要 tooltip
    } else {
        // 文本超出，使用省略号
        displayText = fm.elidedText(m_fullText, m_elideMode, static_cast<int>(m_maxWidth));
        setToolTip(m_fullText);  // 设置 tooltip 显示完整文本
    }
    
    // 设置显示的文本
    setPlainText(displayText);
    
    // 计算实际显示宽度（不超过 maxWidth）
    int displayWidth = fm.horizontalAdvance(displayText);
    int height = fm.height();
    
    // 更新文档宽度（用于 QGraphicsTextItem 的内部计算）
    // 使用实际显示宽度，但确保不超过 maxWidth
    QTextDocument* doc = document();
    double docWidth = qMin(static_cast<double>(displayWidth), m_maxWidth);
    doc->setTextWidth(docWidth);
    doc->setDocumentMargin(0);
    
    // 更新缓存的边界矩形（基于 maxWidth 限制）
    // 宽度使用 maxWidth，确保不会超出
    m_cachedBoundingRect = QRectF(0, 0, m_maxWidth, height);
}

void ElidedTextItem::hoverEnterEvent(QGraphicsSceneHoverEvent* event) {
    // 如果文本被省略，tooltip 已经通过 setToolTip() 设置
    // 这里可以额外处理，但通常不需要
    QGraphicsTextItem::hoverEnterEvent(event);
}

void ElidedTextItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* event) {
    QGraphicsTextItem::hoverLeaveEvent(event);
}

SYSML_NAMESPACE_END
