#include "MembershipImport.h"
#include "Membership.h"

SYSML_NAMESPACE_BEGIN

MembershipImport::MembershipImport(QObject* parent)
    : Import(parent)
{
}

Membership* MembershipImport::importedMembership() const
{
    return m_importedMembership;
}

void MembershipImport::setImportedMembership(Membership* membership)
{
    if (m_importedMembership != membership) {
        m_importedMembership = membership;
        emit importedMembershipChanged(membership);
    }
}

void MembershipImport::setVisibility(Visibility visibility)
{
    if (m_visibility != visibility) {
        m_visibility = visibility;
        emit visibilityChanged(visibility);
    }
}

void MembershipImport::setIsRecursive(bool isRecursive)
{
    if (m_isRecursive != isRecursive) {
        m_isRecursive = isRecursive;
        emit isRecursiveChanged(isRecursive);
    }
}

SYSML_NAMESPACE_END



