#ifndef SYSML_TRANSITIONGUARDPART_H
#define SYSML_TRANSITIONGUARDPART_H

#include "Element.h"
#include "../common/Global.h"
#include <QString>

SYSML_NAMESPACE_BEGIN

/**
 * @brief TransitionGuardPart 表示转换守卫部分
 * 
 * 包含 if 条件表达式
 */
class TransitionGuardPart : public Element {
    Q_OBJECT

    Q_PROPERTY(QString condition READ condition WRITE setCondition NOTIFY conditionChanged)

public:
    explicit TransitionGuardPart(QObject* parent = nullptr);

    QString elementType() const override { return "TransitionGuardPart"; }

    // 守卫条件表达式
    QString condition() const { return m_condition; }
    void setCondition(const QString& cond);

signals:
    void conditionChanged(const QString& cond);

private:
    QString m_condition;  // 条件表达式字符串
};

SYSML_NAMESPACE_END

#endif // SYSML_TRANSITIONGUARDPART_H


