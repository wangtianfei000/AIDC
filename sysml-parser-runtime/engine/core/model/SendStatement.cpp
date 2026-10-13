#include "SendStatement.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

SendStatement::SendStatement(QObject* parent)
    : Element(parent)
{
}

void SendStatement::setHasNew(bool hasNew)
{
    if (m_hasNew != hasNew) {
        m_hasNew = hasNew;
        emit hasNewChanged(hasNew);
        emit elementChanged();
    }
}

void SendStatement::setActionInvocation(const QString& invocation)
{
    if (m_actionInvocation != invocation) {
        m_actionInvocation = invocation;
        emit actionInvocationChanged(invocation);
        emit elementChanged();
    }
}

void SendStatement::setTarget(const QString& tgt)
{
    if (m_target != tgt) {
        m_target = tgt;
        emit targetChanged(tgt);
        emit elementChanged();
    }
}

SYSML_NAMESPACE_END


