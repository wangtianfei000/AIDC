#include "TransitionEffectPart.h"
#include "SendStatement.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

TransitionEffectPart::TransitionEffectPart(QObject* parent)
    : Element(parent)
{
}

void TransitionEffectPart::setActionType(const QString& type)
{
    if (m_actionType != type) {
        m_actionType = type;
        emit actionTypeChanged(type);
        emit elementChanged();
    }
}

void TransitionEffectPart::setSendStatement(SendStatement* stmt)
{
    if (m_sendStatement != stmt) {
        if (m_sendStatement) {
            m_sendStatement->setParent(nullptr);
        }
        m_sendStatement = stmt;
        if (m_sendStatement) {
            m_sendStatement->setParent(this);
        }
        emit sendStatementChanged(stmt);
        emit elementChanged();
    }
}

void TransitionEffectPart::setActionReference(const QString& ref)
{
    if (m_actionReference != ref) {
        m_actionReference = ref;
        emit actionReferenceChanged(ref);
        emit elementChanged();
    }
}

void TransitionEffectPart::setActionInvocation(const QString& invocation)
{
    if (m_actionInvocation != invocation) {
        m_actionInvocation = invocation;
        emit actionInvocationChanged(invocation);
        emit elementChanged();
    }
}

SYSML_NAMESPACE_END


