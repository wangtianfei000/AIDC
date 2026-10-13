#ifndef SYSML_TRANSITIONEFFECTPART_H
#define SYSML_TRANSITIONEFFECTPART_H

#include "Element.h"
#include "SendStatement.h"
#include "../common/Global.h"
#include <QString>

SYSML_NAMESPACE_BEGIN

/**
 * @brief TransitionEffectPart 表示转换效果部分
 * 
 * 包含转换动作（send, actionReference, actionInvocation）
 */
class TransitionEffectPart : public Element {
    Q_OBJECT

    Q_PROPERTY(QString actionType READ actionType WRITE setActionType NOTIFY actionTypeChanged)
    Q_PROPERTY(SendStatement* sendStatement READ sendStatement WRITE setSendStatement NOTIFY sendStatementChanged)
    Q_PROPERTY(QString actionReference READ actionReference WRITE setActionReference NOTIFY actionReferenceChanged)
    Q_PROPERTY(QString actionInvocation READ actionInvocation WRITE setActionInvocation NOTIFY actionInvocationChanged)

public:
    explicit TransitionEffectPart(QObject* parent = nullptr);

    QString elementType() const override { return "TransitionEffectPart"; }

    // 动作类型："send", "reference", "invocation"
    QString actionType() const { return m_actionType; }
    void setActionType(const QString& type);

    // send 语句
    SendStatement* sendStatement() const { return m_sendStatement; }
    void setSendStatement(SendStatement* stmt);

    // 动作引用（字符串形式）
    QString actionReference() const { return m_actionReference; }
    void setActionReference(const QString& ref);

    // 动作调用（字符串形式）
    QString actionInvocation() const { return m_actionInvocation; }
    void setActionInvocation(const QString& invocation);

signals:
    void actionTypeChanged(const QString& type);
    void sendStatementChanged(SendStatement* stmt);
    void actionReferenceChanged(const QString& ref);
    void actionInvocationChanged(const QString& invocation);

private:
    QString m_actionType;        // "send", "reference", "invocation"
    SendStatement* m_sendStatement = nullptr;  // send 语句
    QString m_actionReference;   // 动作引用字符串
    QString m_actionInvocation;  // 动作调用字符串
};

SYSML_NAMESPACE_END

#endif // SYSML_TRANSITIONEFFECTPART_H

