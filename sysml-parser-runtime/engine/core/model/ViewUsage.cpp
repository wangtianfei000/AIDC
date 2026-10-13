#include "ViewUsage.h"
#include "ViewDefinition.h"
#include "Element.h"

SYSML_NAMESPACE_BEGIN

ViewUsage::ViewUsage(QObject* parent)
    : Type(parent)
{
}

void ViewUsage::addExposedElement(Element* element)
{
    if (element && !m_exposedElements.contains(element)) {
        m_exposedElements.append(element);
    }
}

void ViewUsage::removeExposedElement(Element* element)
{
    m_exposedElements.removeAll(element);
}

SYSML_NAMESPACE_END


