#ifndef SYSML_TRANSITIONTRIGGERPART_H
#define SYSML_TRANSITIONTRIGGERPART_H

#include "Element.h"
#include "AcceptClause.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

/**
 * @brief TransitionTriggerPart 表示转换触发部分
 * 
 * 包含 accept 子句
 */
class TransitionTriggerPart : public Element {
    Q_OBJECT

    Q_PROPERTY(AcceptClause* acceptClause READ acceptClause WRITE setAcceptClause NOTIFY acceptClauseChanged)

public:
    explicit TransitionTriggerPart(QObject* parent = nullptr);

    QString elementType() const override { return "TransitionTriggerPart"; }

    // accept 子句
    AcceptClause* acceptClause() const { return m_acceptClause; }
    void setAcceptClause(AcceptClause* clause);

signals:
    void acceptClauseChanged(AcceptClause* clause);

private:
    AcceptClause* m_acceptClause = nullptr;
};

SYSML_NAMESPACE_END

#endif // SYSML_TRANSITIONTRIGGERPART_H

