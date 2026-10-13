#ifndef SYSML_SYSMLTEXTEXPORTER_H
#define SYSML_SYSMLTEXTEXPORTER_H

#include "../common/Global.h"
#include <QString>
#include <QStringList>

SYSML_NAMESPACE_BEGIN

// 前向声明
class Element;
class Package;
class ExhibitStatement;
class StateDefinition;
class StateUsage;
class Transition;
class StateActionMember;
class ConstraintStatement;
class PartUsage;
class TransitionTriggerPart;
class TransitionGuardPart;
class TransitionEffectPart;
class SendStatement;
class AcceptClause;

/**
 * @brief SysMLTextExporter 负责将模型导出为 SysML 文本格式
 * 
 * 支持导出状态机相关的元素：
 * - exhibit state
 * - state def / state usage
 * - transition (包含 first/then/accept/if/do)
 * - entry/do/exit actions
 * - constraint statements
 * - ref statements
 */
class SysMLTextExporter {
public:
    SysMLTextExporter();
    
    // 导出资源到字符串
    QString exportToText(Element* rootElement);
    
    // 导出元素
    QString exportElement(Element* element, int indentLevel = 0);
    
private:
    // 辅助方法
    QString indent(int level) const;
    QString escapeIdentifier(const QString& identifier) const;
    QString formatQualifiedName(const QString& name) const;
    
    // 导出特定元素类型
    QString exportPackage(Package* pkg, int indentLevel);
    QString exportExhibitStatement(ExhibitStatement* stmt, int indentLevel);
    QString exportStateDefinition(StateDefinition* stateDef, int indentLevel);
    QString exportStateUsage(StateUsage* stateUsage, int indentLevel);
    QString exportTransition(Transition* transition, int indentLevel);
    QString exportStateActionMember(StateActionMember* action, int indentLevel);
    QString exportConstraintStatement(ConstraintStatement* constraint, int indentLevel);
    QString exportRefStatement(PartUsage* refPart, int indentLevel);
    
    // 导出转换的各个部分
    QString exportTransitionTriggerPart(TransitionTriggerPart* trigger, int indentLevel);
    QString exportTransitionGuardPart(TransitionGuardPart* guard, int indentLevel);
    QString exportTransitionEffectPart(TransitionEffectPart* effect, int indentLevel);
    QString exportSendStatement(SendStatement* send, int indentLevel);
    QString exportAcceptClause(AcceptClause* accept, int indentLevel);
    
    // 导出子元素
    QString exportOwnedElements(Element* element, int indentLevel);
    
    // 工具方法
    QString formatUsageDeclaration(const QString& name, const QString& type) const;
};

SYSML_NAMESPACE_END

#endif // SYSML_SYSMLTEXTEXPORTER_H

