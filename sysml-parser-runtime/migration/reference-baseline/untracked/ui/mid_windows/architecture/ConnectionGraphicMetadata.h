#ifndef CONNECTIONGRAPHICMETADATA_H
#define CONNECTIONGRAPHICMETADATA_H

#include "BaseGraphicMetadata.h"
#include <QVariantMap>
#include <QList>
#include <QPointF>
#include <QColor>
#include <QMetaType>

SYSML_NAMESPACE_BEGIN

/**
 * @brief ConnectionGraphicMetadata Connection的图形化元数据
 * 
 * 用于图形视图中的Connection，存储路径、线型、箭头、标注等图形信息
 */
class ConnectionGraphicMetadata : public BaseGraphicMetadata {
    Q_OBJECT

public:
    explicit ConnectionGraphicMetadata(QObject* parent = nullptr);
    
    // 路径
    QString pathType() const { return m_pathType; }
    void setPathType(const QString& t) { m_pathType = t; }
    QList<QPointF> pathPoints() const { return m_pathPoints; }
    void setPathPoints(const QList<QPointF>& points) { m_pathPoints = points; }
    QString routingStyle() const { return m_routingStyle; }
    void setRoutingStyle(const QString& s) { m_routingStyle = s; }
    
    // 线型
    QString lineType() const { return m_lineType; }
    void setLineType(const QString& t) { m_lineType = t; }
    QString lineColor() const { return m_lineColor; }
    void setLineColor(const QString& c) { m_lineColor = c; }
    double lineWidth() const { return m_lineWidth; }
    void setLineWidth(double w) { m_lineWidth = w; }
    double lineOpacity() const { return m_lineOpacity; }
    void setLineOpacity(double o) { m_lineOpacity = o; }
    QString lineStyle() const { return m_lineStyle; }
    void setLineStyle(const QString& s) { m_lineStyle = s; }
    
    // 箭头
    QString sourceArrow() const { return m_sourceArrow; }
    void setSourceArrow(const QString& a) { m_sourceArrow = a; }
    QString targetArrow() const { return m_targetArrow; }
    void setTargetArrow(const QString& a) { m_targetArrow = a; }
    double arrowSize() const { return m_arrowSize; }
    void setArrowSize(double s) { m_arrowSize = s; }
    QString arrowColor() const { return m_arrowColor; }
    void setArrowColor(const QString& c) { m_arrowColor = c; }
    QString arrowFillColor() const { return m_arrowFillColor; }
    void setArrowFillColor(const QString& c) { m_arrowFillColor = c; }
    
    // 端点
    QString sourcePort() const { return m_sourcePort; }
    void setSourcePort(const QString& p) { m_sourcePort = p; }
    QString targetPort() const { return m_targetPort; }
    void setTargetPort(const QString& p) { m_targetPort = p; }
    double sourceOffset() const { return m_sourceOffset; }
    void setSourceOffset(double o) { m_sourceOffset = o; }
    double targetOffset() const { return m_targetOffset; }
    void setTargetOffset(double o) { m_targetOffset = o; }
    
    // 标注
    QString labelText() const { return m_labelText; }
    void setLabelText(const QString& t) { m_labelText = t; }
    QString labelPosition() const { return m_labelPosition; }
    void setLabelPosition(const QString& p) { m_labelPosition = p; }
    double labelOffsetX() const { return m_labelOffsetX; }
    void setLabelOffsetX(double x) { m_labelOffsetX = x; }
    double labelOffsetY() const { return m_labelOffsetY; }
    void setLabelOffsetY(double y) { m_labelOffsetY = y; }
    bool labelVisible() const { return m_labelVisible; }
    void setLabelVisible(bool v) { m_labelVisible = v; }
    
    // 可见性和交互
    bool connectionVisible() const { return m_connectionVisible; }
    void setConnectionVisible(bool v) { m_connectionVisible = v; }
    bool connectionSelectable() const { return m_connectionSelectable; }
    void setConnectionSelectable(bool s) { m_connectionSelectable = s; }
    bool connectionMovable() const { return m_connectionMovable; }
    void setConnectionMovable(bool m) { m_connectionMovable = m; }
    bool connectionEditable() const { return m_connectionEditable; }
    void setConnectionEditable(bool e) { m_connectionEditable = e; }
    bool showDirection() const { return m_showDirection; }
    void setShowDirection(bool s) { m_showDirection = s; }
    bool showEndpoints() const { return m_showEndpoints; }
    void setShowEndpoints(bool s) { m_showEndpoints = s; }
    double endpointSize() const { return m_endpointSize; }
    void setEndpointSize(double s) { m_endpointSize = s; }
    QString endpointColor() const { return m_endpointColor; }
    void setEndpointColor(const QString& c) { m_endpointColor = c; }
    
    // 工具方法
    QColor lineColorAsQColor() const;
    
    // 实现基类接口
    void loadFromMap(const QVariantMap& map) override;
    QVariantMap toMap() const override;
    QString metadataType() const override { return "ConnectionGraphic"; }

private:
    QString m_pathType = "Straight";
    QList<QPointF> m_pathPoints;
    QString m_routingStyle = "Manhattan";
    QString m_lineType = "Solid";
    QString m_lineColor = "#000000";
    double m_lineWidth = 1.0;
    double m_lineOpacity = 1.0;
    QString m_lineStyle = "Normal";
    QString m_sourceArrow = "None";
    QString m_targetArrow = "Arrow";
    double m_arrowSize = 8.0;
    QString m_arrowColor = "#000000";
    QString m_arrowFillColor = "#000000";
    QString m_sourcePort;
    QString m_targetPort;
    double m_sourceOffset = 0.0;
    double m_targetOffset = 0.0;
    QString m_labelText;
    QString m_labelPosition = "Center";
    double m_labelOffsetX = 0.0;
    double m_labelOffsetY = -10.0;
    bool m_labelVisible = true;
    bool m_connectionVisible = true;
    bool m_connectionSelectable = true;
    bool m_connectionMovable = false;
    bool m_connectionEditable = true;
    bool m_showDirection = true;
    bool m_showEndpoints = false;
    double m_endpointSize = 4.0;
    QString m_endpointColor = "#000000";
};

SYSML_NAMESPACE_END

Q_DECLARE_METATYPE(SysML::ConnectionGraphicMetadata*)

#endif // CONNECTIONGRAPHICMETADATA_H
