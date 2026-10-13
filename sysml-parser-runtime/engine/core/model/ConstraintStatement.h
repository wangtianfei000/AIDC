#ifndef SYSML_CONSTRAINTSTATEMENT_H
#define SYSML_CONSTRAINTSTATEMENT_H

#include "Element.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

/**
 * @brief ConstraintStatement 表示 constraint 语句
 * 
 * 用于状态体中的约束，如：
 *   constraint {speed >= 0}
 *   constraint;
 * 
 * 参考: org.omg.sysml.metamodel.ConstraintUsage
 */
class ConstraintStatement : public Element {
    Q_OBJECT
    
    Q_PROPERTY(QString expression READ expression WRITE setExpression NOTIFY expressionChanged)
    
public:
    explicit ConstraintStatement(QObject* parent = nullptr);
    
    QString elementType() const override { return "ConstraintStatement"; }
    
    QString expression() const { return m_expression; }
    void setExpression(const QString& expression);
    
    // 是否有表达式（constraint {expression} vs constraint;）
    bool hasExpression() const { return !m_expression.isEmpty(); }
    
signals:
    void expressionChanged(const QString& expression);
    
private:
    QString m_expression;  // 约束表达式（可能为空，对于 constraint; 形式）
};

SYSML_NAMESPACE_END

#endif // SYSML_CONSTRAINTSTATEMENT_H


