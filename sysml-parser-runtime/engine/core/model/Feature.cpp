#include "Feature.h"

SYSML_NAMESPACE_BEGIN

Feature::Feature(QObject* parent)
    : Element(parent)
{
}

void Feature::setIsAbstract(bool isAbstract)
{
    if (m_isAbstract != isAbstract) {
        m_isAbstract = isAbstract;
        emit isAbstractChanged(isAbstract);
    }
}

void Feature::setIsConst(bool isConst)
{
    if (m_isConst != isConst) {
        m_isConst = isConst;
        emit isConstChanged(isConst);
    }
}

void Feature::setIsDerived(bool isDerived)
{
    if (m_isDerived != isDerived) {
        m_isDerived = isDerived;
        emit isDerivedChanged(isDerived);
    }
}

void Feature::setDirection(Direction direction)
{
    if (m_direction != direction) {
        m_direction = direction;
        emit directionChanged(direction);
    }
}

void Feature::setIsRef(bool isRef)
{
    if (m_isRef != isRef) {
        m_isRef = isRef;
        emit isRefChanged(isRef);
    }
}

void Feature::setUnitExpression(const QString& unitExpression)
{
    if (m_unitExpression != unitExpression) {
        m_unitExpression = unitExpression;
        emit unitExpressionChanged(unitExpression);
    }
}

void Feature::setValueExpression(const QString& valueExpression)
{
    if (m_valueExpression != valueExpression) {
        m_valueExpression = valueExpression;
        emit valueExpressionChanged(valueExpression);
    }
}

void Feature::setIsSimplifiedAttribute(bool isSimplified)
{
    if (m_isSimplifiedAttribute != isSimplified) {
        m_isSimplifiedAttribute = isSimplified;
        emit isSimplifiedAttributeChanged(isSimplified);
    }
}

void Feature::setIsRedefines(bool isRedefines)
{
    if (m_isRedefines != isRedefines) {
        m_isRedefines = isRedefines;
        emit isRedefinesChanged(isRedefines);
    }
}

void Feature::setDefaultValue(const QString& defaultValue)
{
    if (m_defaultValue != defaultValue) {
        m_defaultValue = defaultValue;
        emit defaultValueChanged(defaultValue);
    }
}

SYSML_NAMESPACE_END



