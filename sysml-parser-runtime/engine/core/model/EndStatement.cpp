#include "EndStatement.h"

SYSML_NAMESPACE_BEGIN

EndStatement::EndStatement(QObject* parent)
    : Element(parent)
{
}

void EndStatement::setTag(const QString& tag)
{
    if (m_tag != tag) {
        m_tag = tag;
        emit tagChanged(tag);
    }
}

void EndStatement::setTypeName(const QString& typeName)
{
    if (m_typeName != typeName) {
        m_typeName = typeName;
        emit typeNameChanged(typeName);
    }
}

void EndStatement::setIsNegated(bool isNegated)
{
    if (m_isNegated != isNegated) {
        m_isNegated = isNegated;
        emit isNegatedChanged(isNegated);
    }
}

void EndStatement::setUsesReferences(bool usesReferences)
{
    if (m_usesReferences != usesReferences) {
        m_usesReferences = usesReferences;
        emit usesReferencesChanged(usesReferences);
    }
}

SYSML_NAMESPACE_END


