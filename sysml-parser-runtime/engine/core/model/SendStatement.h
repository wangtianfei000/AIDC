#ifndef SYSML_SENDSTATEMENT_H
#define SYSML_SENDSTATEMENT_H

#include "Element.h"
#include "../common/Global.h"
#include <QString>

SYSML_NAMESPACE_BEGIN

/**
 * @brief SendStatement 表示 send 语句
 * 
 * 支持形式：
 * - send new ActionName() to target
 * - send to target
 */
class SendStatement : public Element {
    Q_OBJECT

    Q_PROPERTY(bool hasNew READ hasNew WRITE setHasNew NOTIFY hasNewChanged)
    Q_PROPERTY(QString actionInvocation READ actionInvocation WRITE setActionInvocation NOTIFY actionInvocationChanged)
    Q_PROPERTY(QString target READ target WRITE setTarget NOTIFY targetChanged)

public:
    explicit SendStatement(QObject* parent = nullptr);

    QString elementType() const override { return "SendStatement"; }

    // 是否有 "new" 关键字
    bool hasNew() const { return m_hasNew; }
    void setHasNew(bool hasNew);

    // 动作调用（字符串形式，如 "StartSignal()"）
    QString actionInvocation() const { return m_actionInvocation; }
    void setActionInvocation(const QString& invocation);

    // 目标（字符串形式）
    QString target() const { return m_target; }
    void setTarget(const QString& tgt);

signals:
    void hasNewChanged(bool hasNew);
    void actionInvocationChanged(const QString& invocation);
    void targetChanged(const QString& target);

private:
    bool m_hasNew;              // 是否有 "new" 关键字
    QString m_actionInvocation; // 动作调用字符串
    QString m_target;           // 目标字符串
};

SYSML_NAMESPACE_END

#endif // SYSML_SENDSTATEMENT_H


