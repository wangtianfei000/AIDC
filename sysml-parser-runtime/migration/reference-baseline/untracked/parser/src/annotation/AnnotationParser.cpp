#include "AnnotationParser.h"
#include "../../../core/model/Annotation.h"
#include "../../../ui/mid_windows/architecture/BaseGraphicMetadata.h"
#include "../../../ui/mid_windows/architecture/PartBasicViewMetadata.h"
#include "../../../ui/mid_windows/architecture/PartGraphicMetadata.h"
#include "../../../ui/mid_windows/architecture/PortGraphicMetadata.h"
#include "../../../ui/mid_windows/architecture/ConnectionGraphicMetadata.h"
#include <QDebug>

SYSML_NAMESPACE_BEGIN

BaseGraphicMetadata* AnnotationParser::parseGraphicMetadata(Element* element)
{
    if (!element) {
        return nullptr;
    }
    
    // 查找GraphicMetadata注解
    Annotation* annotation = findAnnotation(element, "GraphicMetadata");
    if (!annotation) {
        return nullptr;
    }
    
    // 提取元数据
    QVariantMap metadataMap = extractMetadata(annotation);
    if (metadataMap.isEmpty()) {
        return nullptr;
    }
    
    // 根据元素类型创建对应的GraphicMetadata子类
    QString elementType = element->elementType();
    BaseGraphicMetadata* metadata = nullptr;
    
    if (elementType == "PartDefinition") {
        // Part def：创建PartBasicViewMetadata
        metadata = new PartBasicViewMetadata();
    } else if (elementType == "PartUsage") {
        // 子Part：创建PartGraphicMetadata
        metadata = new PartGraphicMetadata();
    } else if (elementType == "PortUsage" || elementType == "PortDefinition") {
        // Port：创建PortGraphicMetadata
        metadata = new PortGraphicMetadata();
    } else if (elementType == "ConnectStatement") {
        // Connection：创建ConnectionGraphicMetadata
        metadata = new ConnectionGraphicMetadata();
    } else {
        // 未知类型，返回nullptr
        qWarning() << "[AnnotationParser] Unknown element type for GraphicMetadata:" << elementType;
        return nullptr;
    }
    
    // 从QVariantMap加载数据
    metadata->loadFromMap(metadataMap);
    
    return metadata;
}

Annotation* AnnotationParser::findAnnotation(Element* element, const QString& annotationName)
{
    if (!element) {
        return nullptr;
    }
    
    Element* annotationElement = element->findAnnotation(annotationName);
    if (annotationElement && annotationElement->elementType() == "Annotation") {
        return qobject_cast<Annotation*>(annotationElement);
    }
    
    return nullptr;
}

QVariantMap AnnotationParser::extractMetadata(Annotation* annotation)
{
    if (!annotation) {
        return QVariantMap();
    }
    
    return annotation->metadataAssignments();
}

QVariant AnnotationParser::getMetadataValue(Annotation* annotation, const QString& key)
{
    if (!annotation) {
        return QVariant();
    }
    
    return annotation->getMetadataAssignment(key);
}

SYSML_NAMESPACE_END
