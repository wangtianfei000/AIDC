#include "Subclassification.h"
#include "Type.h"

SYSML_NAMESPACE_BEGIN

Subclassification::Subclassification(QObject* parent)
    : Relationship(parent)
{
}

Type* Subclassification::subtype() const
{
    return m_subtype;
}

void Subclassification::setSubtype(Type* type)
{
    if (m_subtype != type) {
        m_subtype = type;
        emit subtypeChanged(type);
    }
}

Type* Subclassification::supertype() const
{
    return m_supertype;
}

void Subclassification::setSupertype(Type* type)
{
    if (m_supertype != type) {
        m_supertype = type;
        emit supertypeChanged(type);
    }
}

SYSML_NAMESPACE_END



