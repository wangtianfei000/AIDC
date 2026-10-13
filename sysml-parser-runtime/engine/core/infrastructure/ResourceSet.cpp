#include "ResourceSet.h"
#include "Resource.h"

SYSML_NAMESPACE_BEGIN

ResourceSet::ResourceSet(QObject* parent)
    : QObject(parent)
{
}

ResourceSet::~ResourceSet()
{
    clear();
}

void ResourceSet::addResource(Resource* resource)
{
    if (!resource) {
        return;
    }
    
    if (!m_resources.contains(resource)) {
        m_resources.append(resource);
        resource->setParent(this);
        emit resourceAdded(resource);
    }
}

void ResourceSet::removeResource(Resource* resource)
{
    if (m_resources.removeOne(resource)) {
        resource->setParent(nullptr);
        emit resourceRemoved(resource);
    }
}

void ResourceSet::clear()
{
    for (Resource* resource : m_resources) {
        resource->setParent(nullptr);
    }
    m_resources.clear();
}

Resource* ResourceSet::getResource(const QString& uri) const
{
    for (Resource* resource : m_resources) {
        if (resource->uri() == uri) {
            return resource;
        }
    }
    return nullptr;
}

SYSML_NAMESPACE_END



