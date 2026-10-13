#include "PortGraphicMetadata.h"
#include "../../../parser/src/annotation/AnnotationParser.h"
#include "../../../core/model/Annotation.h"

SYSML_NAMESPACE_BEGIN

PortGraphicMetadata::PortGraphicMetadata(QObject* parent)
    : BaseGraphicMetadata(parent)
{
}

QColor PortGraphicMetadata::portFillColorAsQColor() const {
    return QColor(m_portFillColor);
}

QColor PortGraphicMetadata::portBorderColorAsQColor() const {
    return QColor(m_portBorderColor);
}

void PortGraphicMetadata::loadFromMap(const QVariantMap& map) {
    if (map.contains("graphicX")) setGraphicX(map.value("graphicX").toDouble());
    if (map.contains("graphicY")) setGraphicY(map.value("graphicY").toDouble());
    if (map.contains("portSize")) setPortSize(map.value("portSize").toDouble());
    if (map.contains("portWidth")) setPortWidth(map.value("portWidth").toDouble());
    if (map.contains("portHeight")) setPortHeight(map.value("portHeight").toDouble());
    if (map.contains("portPosition")) setPortPosition(map.value("portPosition").toString());
    if (map.contains("portOffset")) setPortOffset(map.value("portOffset").toDouble());
    if (map.contains("portAlignment")) setPortAlignment(map.value("portAlignment").toString());
    if (map.contains("portDirection")) setPortDirection(map.value("portDirection").toString());
    if (map.contains("portAngle")) setPortAngle(map.value("portAngle").toDouble());
    if (map.contains("portShape")) setPortShape(map.value("portShape").toString());
    if (map.contains("portFillColor")) setPortFillColor(map.value("portFillColor").toString());
    if (map.contains("portFillOpacity")) setPortFillOpacity(map.value("portFillOpacity").toDouble());
    if (map.contains("portBorderColor")) setPortBorderColor(map.value("portBorderColor").toString());
    if (map.contains("portBorderWidth")) setPortBorderWidth(map.value("portBorderWidth").toDouble());
    if (map.contains("portBorderStyle")) setPortBorderStyle(map.value("portBorderStyle").toString());
    if (map.contains("portLabel")) setPortLabel(map.value("portLabel").toString());
    if (map.contains("labelDirection")) setLabelDirection(map.value("labelDirection").toString());
    if (map.contains("labelOffsetX")) setLabelOffsetX(map.value("labelOffsetX").toDouble());
    if (map.contains("labelOffsetY")) setLabelOffsetY(map.value("labelOffsetY").toDouble());
    if (map.contains("labelVisible")) setLabelVisible(map.value("labelVisible").toBool());
    if (map.contains("portVisible")) setPortVisible(map.value("portVisible").toBool());
    if (map.contains("portSelectable")) setPortSelectable(map.value("portSelectable").toBool());
    if (map.contains("portMovable")) setPortMovable(map.value("portMovable").toBool());
    if (map.contains("connectionPointX")) setConnectionPointX(map.value("connectionPointX").toDouble());
    if (map.contains("connectionPointY")) setConnectionPointY(map.value("connectionPointY").toDouble());
    if (map.contains("connectionAngle")) setConnectionAngle(map.value("connectionAngle").toDouble());
}

QVariantMap PortGraphicMetadata::toMap() const {
    QVariantMap map;
    map["graphicX"] = m_graphicX;
    map["graphicY"] = m_graphicY;
    map["portSize"] = m_portSize;
    map["portWidth"] = m_portWidth;
    map["portHeight"] = m_portHeight;
    map["portPosition"] = m_portPosition;
    map["portOffset"] = m_portOffset;
    map["portAlignment"] = m_portAlignment;
    map["portDirection"] = m_portDirection;
    map["portAngle"] = m_portAngle;
    map["portShape"] = m_portShape;
    map["portFillColor"] = m_portFillColor;
    map["portFillOpacity"] = m_portFillOpacity;
    map["portBorderColor"] = m_portBorderColor;
    map["portBorderWidth"] = m_portBorderWidth;
    map["portBorderStyle"] = m_portBorderStyle;
    map["portLabel"] = m_portLabel;
    map["labelDirection"] = m_labelDirection;
    map["labelOffsetX"] = m_labelOffsetX;
    map["labelOffsetY"] = m_labelOffsetY;
    map["labelVisible"] = m_labelVisible;
    map["portVisible"] = m_portVisible;
    map["portSelectable"] = m_portSelectable;
    map["portMovable"] = m_portMovable;
    map["connectionPointX"] = m_connectionPointX;
    map["connectionPointY"] = m_connectionPointY;
    map["connectionAngle"] = m_connectionAngle;
    return map;
}

void PortGraphicMetadata::saveToModel(Element* element) {
    qDebug() << "[PortGraphicMetadata] saveToModel: START";
    qDebug() << "[PortGraphicMetadata] saveToModel: element =" << (element ? "valid" : "null");
    
    if (!element) {
        qDebug() << "[PortGraphicMetadata] saveToModel: element is null, returning";
        return;
    }
    
    qDebug() << "[PortGraphicMetadata] saveToModel: Calling AnnotationParser::findAnnotation";
    // 查找或创建 Annotation
    Annotation* annotation = AnnotationParser::findAnnotation(element, "GraphicMetadata");
    qDebug() << "[PortGraphicMetadata] saveToModel: findAnnotation returned:" << (annotation ? "valid" : "null");
    
    if (!annotation) {
        qDebug() << "[PortGraphicMetadata] saveToModel: Creating new Annotation";
        annotation = new Annotation(element);
        qDebug() << "[PortGraphicMetadata] saveToModel: new Annotation created:" << (annotation ? "valid" : "null");
        
        if (!annotation) {
            qDebug() << "[PortGraphicMetadata] saveToModel: ERROR - failed to create annotation";
            return;
        }
        
        qDebug() << "[PortGraphicMetadata] saveToModel: Setting annotation name";
        annotation->setAnnotationName("GraphicMetadata");
        qDebug() << "[PortGraphicMetadata] saveToModel: Calling element->addAnnotation";
        element->addAnnotation(annotation);
        qDebug() << "[PortGraphicMetadata] saveToModel: element->addAnnotation completed";
    }
    
    if (!annotation) {
        qDebug() << "[PortGraphicMetadata] saveToModel: ERROR - annotation is null after creation";
        return;
    }
    
    qDebug() << "[PortGraphicMetadata] saveToModel: Converting metadata to map";
    // 将当前元数据值写入 Annotation
    QVariantMap map = toMap();
    qDebug() << "[PortGraphicMetadata] saveToModel: Map size:" << map.size();
    
    qDebug() << "[PortGraphicMetadata] saveToModel: Setting metadata assignments";
    int count = 0;
    for (auto it = map.begin(); it != map.end(); ++it) {
        qDebug() << "[PortGraphicMetadata] saveToModel: Setting" << it.key() << "=" << it.value();
        annotation->setMetadataAssignment(it.key(), it.value());
        count++;
    }
    qDebug() << "[PortGraphicMetadata] saveToModel: Set" << count << "metadata assignments";
    
    // 注意：Annotation::setMetadataAssignment 已经会调用 notifyChanged()，
    // 这会触发 Annotation 的 elementChanged() 信号，应该足够通知模型变化
    // 如果需要通知拥有这个 Annotation 的 Element，可能需要通过其他机制
    // （但 notifyChanged() 是 protected，无法直接调用）
    
    qDebug() << "[PortGraphicMetadata] saveToModel: END - SUCCESS";
}

SYSML_NAMESPACE_END
