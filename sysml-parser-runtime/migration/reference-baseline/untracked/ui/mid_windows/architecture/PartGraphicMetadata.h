#ifndef PARTGRAPHICMETADATA_H
#define PARTGRAPHICMETADATA_H

#include "BaseGraphicMetadata.h"
#include "../../../core/model/Element.h"
#include <QVariantMap>
#include <QPointF>
#include <QSizeF>
#include <QRectF>
#include <QColor>
#include <QMetaType>

SYSML_NAMESPACE_BEGIN

/**
 * @brief PartGraphicMetadata 子Part的图形化元数据
 * 
 * 用于图形视图中的子Part，存储位置、大小、样式等图形信息
 */
class PartGraphicMetadata : public BaseGraphicMetadata {
    Q_OBJECT

public:
    explicit PartGraphicMetadata(QObject* parent = nullptr);
    
    // 位置和大小
    double graphicX() const { return m_graphicX; }
    void setGraphicX(double x) { m_graphicX = x; }
    double graphicY() const { return m_graphicY; }
    void setGraphicY(double y) { m_graphicY = y; }
    double graphicWidth() const { return m_graphicWidth; }
    void setGraphicWidth(double w) { m_graphicWidth = w; }
    double graphicHeight() const { return m_graphicHeight; }
    void setGraphicHeight(double h) { m_graphicHeight = h; }
    double graphicRotation() const { return m_graphicRotation; }
    void setGraphicRotation(double r) { m_graphicRotation = r; }
    int graphicZOrder() const { return m_graphicZOrder; }
    void setGraphicZOrder(int z) { m_graphicZOrder = z; }
    
    // 形状和样式
    QString shape() const { return m_shape; }
    void setShape(const QString& s) { m_shape = s; }
    double shapeRadius() const { return m_shapeRadius; }
    void setShapeRadius(double r) { m_shapeRadius = r; }
    QString fillColor() const { return m_fillColor; }
    void setFillColor(const QString& c) { m_fillColor = c; }
    double fillOpacity() const { return m_fillOpacity; }
    void setFillOpacity(double o) { m_fillOpacity = o; }
    QString borderColor() const { return m_borderColor; }
    void setBorderColor(const QString& c) { m_borderColor = c; }
    double borderWidth() const { return m_borderWidth; }
    void setBorderWidth(double w) { m_borderWidth = w; }
    QString borderStyle() const { return m_borderStyle; }
    void setBorderStyle(const QString& s) { m_borderStyle = s; }
    
    // 标注
    QString labelText() const { return m_labelText; }
    void setLabelText(const QString& t) { m_labelText = t; }
    QString labelFont() const { return m_labelFont; }
    void setLabelFont(const QString& f) { m_labelFont = f; }
    QString labelColor() const { return m_labelColor; }
    void setLabelColor(const QString& c) { m_labelColor = c; }
    QString labelPosition() const { return m_labelPosition; }
    void setLabelPosition(const QString& p) { m_labelPosition = p; }
    bool labelVisible() const { return m_labelVisible; }
    void setLabelVisible(bool v) { m_labelVisible = v; }
    
    // 可见性和交互
    bool visible() const { return m_visible; }
    void setVisible(bool v) { m_visible = v; }
    bool selectable() const { return m_selectable; }
    void setSelectable(bool s) { m_selectable = s; }
    bool movable() const { return m_movable; }
    void setMovable(bool m) { m_movable = m; }
    bool resizable() const { return m_resizable; }
    void setResizable(bool r) { m_resizable = r; }
    
    // 嵌套内容显示控制
    bool showChildNestedContent() const { return m_showChildNestedContent; }
    void setShowChildNestedContent(bool s) { m_showChildNestedContent = s; }
    
    // 工具方法
    QPointF position() const { return QPointF(m_graphicX, m_graphicY); }
    QSizeF size() const { return QSizeF(m_graphicWidth, m_graphicHeight); }
    QRectF boundingRect() const;
    QColor fillColorAsQColor() const;
    QColor borderColorAsQColor() const;
    
    // 实现基类接口
    void loadFromMap(const QVariantMap& map) override;
    QVariantMap toMap() const override;
    QString metadataType() const override { return "PartGraphic"; }
    
    // 保存到模型
    void saveToModel(Element* element);

private:
    double m_graphicX = 0.0;
    double m_graphicY = 0.0;
    double m_graphicWidth = -1.0;  // -1表示自动计算
    double m_graphicHeight = -1.0; // -1表示自动计算
    double m_graphicRotation = 0.0;
    int m_graphicZOrder = 0;
    QString m_shape = "Rectangle";
    double m_shapeRadius = 5.0;
    QString m_fillColor = "#FFFFFF";
    double m_fillOpacity = 1.0;
    QString m_borderColor = "#000000";
    double m_borderWidth = 1.0;
    QString m_borderStyle = "Solid";
    QString m_labelText;
    QString m_labelFont = "Arial, 10pt";
    QString m_labelColor = "#000000";
    QString m_labelPosition = "Top";
    bool m_labelVisible = true;
    bool m_visible = true;
    bool m_selectable = true;
    bool m_movable = true;
    bool m_resizable = true;
    bool m_showChildNestedContent = false;
};

SYSML_NAMESPACE_END

Q_DECLARE_METATYPE(SysML::PartGraphicMetadata*)

#endif // PARTGRAPHICMETADATA_H
