#include "Conjugation.h"
#include "Type.h"

SYSML_NAMESPACE_BEGIN

Conjugation::Conjugation(QObject* parent)
    : Relationship(parent)
{
}

Type* Conjugation::conjugatedType() const
{
    return m_conjugatedType;
}

void Conjugation::setConjugatedType(Type* type)
{
    if (m_conjugatedType != type) {
        m_conjugatedType = type;
        emit conjugatedTypeChanged(type);
    }
}

Type* Conjugation::originalType() const
{
    return m_originalType;
}

void Conjugation::setOriginalType(Type* type)
{
    if (m_originalType != type) {
        m_originalType = type;
        emit originalTypeChanged(type);
    }
}

SYSML_NAMESPACE_END





