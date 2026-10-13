#include "Membership.h"
#include "Element.h"

SYSML_NAMESPACE_BEGIN

Membership::Membership(QObject* parent)
    : Relationship(parent)
{
}

Element* Membership::memberElement() const
{
    return m_memberElement;
}

void Membership::setMemberElement(Element* element)
{
    if (m_memberElement != element) {
        m_memberElement = element;
        emit memberElementChanged(element);
    }
}

SYSML_NAMESPACE_END



