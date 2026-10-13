#include "NamespaceImport.h"

SYSML_NAMESPACE_BEGIN

NamespaceImport::NamespaceImport(QObject* parent)
    : Import(parent)
{
}

void NamespaceImport::setVisibility(Visibility visibility)
{
    if (m_visibility != visibility) {
        m_visibility = visibility;
        emit visibilityChanged(visibility);
    }
}

void NamespaceImport::setIsAll(bool isAll)
{
    if (m_isAll != isAll) {
        m_isAll = isAll;
        emit isAllChanged(isAll);
    }
}

void NamespaceImport::setIsWildcard(bool isWildcard)
{
    if (m_isWildcard != isWildcard) {
        m_isWildcard = isWildcard;
        emit isWildcardChanged(isWildcard);
    }
}

void NamespaceImport::setIsRecursive(bool isRecursive)
{
    if (m_isRecursive != isRecursive) {
        m_isRecursive = isRecursive;
        emit isRecursiveChanged(isRecursive);
    }
}

SYSML_NAMESPACE_END



