#include "AllocateStatement.h"

SYSML_NAMESPACE_BEGIN

AllocateStatement::AllocateStatement(QObject* parent)
    : Element(parent)
{
}

void AllocateStatement::setSourceEnds(const QStringList& sourceEnds)
{
    if (m_sourceEnds != sourceEnds) {
        m_sourceEnds = sourceEnds;
        emit sourceEndsChanged(sourceEnds);
    }
}

void AllocateStatement::setTargetEnd(const QString& targetEnd)
{
    if (m_targetEnd != targetEnd) {
        m_targetEnd = targetEnd;
        emit targetEndChanged(targetEnd);
    }
}

void AllocateStatement::setIsBinary(bool isBinary)
{
    if (m_isBinary != isBinary) {
        m_isBinary = isBinary;
        emit isBinaryChanged(isBinary);
    }
}

SYSML_NAMESPACE_END




