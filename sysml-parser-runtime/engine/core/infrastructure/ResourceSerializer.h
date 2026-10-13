#ifndef SYSML_RESOURCESERIALIZER_H
#define SYSML_RESOURCESERIALIZER_H

#include <QJsonObject>
#include <QJsonDocument>
#include <QString>
#include <QHash>
#include <functional>
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

class Resource;
class Element;

/**
 * @brief ResourceSerializer 负责资源的序列化和反序列化
 * 
 * 支持格式: JSON
 */
class ResourceSerializer {
public:
    // 资源序列化
    static QJsonObject serializeResource(Resource* resource);
    static bool deserializeResource(Resource* resource, const QJsonObject& json);
    
    // 元素序列化
    static QJsonObject serializeElement(Element* element);
    static Element* deserializeElement(const QJsonObject& json, QObject* parent = nullptr);
    
    // 文件操作
    static bool saveToFile(Resource* resource, const QString& filePath);
    static bool loadFromFile(Resource* resource, const QString& filePath);
    
    // 类型注册
    using ElementFactory = std::function<Element*(const QJsonObject&)>;
    static void registerFactory(const QString& typeName, ElementFactory factory);
    
private:
    // 类型注册表
    static QHash<QString, ElementFactory>& factories();
    static void registerDefaultFactories();
};

SYSML_NAMESPACE_END

#endif // SYSML_RESOURCESERIALIZER_H

