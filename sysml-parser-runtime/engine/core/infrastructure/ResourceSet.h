#ifndef SYSML_RESOURCESET_H
#define SYSML_RESOURCESET_H

#include <QObject>
#include <QList>
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

class Resource;

/**
 * @brief ResourceSet 管理多个资源
 * 
 * 功能:
 * - 资源集合管理
 * - URI 解析
 */
class ResourceSet : public QObject {
    Q_OBJECT
    
public:
    explicit ResourceSet(QObject* parent = nullptr);
    ~ResourceSet();
    
    // 资源管理
    QList<Resource*> resources() const { return m_resources; }
    void addResource(Resource* resource);
    void removeResource(Resource* resource);
    void clear();
    
    // URI 解析
    Resource* getResource(const QString& uri) const;
    
signals:
    void resourceAdded(Resource* resource);
    void resourceRemoved(Resource* resource);
    
private:
    QList<Resource*> m_resources;
};

SYSML_NAMESPACE_END

#endif // SYSML_RESOURCESET_H



