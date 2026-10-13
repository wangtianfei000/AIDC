#ifndef SYSML_EXHIBITSTATEMENT_H
#define SYSML_EXHIBITSTATEMENT_H

#include "Element.h"
#include "../common/Global.h"

SYSML_NAMESPACE_BEGIN

/**
 * @brief ExhibitStatement 表示 exhibit state 语句
 * 
 * 用于在 part 或其他元素中展示状态机
 * 语法: exhibit state name parallel? { ... }
 */
class ExhibitStatement : public Element {
    Q_OBJECT
    
    Q_PROPERTY(QString stateMachineKind READ stateMachineKind WRITE setStateMachineKind NOTIFY stateMachineKindChanged)
    
public:
    explicit ExhibitStatement(QObject* parent = nullptr);
    
    QString elementType() const override { return "ExhibitStatement"; }
    
    // 状态机类型（"parallel" 或空字符串表示顺序状态机）
    QString stateMachineKind() const { return m_stateMachineKind; }
    void setStateMachineKind(const QString& kind);
    
signals:
    void stateMachineKindChanged(const QString& kind);
    
private:
    QString m_stateMachineKind;  // "parallel" 或空
};

SYSML_NAMESPACE_END

#endif // SYSML_EXHIBITSTATEMENT_H


