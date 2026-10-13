#include "RedefinesStatement.h"

SYSML_NAMESPACE_BEGIN

RedefinesStatement::RedefinesStatement(QObject* parent)
    : Element(parent)
{
}

void RedefinesStatement::setRedefinesName(const QString& name)
{
    if (m_redefinesName != name) {
        m_redefinesName = name;
        emit redefinesNameChanged(name);
    }
}

void RedefinesStatement::setExpression(const QString& expression)
{
    if (m_expression != expression) {
        m_expression = expression;
        emit expressionChanged(expression);
    }
}

SYSML_NAMESPACE_END

