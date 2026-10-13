#include "BindStatement.h"

SYSML_NAMESPACE_BEGIN

BindStatement::BindStatement(QObject* parent)
    : Element(parent)
{
}

void BindStatement::setSourceEnd(const QString& sourceEnd)
{
    if (m_sourceEnd != sourceEnd) {
        m_sourceEnd = sourceEnd;
        emit sourceEndChanged(sourceEnd);
    }
}

void BindStatement::setTargetEnd(const QString& targetEnd)
{
    if (m_targetEnd != targetEnd) {
        m_targetEnd = targetEnd;
        emit targetEndChanged(targetEnd);
    }
}

SYSML_NAMESPACE_END




