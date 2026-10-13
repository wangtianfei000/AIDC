#include "ConstraintStatement.h"

SYSML_NAMESPACE_BEGIN

ConstraintStatement::ConstraintStatement(QObject* parent)
    : Element(parent)
{
}

void ConstraintStatement::setExpression(const QString& expression)
{
    if (m_expression != expression) {
        m_expression = expression;
        emit expressionChanged(expression);
        emit elementChanged();
    }
}

SYSML_NAMESPACE_END


