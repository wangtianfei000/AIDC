#include "Import.h"
#include "Namespace.h"

SYSML_NAMESPACE_BEGIN

Import::Import(QObject* parent)
    : Relationship(parent)
{
}

Namespace* Import::importedNamespace() const
{
    return m_importedNamespace;
}

void Import::setImportedNamespace(Namespace* namespace_)
{
    if (m_importedNamespace != namespace_) {
        m_importedNamespace = namespace_;
        emit importedNamespaceChanged(namespace_);
    }
}

SYSML_NAMESPACE_END



