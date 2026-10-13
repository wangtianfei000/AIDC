#include "SysMLTextExporter.h"
#include "../model/Element.h"
#include "../model/Package.h"
#include "../model/ExhibitStatement.h"
#include "../model/StateDefinition.h"
#include "../model/StateUsage.h"
#include "../model/Transition.h"
#include "../model/StateActionMember.h"
#include "../model/ConstraintStatement.h"
#include "../model/PartUsage.h"
#include "../model/TransitionTriggerPart.h"
#include "../model/TransitionGuardPart.h"
#include "../model/TransitionEffectPart.h"
#include "../model/SendStatement.h"
#include "../model/AcceptClause.h"
#include "../model/Type.h"
#include <QDebug>
#include <QTextStream>

SYSML_NAMESPACE_BEGIN

SysMLTextExporter::SysMLTextExporter()
{
}

QString SysMLTextExporter::exportToText(Element* rootElement)
{
    if (!rootElement) {
        return QString();
    }
    
    return exportElement(rootElement, 0);
}

QString SysMLTextExporter::exportElement(Element* element, int indentLevel)
{
    if (!element) {
        return QString();
    }
    
    QString type = element->elementType();
    
    if (type == "Package") {
        return exportPackage(qobject_cast<Package*>(element), indentLevel);
    } else if (type == "ExhibitStatement") {
        return exportExhibitStatement(qobject_cast<ExhibitStatement*>(element), indentLevel);
    } else if (type == "StateDefinition") {
        return exportStateDefinition(qobject_cast<StateDefinition*>(element), indentLevel);
    } else if (type == "StateUsage") {
        return exportStateUsage(qobject_cast<StateUsage*>(element), indentLevel);
    } else if (type == "Transition") {
        return exportTransition(qobject_cast<Transition*>(element), indentLevel);
    } else if (type == "StateActionMember") {
        return exportStateActionMember(qobject_cast<StateActionMember*>(element), indentLevel);
    } else if (type == "ConstraintStatement") {
        return exportConstraintStatement(qobject_cast<ConstraintStatement*>(element), indentLevel);
    } else if (type == "PartUsage") {
        PartUsage* partUsage = qobject_cast<PartUsage*>(element);
        if (partUsage && partUsage->isRef()) {
            return exportRefStatement(partUsage, indentLevel);
        }
        // 如果不是ref，使用默认处理
    }
    
    // 默认处理：只导出名称和子元素
    QString result;
    QTextStream stream(&result);
    
    QString name = element->declaredName();
    if (!name.isEmpty()) {
        stream << indent(indentLevel) << type.toLower() << " " << name << ";\n";
    }
    
    result += exportOwnedElements(element, indentLevel);
    
    return result;
}

QString SysMLTextExporter::indent(int level) const
{
    return QString(level * 4, ' ');
}

QString SysMLTextExporter::escapeIdentifier(const QString& identifier) const
{
    // 如果标识符是关键字（如 "initial"），可能需要转义
    // 目前直接返回
    return identifier;
}

QString SysMLTextExporter::formatQualifiedName(const QString& name) const
{
    // 将 :: 分隔的限定名格式化
    return name;
}

QString SysMLTextExporter::exportPackage(Package* pkg, int indentLevel)
{
    if (!pkg) {
        return QString();
    }
    
    QString result;
    QTextStream stream(&result);
    
    QString name = pkg->declaredName();
    if (!name.isEmpty()) {
        stream << indent(indentLevel) << "package " << name << " {\n";
        result += exportOwnedElements(pkg, indentLevel + 1);
        stream << indent(indentLevel) << "}\n";
    } else {
        // 匿名包，只导出子元素
        result += exportOwnedElements(pkg, indentLevel);
    }
    
    return result;
}

