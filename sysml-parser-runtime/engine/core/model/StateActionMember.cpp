#include "StateActionMember.h"

SYSML_NAMESPACE_BEGIN

StateActionMember::StateActionMember(QObject* parent)
    : Element(parent)
    , m_hasActionKeyword(false)
{
}

void StateActionMember::setActionKind(const QString& kind)
{
    if (m_actionKind != kind) {
        m_actionKind = kind;
        emit actionKindChanged(kind);
    }
}

void StateActionMember::setActionName(const QString& name)
{
    if (m_actionName != name) {
        m_actionName = name;
        emit actionNameChanged(name);
    }
}

void StateActionMember::setHasActionKeyword(bool hasKeyword)
{
    if (m_hasActionKeyword != hasKeyword) {
        m_hasActionKeyword = hasKeyword;
        emit hasActionKeywordChanged(hasKeyword);
    }
}

SYSML_NAMESPACE_END


