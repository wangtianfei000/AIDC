#ifndef SYSML_RESOURCEFACTORY_H
#define SYSML_RESOURCEFACTORY_H

#include <QString>
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

class Resource;

/**
 * @brief ResourceFactory 创建资源实例
 */
class ResourceFactory {
public:
    static Resource* createResource(const QString& filePath = QString());
    static Resource* createResourceForExtension(const QString& filePath);
    
private:
    static QString extensionForFile(const QString& filePath);
};

SYSML_NAMESPACE_END

#endif // SYSML_RESOURCEFACTORY_H



