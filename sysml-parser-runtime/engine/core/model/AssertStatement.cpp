#include "AssertStatement.h"

SYSML_NAMESPACE_BEGIN

AssertStatement::AssertStatement(QObject* parent)
    : Element(parent)
{
}

void AssertStatement::setExpression(const QString& expression)
{
    if (m_expression != expression) {
        m_expression = expression;
        emit expressionChanged(expression);
    }
}

SYSML_NAMESPACE_END