QString SysMLTextExporter::exportExhibitStatement(ExhibitStatement* stmt, int indentLevel)
{
    if (!stmt) {
        return QString();
    }
    
    QString result;
    QTextStream stream(&result);
    
    QString name = stmt->declaredName();
    QString kind = stmt->stateMachineKind();
    
    stream << indent(indentLevel) << "exhibit state";
    if (!name.isEmpty()) {
        stream << " " << name;
    }
    if (kind == "parallel") {
        stream << " parallel";
    }
    stream << " {\n";
    
    result += exportOwnedElements(stmt, indentLevel + 1);
    
    stream << indent(indentLevel) << "}\n";
    
    return result;
}

QString SysMLTextExporter::exportStateDefinition(StateDefinition* stateDef, int indentLevel)
{
    if (!stateDef) {
        return QString();
    }
    
    QString result;
    QTextStream stream(&result);
    
    QString name = stateDef->declaredName();
    if (!name.isEmpty()) {
        stream << indent(indentLevel) << "state def " << name;
        
        // 检查是否有子元素
        QList<Element*> children = stateDef->ownedElements();
        if (children.isEmpty()) {
            stream << ";\n";
        } else {
            stream << " {\n";
            result += exportOwnedElements(stateDef, indentLevel + 1);
            stream << indent(indentLevel) << "}\n";
        }
    }
    
    return result;
}

QString SysMLTextExporter::exportStateUsage(StateUsage* stateUsage, int indentLevel)
{
    if (!stateUsage) {
        return QString();
    }
    
    QString result;
    QTextStream stream(&result);
    
    QString name = stateUsage->declaredName();
    if (!name.isEmpty()) {
        stream << indent(indentLevel) << "state " << name;
        
        // 检查是否有子元素
        QList<Element*> children = stateUsage->ownedElements();
        if (children.isEmpty()) {
            stream << ";\n";
        } else {
            stream << " {\n";
            result += exportOwnedElements(stateUsage, indentLevel + 1);
            stream << indent(indentLevel) << "}\n";
        }
    }
    
    return result;
}

QString SysMLTextExporter::exportTransition(Transition* transition, int indentLevel)
{
    if (!transition) {
        return QString();
    }
    
    QString result;
    QTextStream stream(&result);
    
    stream << indent(indentLevel) << "transition";
    
    // 转换名称
    QString name = transition->declaredName();
    // 如果名称是 transition<source,target> 格式，需要提取原始名称或忽略
    if (!name.isEmpty() && !name.startsWith("transition<")) {
        stream << " " << name;
    }
    
    QString sourceState = transition->sourceState();
    QString targetState = transition->targetState();
    bool isInitial = transition->isInitial();
    
    // 判断是简单转换还是复杂转换
    bool hasTrigger = transition->triggerPart() != nullptr;
    bool hasGuard = transition->guardPart() != nullptr;
    bool hasEffect = transition->effectPart() != nullptr;
    bool hasName = !name.isEmpty() && !name.startsWith("transition<");
    bool isComplex = hasTrigger || hasGuard || hasEffect || hasName;
    
    if (isInitial && !targetState.isEmpty() && !isComplex) {
        // 简单初始转换：transition initial then target;
        stream << " initial then " << targetState << ";\n";
    } else if (!isInitial && !sourceState.isEmpty() && !targetState.isEmpty() && !isComplex) {
        // 简单转换：transition source then target;
        stream << " " << sourceState << " then " << targetState << ";\n";
    } else {
        // 复杂转换
        stream << "\n";
        
        // first 子句（初始转换不需要first）
        if (!isInitial && !sourceState.isEmpty()) {
            stream << indent(indentLevel + 1) << "first " << sourceState << "\n";
        }
        
        // accept 子句（触发部分）
        TransitionTriggerPart* trigger = transition->triggerPart();
        if (trigger) {
            QString triggerText = exportTransitionTriggerPart(trigger, indentLevel + 1);
            stream << triggerText;
            // triggerText 已经包含换行符
        }
        
        // if 子句（守卫部分）- 通常在accept后的下一行，带缩进
        TransitionGuardPart* guard = transition->guardPart();
        if (guard) {
            QString guardText = exportTransitionGuardPart(guard, indentLevel + 1);
            stream << guardText;
            // guardText 已经包含换行符
        }
        
        // do 子句（效果部分）
        TransitionEffectPart* effect = transition->effectPart();
        if (effect) {
            QString effectText = exportTransitionEffectPart(effect, indentLevel + 1);
            stream << effectText;
            if (!effectText.isEmpty() && !effectText.endsWith("\n")) {
                stream << "\n";
            }
        }
        
        // then 子句
        if (!targetState.isEmpty()) {
            stream << indent(indentLevel + 1) << "then " << targetState << ";\n";
        }
    }
    
    return result;
}

