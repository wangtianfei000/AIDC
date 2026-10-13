#ifndef PORTGRAPHICMETADATA_H
#define PORTGRAPHICMETADATA_H

#include "BaseGraphicMetadata.h"
#include "../../../core/model/Element.h"
#include <QVariantMap>
#include <QPointF>
#include <QColor>
#include <QMetaType>

SYSML_NAMESPACE_BEGIN

/**
 * @brief PortGraphicMetadata Port的图形化元数据
 * 
 * 用于图形视图中的Port，存储位置、大小、样式、标注等图形信息
 */
class PortGraphicMetadata : public BaseGraphicMetadata {
    Q_OBJECT

public:
    explicit PortGraphicMetadata(QObject* parent = nullptr);
    
    // 位置和大小
    double graphicX() const { return m_graphicX; }
    void setGraphicX(double x) { m_graphicX = x; }
    double graphicY() const { return m_graphicY; }
    void setGraphicY(double y) { m_graphicY = y; }
    double portSize() const { return m_portSize; }
    void setPortSize(double s) { m_portSize = s; }
    double portWidth() const { return m_portWidth; }
    void setPortWidth(double w) { m_portWidth = w; }
    double portHeight() const { return m_portHeight; }
    void setPortHeight(double h) { m_portHeight = h; }
    
    // 位置和方向（相对于父Part）
    QString portPosition() const { return m_portPosition; }
    void setPortPosition(const QString& p) { m_portPosition = p; }
    double portOffset() const { return m_portOffset; }
    void setPortOffset(double o) { m_portOffset = o; }
    QString portAlignment() const { return m_portAlignment; }
    void setPortAlignment(const QString& a) { m_portAlignment = a; }
    QString portDirection() const { return m_portDirection; }
    void setPortDirection(const QString& d) { m_portDirection = d; }
    double portAngle() const { return m_portAngle; }
    void setPortAngle(double a) { m_portAngle = a; }
    
    // 样式
    QString portShape() const { return m_portShape; }
    void setPortShape(const QString& s) { m_portShape = s; }
    QString portFillColor() const { return m_portFillColor; }
    void setPortFillColor(const QString& c) { m_portFillColor = c; }
    double portFillOpacity() const { return m_portFillOpacity; }
    void setPortFillOpacity(double o) { m_portFillOpacity = o; }
    QString portBorderColor() const { return m_portBorderColor; }
    void setPortBorderColor(const QString& c) { m_portBorderColor = c; }
    double portBorderWidth() const { return m_portBorderWidth; }
    void setPortBorderWidth(double w) { m_portBorderWidth = w; }
    QString portBorderStyle() const { return m_portBorderStyle; }
    void setPortBorderStyle(const QString& s) { m_portBorderStyle = s; }
    
    // 标注
    QString portLabel() const { return m_portLabel; }
    void setPortLabel(const QString& l) { m_portLabel = l; }
    QString labelDirection() const { return m_labelDirection; }
    void setLabelDirection(const QString& d) { m_labelDirection = d; }
    double labelOffsetX() const { return m_labelOffsetX; }
    void setLabelOffsetX(double x) { m_labelOffsetX = x; }
    double labelOffsetY() const { return m_labelOffsetY; }
    void setLabelOffsetY(double y) { m_labelOffsetY = y; }
    bool labelVisible() const { return m_labelVisible; }
    void setLabelVisible(bool v) { m_labelVisible = v; }
    
    // 可见性和交互
    bool portVisible() const { return m_portVisible; }
    void setPortVisible(bool v) { m_portVisible = v; }
    bool portSelectable() const { return m_portSelectable; }
    void setPortSelectable(bool s) { m_portSelectable = s; }
    bool portMovable() const { return m_portMovable; }
    void setPortMovable(bool m) { m_portMovable = m; }
    
    // 连接点
    double connectionPointX() const { return m_connectionPointX; }
    void setConnectionPointX(double x) { m_connectionPointX = x; }
    double connectionPointY() const { return m_connectionPointY; }
    void setConnectionPointY(double y) { m_connectionPointY = y; }
    double connectionAngle() const { return m_connectionAngle; }
    void setConnectionAngle(double a) { m_connectionAngle = a; }
    
    // 工具方法
    QPointF position() const { return QPointF(m_graphicX, m_graphicY); }
    QColor portFillColorAsQColor() const;
    QColor portBorderColorAsQColor() const;
    
    // 实现基类接口
    void loadFromMap(const QVariantMap& map) override;
    QVariantMap toMap() const override;
    QString metadataType() const override { return "PortGraphic"; }
    
    // 保存到模型
    void saveToModel(Element* element);

private:
    double m_graphicX = 0.0;
    double m_graphicY = 0.0;
    double m_portSize = 6.0;
    double m_portWidth = 6.0;
    double m_portHeight = 6.0;
    QString m_portPosition = "left";
    double m_portOffset = 0.0;
    QString m_portAlignment = "Center";
    QString m_portDirection = "In";
    double m_portAngle = 0.0;
    QString m_portShape = "Circle";
    QString m_portFillColor = "#FFFFFF";
    double m_portFillOpacity = 1.0;
    QString m_portBorderColor = "#000000";
    double m_portBorderWidth = 1.0;
    QString m_portBorderStyle = "Solid";
    QString m_portLabel;
    QString m_labelDirection;  // 默认为空字符串，表示未设置，将根据 portPosition 自动设置
    double m_labelOffsetX = 0.0;
    double m_labelOffsetY = 0.0;
    bool m_labelVisible = true;
    bool m_portVisible = true;
    bool m_portSelectable = true;
    // 默认允许拖动端口；如果需要禁止拖动，可在元数据中显式设置 portMovable = false
    bool m_portMovable = true;
    double m_connectionPointX = 0.0;
    double m_connectionPointY = 0.0;
    double m_connectionAngle = 0.0;
};

SYSML_NAMESPACE_END

Q_DECLARE_METATYPE(SysML::PortGraphicMetadata*)

#endif // PORTGRAPHICMETADATA_H
