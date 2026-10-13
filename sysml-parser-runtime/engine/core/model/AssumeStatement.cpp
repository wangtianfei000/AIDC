#include "AssumeStatement.h"

SYSML_NAMESPACE_BEGIN

AssumeStatement::AssumeStatement(QObject* parent)
    : Element(parent)
{
}

void AssumeStatement::setExpression(const QString& expression)
{
    if (m_expression != expression) {
        m_expression = expression;
        emit expressionChanged(expression);
    }
}

SYSML_NAMESPACE_END

