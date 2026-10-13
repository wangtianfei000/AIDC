#include "PartGraphicMetadata.h"
#include "../../../parser/src/annotation/AnnotationParser.h"
#include "../../../core/model/Annotation.h"

SYSML_NAMESPACE_BEGIN

PartGraphicMetadata::PartGraphicMetadata(QObject* parent)
    : BaseGraphicMetadata(parent)
{
}

QRectF PartGraphicMetadata::boundingRect() const {
    if (m_graphicWidth < 0 || m_graphicHeight < 0) {
        return QRectF(m_graphicX, m_graphicY, 100, 80);  // 默认大小
    }
    return QRectF(m_graphicX, m_graphicY, m_graphicWidth, m_graphicHeight);
}

QColor PartGraphicMetadata::fillColorAsQColor() const {
    return QColor(m_fillColor);
}

QColor PartGraphicMetadata::borderColorAsQColor() const {
    return QColor(m_borderColor);
}

void PartGraphicMetadata::loadFromMap(const QVariantMap& map) {
    if (map.contains("graphicX")) setGraphicX(map.value("graphicX").toDouble());
    if (map.contains("graphicY")) setGraphicY(map.value("graphicY").toDouble());
    if (map.contains("graphicWidth")) setGraphicWidth(map.value("graphicWidth").toDouble());
    if (map.contains("graphicHeight")) setGraphicHeight(map.value("graphicHeight").toDouble());
    if (map.contains("graphicRotation")) setGraphicRotation(map.value("graphicRotation").toDouble());
    if (map.contains("graphicZOrder")) setGraphicZOrder(map.value("graphicZOrder").toInt());
    if (map.contains("shape")) setShape(map.value("shape").toString());
    if (map.contains("shapeRadius")) setShapeRadius(map.value("shapeRadius").toDouble());
    if (map.contains("fillColor")) setFillColor(map.value("fillColor").toString());
    if (map.contains("fillOpacity")) setFillOpacity(map.value("fillOpacity").toDouble());
    if (map.contains("borderColor")) setBorderColor(map.value("borderColor").toString());
    if (map.contains("borderWidth")) setBorderWidth(map.value("borderWidth").toDouble());
    if (map.contains("borderStyle")) setBorderStyle(map.value("borderStyle").toString());
    if (map.contains("labelText")) setLabelText(map.value("labelText").toString());
    if (map.contains("labelFont")) setLabelFont(map.value("labelFont").toString());
    if (map.contains("labelColor")) setLabelColor(map.value("labelColor").toString());
    if (map.contains("labelPosition")) setLabelPosition(map.value("labelPosition").toString());
    if (map.contains("labelVisible")) setLabelVisible(map.value("labelVisible").toBool());
    if (map.contains("visible")) setVisible(map.value("visible").toBool());
    if (map.contains("selectable")) setSelectable(map.value("selectable").toBool());
    if (map.contains("movable")) setMovable(map.value("movable").toBool());
    if (map.contains("resizable")) setResizable(map.value("resizable").toBool());
    if (map.contains("showChildNestedContent")) {
        setShowChildNestedContent(map.value("showChildNestedContent").toBool());
    }
}

QVariantMap PartGraphicMetadata::toMap() const {
    QVariantMap map;
    map["graphicX"] = m_graphicX;
    map["graphicY"] = m_graphicY;
    map["graphicWidth"] = m_graphicWidth;
    map["graphicHeight"] = m_graphicHeight;
    map["graphicRotation"] = m_graphicRotation;
    map["graphicZOrder"] = m_graphicZOrder;
    map["shape"] = m_shape;
    map["shapeRadius"] = m_shapeRadius;
    map["fillColor"] = m_fillColor;
    map["fillOpacity"] = m_fillOpacity;
    map["borderColor"] = m_borderColor;
    map["borderWidth"] = m_borderWidth;
    map["borderStyle"] = m_borderStyle;
    map["labelText"] = m_labelText;
    map["labelFont"] = m_labelFont;
    map["labelColor"] = m_labelColor;
    map["labelPosition"] = m_labelPosition;
    map["labelVisible"] = m_labelVisible;
    map["visible"] = m_visible;
    map["selectable"] = m_selectable;
    map["movable"] = m_movable;
    map["resizable"] = m_resizable;
    map["showChildNestedContent"] = m_showChildNestedContent;
    return map;
}

