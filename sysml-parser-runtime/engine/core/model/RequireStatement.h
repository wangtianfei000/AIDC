#ifndef SYSML_REQUIRESTATEMENT_H
#define SYSML_REQUIRESTATEMENT_H

#include "Element.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

/**
 * @brief RequireStatement 表示 require 语句
 * 
 * 参考: org.omg.sysml.metamodel.RequireConstraintUsage
 */
class RequireStatement : public Element {
    Q_OBJECT
    
    Q_PROPERTY(QString expression READ expression WRITE setExpression NOTIFY expressionChanged)
    
public:
    explicit RequireStatement(QObject* parent = nullptr);
    
    QString elementType() const override { return "RequireStatement"; }
    
    QString expression() const { return m_expression; }
    void setExpression(const QString& expression);
    
    // 是否包含 constraint 关键字（require constraint {expression} vs require expression）
    bool hasConstraintKeyword() const { return m_hasConstraintKeyword; }
    void setHasConstraintKeyword(bool hasConstraint);
    
signals:
    void expressionChanged(const QString& expression);
    void hasConstraintKeywordChanged(bool hasConstraint);
    
private:
    QString m_expression;  // 约束表达式
    bool m_hasConstraintKeyword = false;  // 是否包含 constraint 关键字
};

SYSML_NAMESPACE_END

#endif // SYSML_REQUIRESTATEMENT_H

