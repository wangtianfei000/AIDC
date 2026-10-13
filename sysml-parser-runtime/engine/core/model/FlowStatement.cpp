#include "FlowStatement.h"

SYSML_NAMESPACE_BEGIN

FlowStatement::FlowStatement(QObject* parent)
    : Element(parent)
{
}

void FlowStatement::setFromEnd(const QString& fromEnd)
{
    if (m_fromEnd != fromEnd) {
        m_fromEnd = fromEnd;
        emit fromEndChanged(fromEnd);
    }
}

void FlowStatement::setToEnd(const QString& toEnd)
{
    if (m_toEnd != toEnd) {
        m_toEnd = toEnd;
        emit toEndChanged(toEnd);
    }
}

void FlowStatement::setPayload(const QString& payload)
{
    if (m_payload != payload) {
        m_payload = payload;
        emit payloadChanged(payload);
    }
}

void FlowStatement::setFlowName(const QString& flowName)
{
    if (m_flowName != flowName) {
        m_flowName = flowName;
        emit flowNameChanged(flowName);
    }
}

void FlowStatement::setFlowType(const QString& flowType)
{
    if (m_flowType != flowType) {
        m_flowType = flowType;
        emit flowTypeChanged(flowType);
    }
}

void FlowStatement::setHasFromKeyword(bool hasFromKeyword)
{
    if (m_hasFromKeyword != hasFromKeyword) {
        m_hasFromKeyword = hasFromKeyword;
        emit hasFromKeywordChanged(hasFromKeyword);
    }
}

SYSML_NAMESPACE_END

