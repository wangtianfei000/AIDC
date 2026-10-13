#ifndef SYSML_REDEFINESSTATEMENT_H
#define SYSML_REDEFINESSTATEMENT_H

#include "Element.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

/**
 * @brief RedefinesStatement 表示 redefines 语句
 * 
 * 用于 requirement usage 中的独立 redefines 语句
 */
class RedefinesStatement : public Element {
    Q_OBJECT
    
    Q_PROPERTY(QString redefinesName READ redefinesName WRITE setRedefinesName NOTIFY redefinesNameChanged)
    Q_PROPERTY(QString expression READ expression WRITE setExpression NOTIFY expressionChanged)
    
public:
    explicit RedefinesStatement(QObject* parent = nullptr);
    
    QString elementType() const override { return "RedefinesStatement"; }
    
    QString redefinesName() const { return m_redefinesName; }
    void setRedefinesName(const QString& name);
    
    QString expression() const { return m_expression; }
    void setExpression(const QString& expression);
    
signals:
    void redefinesNameChanged(const QString& name);
    void expressionChanged(const QString& expression);
    
private:
    QString m_redefinesName;  // 被重定义的名称
    QString m_expression;  // 重定义的表达式
};

SYSML_NAMESPACE_END

#endif // SYSML_REDEFINESSTATEMENT_H