void PartGraphicMetadata::saveToModel(Element* element) {
    qDebug() << "[PartGraphicMetadata] saveToModel: START";
    qDebug() << "[PartGraphicMetadata] saveToModel: element =" << (element ? "valid" : "null");
    
    if (!element) {
        qDebug() << "[PartGraphicMetadata] saveToModel: element is null, returning";
        return;
    }
    
    qDebug() << "[PartGraphicMetadata] saveToModel: Calling AnnotationParser::findAnnotation";
    // 查找或创建 Annotation
    Annotation* annotation = AnnotationParser::findAnnotation(element, "GraphicMetadata");
    qDebug() << "[PartGraphicMetadata] saveToModel: findAnnotation returned:" << (annotation ? "valid" : "null");
    
    if (!annotation) {
        qDebug() << "[PartGraphicMetadata] saveToModel: Creating new Annotation";
        annotation = new Annotation(element);
        qDebug() << "[PartGraphicMetadata] saveToModel: new Annotation created:" << (annotation ? "valid" : "null");
        
        if (!annotation) {
            qDebug() << "[PartGraphicMetadata] saveToModel: ERROR - failed to create annotation";
            return;
        }
        
        qDebug() << "[PartGraphicMetadata] saveToModel: Setting annotation name";
        annotation->setAnnotationName("GraphicMetadata");
        qDebug() << "[PartGraphicMetadata] saveToModel: Calling element->addAnnotation";
        element->addAnnotation(annotation);
        qDebug() << "[PartGraphicMetadata] saveToModel: element->addAnnotation completed";
    }
    
    if (!annotation) {
        qDebug() << "[PartGraphicMetadata] saveToModel: ERROR - annotation is null after creation";
        return;
    }
    
    qDebug() << "[PartGraphicMetadata] saveToModel: Converting metadata to map";
    // 将当前元数据值写入 Annotation
    QVariantMap map = toMap();
    
    // 对于 PartDefinition，位置应该始终为 (0,0)，不保存位置属性
    // 因为 PartDefinition 的外边框相当于画布，位置是固定的，保存这些属性没有意义
    bool isPartDefinition = (element && element->elementType() == "PartDefinition");
    if (isPartDefinition) {
        map.remove("graphicX");
        map.remove("graphicY");
        qDebug() << "[PartGraphicMetadata] saveToModel: PartDefinition detected, removing graphicX/Y from save";
    }
    
    qDebug() << "[PartGraphicMetadata] saveToModel: Map size:" << map.size();
    
    qDebug() << "[PartGraphicMetadata] saveToModel: Setting metadata assignments";
    int count = 0;
    for (auto it = map.begin(); it != map.end(); ++it) {
        qDebug() << "[PartGraphicMetadata] saveToModel: Setting" << it.key() << "=" << it.value();
        annotation->setMetadataAssignment(it.key(), it.value());
        count++;
    }
    qDebug() << "[PartGraphicMetadata] saveToModel: Set" << count << "metadata assignments";
    
    // 验证保存后的值
    qDebug() << "[PartGraphicMetadata] saveToModel: Verifying saved values";
    QVariantMap savedMap = annotation->metadataAssignments();
    qDebug() << "[PartGraphicMetadata] saveToModel: Saved map size:" << savedMap.size();
    if (savedMap.contains("graphicWidth")) {
        qDebug() << "[PartGraphicMetadata] saveToModel: Verified graphicWidth =" << savedMap.value("graphicWidth");
    }
    if (savedMap.contains("graphicHeight")) {
        qDebug() << "[PartGraphicMetadata] saveToModel: Verified graphicHeight =" << savedMap.value("graphicHeight");
    }
    
    // 注意：Annotation::setMetadataAssignment 已经会调用 notifyChanged()，
    // 这会触发 Annotation 的 elementChanged() 信号，应该足够通知模型变化
    // 如果需要通知拥有这个 Annotation 的 Element，可能需要通过其他机制
    // （但 notifyChanged() 是 protected，无法直接调用）
    
    qDebug() << "[PartGraphicMetadata] saveToModel: END - SUCCESS";
}

SYSML_NAMESPACE_END