QString SysMLTextExporter::exportStateActionMember(StateActionMember* action, int indentLevel)
{
    if (!action) {
        return QString();
    }
    
    QString result;
    QTextStream stream(&result);
    
    QString kind = action->actionKind();  // "entry", "do", "exit"
    QString name = action->actionName();
    bool hasActionKeyword = action->hasActionKeyword();
    
    stream << indent(indentLevel) << kind;
    
    if (hasActionKeyword) {
        stream << " action";
    }
    
    if (!name.isEmpty()) {
        stream << " " << name;
    }
    
    stream << ";\n";
    
    return result;
}

QString SysMLTextExporter::exportConstraintStatement(ConstraintStatement* constraint, int indentLevel)
{
    if (!constraint) {
        return QString();
    }
    
    QString result;
    QTextStream stream(&result);
    
    stream << indent(indentLevel) << "constraint";
    
    QString expression = constraint->expression();
    if (!expression.isEmpty()) {
        stream << " {" << expression << "}\n";
    } else {
        stream << ";\n";
    }
    
    return result;
}

QString SysMLTextExporter::exportRefStatement(PartUsage* refPart, int indentLevel)
{
    if (!refPart || !refPart->isRef()) {
        return QString();
    }
    
    QString result;
    QTextStream stream(&result);
    
    stream << indent(indentLevel) << "ref";
    
    QString name = refPart->declaredName();
    QString typeName = refPart->typeName();
    
    if (!name.isEmpty() || !typeName.isEmpty()) {
        if (!name.isEmpty() && !typeName.isEmpty()) {
            // ref name : Type;
            stream << " " << name << " : " << typeName;
        } else if (!name.isEmpty()) {
            stream << " " << name;
        } else if (!typeName.isEmpty()) {
            stream << " : " << typeName;
        }
    }
    
    stream << ";\n";
    
    return result;
}

QString SysMLTextExporter::exportTransitionTriggerPart(TransitionTriggerPart* trigger, int indentLevel)
{
    if (!trigger) {
        return QString();
    }
    
    AcceptClause* accept = trigger->acceptClause();
    if (!accept) {
        return QString();
    }
    
    return exportAcceptClause(accept, indentLevel);
}

QString SysMLTextExporter::exportTransitionGuardPart(TransitionGuardPart* guard, int indentLevel)
{
    if (!guard) {
        return QString();
    }
    
    QString result;
    QTextStream stream(&result);
    
    QString condition = guard->condition();
    if (!condition.isEmpty()) {
        // if 子句通常在 accept 之后，需要额外缩进一级
        stream << indent(indentLevel + 1) << "if " << condition << "\n";
    }
    
    return result;
}

QString SysMLTextExporter::exportTransitionEffectPart(TransitionEffectPart* effect, int indentLevel)
{
    if (!effect) {
        return QString();
    }
    
    QString result;
    QTextStream stream(&result);
    
    QString actionType = effect->actionType();
    
    if (actionType == "send") {
        SendStatement* send = effect->sendStatement();
        if (send) {
            stream << indent(indentLevel) << "do ";
            QString sendText = exportSendStatement(send, 0);  // sendStatement 自己处理格式
            stream << sendText << "\n";
        }
    } else if (actionType == "reference") {
        QString ref = effect->actionReference();
        if (!ref.isEmpty()) {
            stream << indent(indentLevel) << "do " << ref << "\n";
        }
    } else if (actionType == "invocation") {
        QString invocation = effect->actionInvocation();
        if (!invocation.isEmpty()) {
            stream << indent(indentLevel) << "do " << invocation << "\n";
        }
    }
    
    return result;
}

