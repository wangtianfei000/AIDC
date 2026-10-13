#include "DocStatement.h"

SYSML_NAMESPACE_BEGIN

DocStatement::DocStatement(QObject* parent)
    : Element(parent)
{
}

void DocStatement::setContent(const QString& content)
{
    if (m_content != content) {
        m_content = content;
        emit contentChanged(content);
    }
}

SYSML_NAMESPACE_END


