#include "ConnectStatement.h"

SYSML_NAMESPACE_BEGIN

ConnectStatement::ConnectStatement(QObject* parent)
    : Element(parent)
{
}

void ConnectStatement::setEnds(const QStringList& ends)
{
    if (m_ends != ends) {
        m_ends = ends;
        emit endsChanged(ends);
    }
}

void ConnectStatement::setIsBinary(bool isBinary)
{
    if (m_isBinary != isBinary) {
        m_isBinary = isBinary;
        emit isBinaryChanged(isBinary);
    }
}

SYSML_NAMESPACE_END




