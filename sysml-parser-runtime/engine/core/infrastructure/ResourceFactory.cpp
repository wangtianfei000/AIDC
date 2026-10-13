#include "ResourceFactory.h"
#include "Resource.h"
#include <QFileInfo>

SYSML_NAMESPACE_BEGIN

Resource* ResourceFactory::createResource(const QString& filePath)
{
    return new Resource(filePath);
}

Resource* ResourceFactory::createResourceForExtension(const QString& filePath)
{
    QString ext = extensionForFile(filePath);
    
    // 根据扩展名创建不同类型的资源
    // 目前只支持 JSON
    if (ext == "json") {
        return new Resource(filePath);
    }
    
    return new Resource(filePath);
}

QString ResourceFactory::extensionForFile(const QString& filePath)
{
    QFileInfo info(filePath);
    return info.suffix().toLower();
}

SYSML_NAMESPACE_END



