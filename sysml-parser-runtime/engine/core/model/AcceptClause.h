#ifndef SYSML_ACCEPTCLAUSE_H
#define SYSML_ACCEPTCLAUSE_H

#include "Element.h"
#include "../common/Global.h"
#include <QString>

SYSML_NAMESPACE_BEGIN

/**
 * @brief AcceptClause 表示 accept 子句
 * 
 * 支持多种形式：
 * - acceptItem: accept ignitionCmd:IgnitionCmd via ignitionCmdPort
 * - acceptExpression: accept when condition, accept at time, accept event
 */
class AcceptClause : public Element {
    Q_OBJECT

    Q_PROPERTY(QString acceptType READ acceptType WRITE setAcceptType NOTIFY acceptTypeChanged)
    Q_PROPERTY(QString usageDeclaration READ usageDeclaration WRITE setUsageDeclaration NOTIFY usageDeclarationChanged)
    Q_PROPERTY(QString viaPort READ viaPort WRITE setViaPort NOTIFY viaPortChanged)
    Q_PROPERTY(QString condition READ condition WRITE setCondition NOTIFY conditionChanged)

public:
    explicit AcceptClause(QObject* parent = nullptr);

    QString elementType() const override { return "AcceptClause"; }

    // accept 类型："item", "when", "at", "event"
    QString acceptType() const { return m_acceptType; }
    void setAcceptType(const QString& type);

    // 使用声明（用于 acceptItem）
    QString usageDeclaration() const { return m_usageDeclaration; }
    void setUsageDeclaration(const QString& decl);

    // via 端口（用于 acceptItem）
    QString viaPort() const { return m_viaPort; }
    void setViaPort(const QString& port);

    // 条件表达式（用于 acceptExpression）
    QString condition() const { return m_condition; }
    void setCondition(const QString& cond);

signals:
    void acceptTypeChanged(const QString& type);
    void usageDeclarationChanged(const QString& decl);
    void viaPortChanged(const QString& port);
    void conditionChanged(const QString& cond);

private:
    QString m_acceptType;        // "item", "when", "at", "event"
    QString m_usageDeclaration;  // 使用声明字符串
    QString m_viaPort;           // via 端口名称
    QString m_condition;         // 条件表达式字符串
};

SYSML_NAMESPACE_END

#endif // SYSML_ACCEPTCLAUSE_H


