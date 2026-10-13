#include "TransitionGuardPart.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

TransitionGuardPart::TransitionGuardPart(QObject* parent)
    : Element(parent)
{
}

void TransitionGuardPart::setCondition(const QString& cond)
{
    if (m_condition != cond) {
        m_condition = cond;
        emit conditionChanged(cond);
        emit elementChanged();
    }
}

SYSML_NAMESPACE_END


