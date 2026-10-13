#include "ConnectionGraphicMetadata.h"
#include <QVariantList>

SYSML_NAMESPACE_BEGIN

ConnectionGraphicMetadata::ConnectionGraphicMetadata(QObject* parent)
    : BaseGraphicMetadata(parent)
{
}

QColor ConnectionGraphicMetadata::lineColorAsQColor() const {
    return QColor(m_lineColor);
}

void ConnectionGraphicMetadata::loadFromMap(const QVariantMap& map) {
    if (map.contains("pathType")) setPathType(map.value("pathType").toString());
    if (map.contains("pathPoints")) {
        // 解析路径点列表
        QVariantList pointsList = map.value("pathPoints").toList();
        QList<QPointF> points;
        for (const QVariant& pointVar : pointsList) {
            QVariantMap pointMap = pointVar.toMap();
            points.append(QPointF(
                pointMap.value("x").toDouble(),
                pointMap.value("y").toDouble()
            ));
        }
        setPathPoints(points);
    }
    if (map.contains("routingStyle")) setRoutingStyle(map.value("routingStyle").toString());
    if (map.contains("lineType")) setLineType(map.value("lineType").toString());
    if (map.contains("lineColor")) setLineColor(map.value("lineColor").toString());
    if (map.contains("lineWidth")) setLineWidth(map.value("lineWidth").toDouble());
    if (map.contains("lineOpacity")) setLineOpacity(map.value("lineOpacity").toDouble());
    if (map.contains("lineStyle")) setLineStyle(map.value("lineStyle").toString());
    if (map.contains("sourceArrow")) setSourceArrow(map.value("sourceArrow").toString());
    if (map.contains("targetArrow")) setTargetArrow(map.value("targetArrow").toString());
    if (map.contains("arrowSize")) setArrowSize(map.value("arrowSize").toDouble());
    if (map.contains("arrowColor")) setArrowColor(map.value("arrowColor").toString());
    if (map.contains("arrowFillColor")) setArrowFillColor(map.value("arrowFillColor").toString());
    if (map.contains("sourcePort")) setSourcePort(map.value("sourcePort").toString());
    if (map.contains("targetPort")) setTargetPort(map.value("targetPort").toString());
    if (map.contains("sourceOffset")) setSourceOffset(map.value("sourceOffset").toDouble());
    if (map.contains("targetOffset")) setTargetOffset(map.value("targetOffset").toDouble());
    if (map.contains("labelText")) setLabelText(map.value("labelText").toString());
    if (map.contains("labelPosition")) setLabelPosition(map.value("labelPosition").toString());
    if (map.contains("labelOffsetX")) setLabelOffsetX(map.value("labelOffsetX").toDouble());
    if (map.contains("labelOffsetY")) setLabelOffsetY(map.value("labelOffsetY").toDouble());
    if (map.contains("labelVisible")) setLabelVisible(map.value("labelVisible").toBool());
    if (map.contains("connectionVisible")) setConnectionVisible(map.value("connectionVisible").toBool());
    if (map.contains("connectionSelectable")) setConnectionSelectable(map.value("connectionSelectable").toBool());
    if (map.contains("connectionMovable")) setConnectionMovable(map.value("connectionMovable").toBool());
    if (map.contains("connectionEditable")) setConnectionEditable(map.value("connectionEditable").toBool());
    if (map.contains("showDirection")) setShowDirection(map.value("showDirection").toBool());
    if (map.contains("showEndpoints")) setShowEndpoints(map.value("showEndpoints").toBool());
    if (map.contains("endpointSize")) setEndpointSize(map.value("endpointSize").toDouble());
    if (map.contains("endpointColor")) setEndpointColor(map.value("endpointColor").toString());
}

QVariantMap ConnectionGraphicMetadata::toMap() const {
    QVariantMap map;
    map["pathType"] = m_pathType;
    // 序列化路径点列表
    QVariantList pointsList;
    for (const QPointF& point : m_pathPoints) {
        QVariantMap pointMap;
        pointMap["x"] = point.x();
        pointMap["y"] = point.y();
        pointsList.append(pointMap);
    }
    map["pathPoints"] = pointsList;
    map["routingStyle"] = m_routingStyle;
    map["lineType"] = m_lineType;
    map["lineColor"] = m_lineColor;
    map["lineWidth"] = m_lineWidth;
    map["lineOpacity"] = m_lineOpacity;
    map["lineStyle"] = m_lineStyle;
    map["sourceArrow"] = m_sourceArrow;
    map["targetArrow"] = m_targetArrow;
    map["arrowSize"] = m_arrowSize;
    map["arrowColor"] = m_arrowColor;
    map["arrowFillColor"] = m_arrowFillColor;
    map["sourcePort"] = m_sourcePort;
    map["targetPort"] = m_targetPort;
    map["sourceOffset"] = m_sourceOffset;
    map["targetOffset"] = m_targetOffset;
    map["labelText"] = m_labelText;
    map["labelPosition"] = m_labelPosition;
    map["labelOffsetX"] = m_labelOffsetX;
    map["labelOffsetY"] = m_labelOffsetY;
    map["labelVisible"] = m_labelVisible;
    map["connectionVisible"] = m_connectionVisible;
    map["connectionSelectable"] = m_connectionSelectable;
    map["connectionMovable"] = m_connectionMovable;
    map["connectionEditable"] = m_connectionEditable;
    map["showDirection"] = m_showDirection;
    map["showEndpoints"] = m_showEndpoints;
    map["endpointSize"] = m_endpointSize;
    map["endpointColor"] = m_endpointColor;
    return map;
}

SYSML_NAMESPACE_END
