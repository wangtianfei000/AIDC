#ifndef SYSML_ASSERTSTATEMENT_H
#define SYSML_ASSERTSTATEMENT_H

#include "Element.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

/**
 * @brief AssertStatement 表示 assert 语句
 * 
 * 参考: org.omg.sysml.metamodel.AssertConstraintUsage
 */
class AssertStatement : public Element {
    Q_OBJECT
    
    Q_PROPERTY(QString expression READ expression WRITE setExpression NOTIFY expressionChanged)
    
public:
    explicit AssertStatement(QObject* parent = nullptr);
    
    QString elementType() const override { return "AssertStatement"; }
    
    QString expression() const { return m_expression; }
    void setExpression(const QString& expression);
    
signals:
    void expressionChanged(const QString& expression);
    
private:
    QString m_expression;  // 约束表达式
};

SYSML_NAMESPACE_END

#endif // SYSML_ASSERTSTATEMENT_H



