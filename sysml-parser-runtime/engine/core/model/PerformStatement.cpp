#include "PerformStatement.h"

SYSML_NAMESPACE_BEGIN

PerformStatement::PerformStatement(QObject* parent)
    : Element(parent)
{
}

void PerformStatement::setActionReference(const QString& actionReference)
{
    if (m_actionReference != actionReference) {
        m_actionReference = actionReference;
        emit actionReferenceChanged(actionReference);
    }
}

void PerformStatement::setIsActionKeyword(bool isActionKeyword)
{
    if (m_isActionKeyword != isActionKeyword) {
        m_isActionKeyword = isActionKeyword;
        emit isActionKeywordChanged(isActionKeyword);
    }
}

void PerformStatement::setActionName(const QString& actionName)
{
    if (m_actionName != actionName) {
        m_actionName = actionName;
        emit actionNameChanged(actionName);
    }
}

void PerformStatement::setActionType(const QString& actionType)
{
    if (m_actionType != actionType) {
        m_actionType = actionType;
        emit actionTypeChanged(actionType);
    }
}

void PerformStatement::setSpecializationType(const QString& specializationType)
{
    if (m_specializationType != specializationType) {
        m_specializationType = specializationType;
        emit specializationTypeChanged(specializationType);
    }
}

SYSML_NAMESPACE_END

