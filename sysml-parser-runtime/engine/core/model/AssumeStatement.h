#ifndef SYSML_ASSUMESTATEMENT_H
#define SYSML_ASSUMESTATEMENT_H

#include "Element.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

/**
 * @brief AssumeStatement 表示 assume 语句
 * 
 * 参考: org.omg.sysml.metamodel.AssumeConstraintUsage
 */
class AssumeStatement : public Element {
    Q_OBJECT
    
    Q_PROPERTY(QString expression READ expression WRITE setExpression NOTIFY expressionChanged)
    
public:
    explicit AssumeStatement(QObject* parent = nullptr);
    
    QString elementType() const override { return "AssumeStatement"; }
    
    QString expression() const { return m_expression; }
    void setExpression(const QString& expression);
    
signals:
    void expressionChanged(const QString& expression);
    
private:
    QString m_expression;  // 约束表达式
};

SYSML_NAMESPACE_END

#endif // SYSML_ASSUMESTATEMENT_H

