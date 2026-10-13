#include "ViewpointDefinition.h"
#include "Element.h"

SYSML_NAMESPACE_BEGIN

ViewpointDefinition::ViewpointDefinition(QObject* parent)
    : Type(parent)
{
}

void ViewpointDefinition::addFramedConcern(Element* concern)
{
    if (concern && !m_framedConcerns.contains(concern)) {
        m_framedConcerns.append(concern);
        concern->setParent(this);
    }
}

void ViewpointDefinition::removeFramedConcern(Element* concern)
{
    m_framedConcerns.removeAll(concern);
}

SYSML_NAMESPACE_END