QString SysMLTextExporter::exportSendStatement(SendStatement* send, int indentLevel)
{
    if (!send) {
        return QString();
    }
    
    QString result;
    QTextStream stream(&result);
    
    stream << "send";
    
    bool hasNew = send->hasNew();
    QString actionInv = send->actionInvocation();
    QString target = send->target();
    
    if (hasNew && !actionInv.isEmpty()) {
        stream << " new " << actionInv;
    } else if (!actionInv.isEmpty()) {
        stream << " " << actionInv;
    }
    
    if (!target.isEmpty()) {
        stream << " to " << target;
    }
    
    return result;
}

QString SysMLTextExporter::exportAcceptClause(AcceptClause* accept, int indentLevel)
{
    if (!accept) {
        return QString();
    }
    
    QString result;
    QTextStream stream(&result);
    
    stream << indent(indentLevel) << "accept";
    
    QString acceptType = accept->acceptType();
    
    if (acceptType == "item") {
        QString usageDecl = accept->usageDeclaration();
        QString viaPort = accept->viaPort();
        
        if (!usageDecl.isEmpty()) {
            stream << " " << usageDecl;
        }
        
        if (!viaPort.isEmpty()) {
            stream << " via " << viaPort;
        }
    } else if (acceptType == "when") {
        QString condition = accept->condition();
        if (!condition.isEmpty()) {
            stream << " when " << condition;
        }
    } else if (acceptType == "at") {
        QString condition = accept->condition();
        if (!condition.isEmpty()) {
            stream << " at " << condition;
        }
    } else if (acceptType == "event") {
        QString condition = accept->condition();  // 事件名称
        if (!condition.isEmpty()) {
            stream << " " << condition;
        }
    }
    
    stream << "\n";
    
    return result;
}

QString SysMLTextExporter::exportOwnedElements(Element* element, int indentLevel)
{
    if (!element) {
        return QString();
    }
    
    QString result;
    
    // 获取所有子元素
    QList<Element*> children = element->ownedElements();
    
    // 按类型分组，以保持合理的顺序
    // 顺序：ref statements, states (def/usage), state actions, constraints, transitions
    QList<Element*> refs;
    QList<Element*> states;
    QList<Element*> actions;
    QList<Element*> constraints;
    QList<Element*> transitions;
    QList<Element*> others;
    
    for (Element* child : children) {
        QString type = child->elementType();
        if (type == "PartUsage") {
            PartUsage* partUsage = qobject_cast<PartUsage*>(child);
            if (partUsage && partUsage->isRef()) {
                refs.append(child);
                continue;
            }
        } else if (type == "StateDefinition" || type == "StateUsage") {
            states.append(child);
            continue;
        } else if (type == "StateActionMember") {
            actions.append(child);
            continue;
        } else if (type == "ConstraintStatement") {
            constraints.append(child);
            continue;
        } else if (type == "Transition") {
            transitions.append(child);
            continue;
        }
        others.append(child);
    }
    
    // 导出各组
    for (Element* child : refs) {
        result += exportElement(child, indentLevel);
    }
    
    for (Element* child : states) {
        result += exportElement(child, indentLevel);
    }
    
    for (Element* child : actions) {
        result += exportElement(child, indentLevel);
    }
    
    for (Element* child : constraints) {
        result += exportElement(child, indentLevel);
    }
    
    for (Element* child : transitions) {
        result += exportElement(child, indentLevel);
    }
    
    for (Element* child : others) {
        result += exportElement(child, indentLevel);
    }
    
    return result;
}

QString SysMLTextExporter::formatUsageDeclaration(const QString& name, const QString& type) const
{
    if (!name.isEmpty() && !type.isEmpty()) {
        return name + " : " + type;
    } else if (!name.isEmpty()) {
        return name;
    } else if (!type.isEmpty()) {
        return " : " + type;
    }
    return QString();
}

SYSML_NAMESPACE_END

