#include "RequireStatement.h"

SYSML_NAMESPACE_BEGIN

RequireStatement::RequireStatement(QObject* parent)
    : Element(parent)
{
}

void RequireStatement::setExpression(const QString& expression)
{
    if (m_expression != expression) {
        m_expression = expression;
        emit expressionChanged(expression);
    }
}

void RequireStatement::setHasConstraintKeyword(bool hasConstraint)
{
    if (m_hasConstraintKeyword != hasConstraint) {
        m_hasConstraintKeyword = hasConstraint;
        emit hasConstraintKeywordChanged(hasConstraint);
    }
}

SYSML_NAMESPACE_END

