#include "PartUsage.h"

SYSML_NAMESPACE_BEGIN

PartUsage::PartUsage(QObject* parent)
    : Type(parent)
{
}

void PartUsage::setIsDerived(bool isDerived)
{
    if (m_isDerived != isDerived) {
        m_isDerived = isDerived;
        emit isDerivedChanged(isDerived);
    }
}

void PartUsage::setIsConstant(bool isConstant)
{
    if (m_isConstant != isConstant) {
        m_isConstant = isConstant;
        emit isConstantChanged(isConstant);
    }
}

void PartUsage::setIsRef(bool isRef)
{
    if (m_isRef != isRef) {
        m_isRef = isRef;
        emit isRefChanged(isRef);
    }
}

void PartUsage::setDirection(Direction direction)
{
    if (m_direction != direction) {
        m_direction = direction;
        emit directionChanged(direction);
    }
}

void PartUsage::setPortionKind(PortionKind portionKind)
{
    if (m_portionKind != portionKind) {
        m_portionKind = portionKind;
        emit portionKindChanged(portionKind);
    }
}

void PartUsage::setMemberKind(const QString& kind)
{
    if (m_memberKind != kind) {
        m_memberKind = kind;
        emit memberKindChanged(kind);
    }
}

SYSML_NAMESPACE_END



