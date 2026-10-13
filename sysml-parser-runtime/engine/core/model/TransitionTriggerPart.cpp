#include "TransitionTriggerPart.h"
#include "AcceptClause.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

TransitionTriggerPart::TransitionTriggerPart(QObject* parent)
    : Element(parent)
{
}

void TransitionTriggerPart::setAcceptClause(AcceptClause* clause)
{
    if (m_acceptClause != clause) {
        if (m_acceptClause) {
            m_acceptClause->setParent(nullptr);
        }
        m_acceptClause = clause;
        if (m_acceptClause) {
            m_acceptClause->setParent(this);
        }
        emit acceptClauseChanged(clause);
        emit elementChanged();
    }
}

SYSML_NAMESPACE_END


