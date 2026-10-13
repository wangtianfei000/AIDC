#ifndef SYSML_ANNOTATIONPARSER_H
#define SYSML_ANNOTATIONPARSER_H

#include "../../../core/model/Element.h"
#include "../../../core/model/Annotation.h"
#include "../../../core/common/Global.h"
#include <QVariant>

SYSML_NAMESPACE_BEGIN

// 前向声明
class BaseGraphicMetadata;
class PartBasicViewMetadata;
class PartGraphicMetadata;
class PortGraphicMetadata;
class ConnectionGraphicMetadata;

/**
 * @brief AnnotationParser 注解解析器
 * 
 * 负责解析Annotation中的GraphicMetadata等注解信息
 * 根据元素类型返回对应的GraphicMetadata子类
 */
class AnnotationParser {
public:
    /**
     * @brief 解析元素的GraphicMetadata注解
     * @param element 要解析的元素
     * @return BaseGraphicMetadata对象（具体子类），如果不存在则返回nullptr
     */
    static BaseGraphicMetadata* parseGraphicMetadata(Element* element);
    
    /**
     * @brief 查找指定名称的注解
     * @param element 要查找的元素
     * @param annotationName 注解名称（如 "GraphicMetadata"）
     * @return Annotation对象，如果不存在则返回nullptr
     */
    static Annotation* findAnnotation(Element* element, const QString& annotationName);
    
    /**
     * @brief 从Annotation中提取元数据
     * @param annotation 注解对象
     * @return QVariantMap，包含所有元数据赋值
     */
    static QVariantMap extractMetadata(Annotation* annotation);
    
    /**
     * @brief 从Annotation中获取指定的元数据值
     * @param annotation 注解对象
     * @param key 元数据键
     * @return QVariant值，如果不存在则返回无效的QVariant
     */
    static QVariant getMetadataValue(Annotation* annotation, const QString& key);
    
private:
    AnnotationParser() = default;  // 静态类，不允许实例化
};

SYSML_NAMESPACE_END

#endif // SYSML_ANNOTATIONPARSER_H
