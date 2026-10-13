#include "ExhibitStatement.h"

SYSML_NAMESPACE_BEGIN

ExhibitStatement::ExhibitStatement(QObject* parent)
    : Element(parent)
    , m_stateMachineKind("")
{
}

void ExhibitStatement::setStateMachineKind(const QString& kind)
{
    if (m_stateMachineKind != kind) {
        m_stateMachineKind = kind;
        emit stateMachineKindChanged(kind);
    }
}

SYSML_NAMESPACE_END


