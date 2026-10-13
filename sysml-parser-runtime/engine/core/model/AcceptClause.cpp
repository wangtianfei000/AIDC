#include "AcceptClause.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

AcceptClause::AcceptClause(QObject* parent)
    : Element(parent)
{
}

void AcceptClause::setAcceptType(const QString& type)
{
    if (m_acceptType != type) {
        m_acceptType = type;
        emit acceptTypeChanged(type);
        emit elementChanged();
    }
}

void AcceptClause::setUsageDeclaration(const QString& decl)
{
    if (m_usageDeclaration != decl) {
        m_usageDeclaration = decl;
        emit usageDeclarationChanged(decl);
        emit elementChanged();
    }
}

void AcceptClause::setViaPort(const QString& port)
{
    if (m_viaPort != port) {
        m_viaPort = port;
        emit viaPortChanged(port);
        emit elementChanged();
    }
}

void AcceptClause::setCondition(const QString& cond)
{
    if (m_condition != cond) {
        m_condition = cond;
        emit conditionChanged(cond);
        emit elementChanged();
    }
}

SYSML_NAMESPACE_END


