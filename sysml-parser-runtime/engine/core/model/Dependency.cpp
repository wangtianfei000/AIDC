#include "Dependency.h"
#include "Element.h"

SYSML_NAMESPACE_BEGIN

Dependency::Dependency(QObject* parent)
    : Relationship(parent)
{
}

Element* Dependency::clientElement() const
{
    return m_clientElement;
}

void Dependency::setClientElement(Element* element)
{
    if (m_clientElement != element) {
        m_clientElement = element;
        emit clientElementChanged(element);
    }
}

Element* Dependency::supplierElement() const
{
    return m_supplierElement;
}

void Dependency::setSupplierElement(Element* element)
{
    if (m_supplierElement != element) {
        m_supplierElement = element;
        emit supplierElementChanged(element);
    }
}

SYSML_NAMESPACE_END





