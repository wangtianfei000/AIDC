#include "ItemUsage.h"

SYSML_NAMESPACE_BEGIN

ItemUsage::ItemUsage(QObject* parent)
    : Type(parent)
{
}

void ItemUsage::setIsDerived(bool isDerived)
{
    if (m_isDerived != isDerived) {
        m_isDerived = isDerived;
        emit isDerivedChanged(isDerived);
    }
}

void ItemUsage::setIsConstant(bool isConstant)
{
    if (m_isConstant != isConstant) {
        m_isConstant = isConstant;
        emit isConstantChanged(isConstant);
    }
}

void ItemUsage::setIsRef(bool isRef)
{
    if (m_isRef != isRef) {
        m_isRef = isRef;
        emit isRefChanged(isRef);
    }
}

void ItemUsage::setDirection(Direction direction)
{
    if (m_direction != direction) {
        m_direction = direction;
        emit directionChanged(direction);
    }
}

void ItemUsage::setPortionKind(PortionKind portionKind)
{
    if (m_portionKind != portionKind) {
        m_portionKind = portionKind;
        emit portionKindChanged(portionKind);
    }
}

SYSML_NAMESPACE_END

