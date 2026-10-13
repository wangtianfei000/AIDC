#include "SysMLSerializer.h"
#include "../../core/model/Package.h"
#include "../../core/model/PartDefinition.h"
#include "../../core/model/PartUsage.h"
#include "../../core/model/ActionUsage.h"
#include "../../core/model/StateUsage.h"
#include "../../core/model/RequirementUsage.h"
#include "../../core/model/ConstraintUsage.h"
#include "../../core/model/PortUsage.h"
#include "../../core/model/ConnectorUsage.h"
#include "../../core/model/FlowUsage.h"
#include "../../core/model/Feature.h"
#include "../../core/model/Import.h"
#include "../../core/model/NamespaceImport.h"
#include "../../core/model/FeatureTyping.h"
#include "../../core/model/Type.h"
#include "../../core/model/Namespace.h"
#include "../../core/model/Relationship.h"
#include "../../core/model/ActionDefinition.h"
#include "../../core/model/StateDefinition.h"
#include "../../core/model/RequirementDefinition.h"
#include "../../core/model/ConstraintDefinition.h"
#include "../../core/model/PortDefinition.h"
#include "../../core/model/ConnectorDefinition.h"
#include "../../core/model/InterfaceDefinition.h"
#include "../../core/model/DataTypeDefinition.h"
#include "../../core/model/ClassDefinition.h"
#include "../../core/model/EnumDefinition.h"
#include "../../core/model/AttributeDefinition.h"
#include "../../core/model/ConnectionDefinition.h"
#include "../../core/model/ConnectionUsage.h"
#include "../../core/model/AllocationDefinition.h"
#include "../../core/model/AllocationUsage.h"
#include "../../core/model/ItemDefinition.h"
#include "../../core/model/ItemUsage.h"
#include "../../core/model/ViewpointDefinition.h"
#include "../../core/model/ViewDefinition.h"
#include "../../core/model/ViewUsage.h"
#include "../../core/model/ConcernDefinition.h"
#include "../../core/model/ExposeStatement.h"
#include "../../core/model/ElementFilterMember.h"
#include "../../core/model/ViewRenderingMember.h"
#include "../../core/model/FrameConcernMember.h"
#include "../../core/model/EndStatement.h"
#include "../../core/model/BindStatement.h"
#include "../../core/model/ConnectStatement.h"
#include "../../core/model/AllocateStatement.h"
#include "../../core/model/FlowDefinition.h"
#include "../../core/model/FlowStatement.h"
#include "../../core/model/PerformStatement.h"
#include "../../core/model/AssertStatement.h"
#include "../../core/model/RequireStatement.h"
#include "../../core/model/AssumeStatement.h"
#include "../../core/model/DocStatement.h"
#include "../../core/model/ActionInputOutput.h"
#include "../../core/model/Subclassification.h"
#include "../../core/model/Subsetting.h"
#include "../../core/model/Redefinition.h"
#include "../../core/model/Conjugation.h"
#include "../../core/model/Dependency.h"
// 状态机相关模型类
#include "../../core/model/ExhibitStatement.h"
#include "../../core/model/Transition.h"
#include "../../core/model/StateActionMember.h"
#include "../../core/model/ConstraintStatement.h"
#include "../../core/model/TransitionTriggerPart.h"
#include "../../core/model/TransitionGuardPart.h"
#include "../../core/model/TransitionEffectPart.h"
#include "../../core/model/AcceptClause.h"
#include "../../core/model/SendStatement.h"
#include "../../core/model/Annotation.h"
#include <QFile>
#include <QDebug>
#include <QRegularExpression>

SYSML_NAMESPACE_BEGIN

SysMLSerializer::SysMLSerializer()
{
}

SysMLSerializer::~SysMLSerializer()
{
}

QString SysMLSerializer::serializeResource(Resource* resource)
{
    if (!resource) {
        return QString();
    }
    
    QString result;
    QTextStream stream(&result);
    
    // 序列化所有顶层元素（元素之间不加空行）
    for (Element* element : resource->contents()) {
        serializeElement(element, stream, 0);
    }
    
    return result;
}

QString SysMLSerializer::serializeElement(Element* element)
{
    QString result;
    QTextStream stream(&result);
    serializeElement(element, stream, 0);
    return result;
}

void SysMLSerializer::serializeElement(Element* element, QTextStream& stream, int indent)
{
    if (!element) {
        return;
    }
    
    QString type = element->elementType();
    
    if (type == "Package") {
        serializePackage(qobject_cast<Package*>(element), stream, indent);
    } else if (type == "PartDefinition") {
        serializePartDefinition(qobject_cast<PartDefinition*>(element), stream, indent);
    } else if (type == "PartUsage") {
        serializePartUsage(qobject_cast<PartUsage*>(element), stream, indent);
    } else if (type == "ActionUsage") {
        serializeActionUsage(qobject_cast<ActionUsage*>(element), stream, indent);
    } else if (type == "StateUsage") {
        serializeStateUsage(qobject_cast<StateUsage*>(element), stream, indent);
    } else if (type == "RequirementUsage") {
        serializeRequirementUsage(qobject_cast<RequirementUsage*>(element), stream, indent);
    } else if (type == "ConstraintUsage") {
        serializeConstraintUsage(qobject_cast<ConstraintUsage*>(element), stream, indent);
    } else if (type == "PortUsage") {
        serializePortUsage(qobject_cast<PortUsage*>(element), stream, indent);
    } else if (type == "ConnectorUsage") {
        serializeConnectorUsage(qobject_cast<ConnectorUsage*>(element), stream, indent);
    } else if (type == "FlowUsage") {
        serializeFlowUsage(qobject_cast<FlowUsage*>(element), stream, indent);
    } else if (type == "Feature") {
        serializeFeature(qobject_cast<Feature*>(element), stream, indent);
    } else if (type == "ActionDefinition") {
        serializeActionDefinition(qobject_cast<ActionDefinition*>(element), stream, indent);
    } else if (type == "StateDefinition") {
        serializeStateDefinition(qobject_cast<StateDefinition*>(element), stream, indent);
    } else if (type == "RequirementDefinition") {
        serializeRequirementDefinition(qobject_cast<RequirementDefinition*>(element), stream, indent);
    } else if (type == "ConstraintDefinition") {
        serializeConstraintDefinition(qobject_cast<ConstraintDefinition*>(element), stream, indent);
    } else if (type == "PortDefinition") {
        serializePortDefinition(qobject_cast<PortDefinition*>(element), stream, indent);
    } else if (type == "ConnectorDefinition") {
        serializeConnectorDefinition(qobject_cast<ConnectorDefinition*>(element), stream, indent);
    } else if (type == "InterfaceDefinition") {
        serializeInterfaceDefinition(qobject_cast<InterfaceDefinition*>(element), stream, indent);
    } else if (type == "DataTypeDefinition") {
        serializeDataTypeDefinition(qobject_cast<DataTypeDefinition*>(element), stream, indent);
    } else if (type == "ClassDefinition") {
        serializeClassDefinition(qobject_cast<ClassDefinition*>(element), stream, indent);
    } else if (type == "EnumDefinition") {
        serializeEnumDefinition(qobject_cast<EnumDefinition*>(element), stream, indent);
    } else if (type == "AttributeDefinition") {
        serializeAttributeDefinition(qobject_cast<AttributeDefinition*>(element), stream, indent);
    } else if (type == "ConnectionDefinition") {
        serializeConnectionDefinition(qobject_cast<ConnectionDefinition*>(element), stream, indent);
    } else if (type == "ConnectionUsage") {
        serializeConnectionUsage(qobject_cast<ConnectionUsage*>(element), stream, indent);
    } else if (type == "AllocationDefinition") {
        serializeAllocationDefinition(qobject_cast<AllocationDefinition*>(element), stream, indent);
    } else if (type == "AllocationUsage") {
        serializeAllocationUsage(qobject_cast<AllocationUsage*>(element), stream, indent);
    } else if (type == "ItemDefinition") {
        serializeItemDefinition(qobject_cast<ItemDefinition*>(element), stream, indent);
    } else if (type == "ItemUsage") {
        serializeItemUsage(qobject_cast<ItemUsage*>(element), stream, indent);
    } else if (type == "ViewpointDefinition") {
        serializeViewpointDefinition(qobject_cast<ViewpointDefinition*>(element), stream, indent);
    } else if (type == "ViewDefinition") {
        serializeViewDefinition(qobject_cast<ViewDefinition*>(element), stream, indent);
    } else if (type == "ViewUsage") {
        serializeViewUsage(qobject_cast<ViewUsage*>(element), stream, indent);
    } else if (type == "ConcernDefinition") {
        serializeConcernDefinition(qobject_cast<ConcernDefinition*>(element), stream, indent);
    } else if (type == "ExposeStatement") {
        serializeExposeStatement(qobject_cast<ExposeStatement*>(element), stream, indent);
    } else if (type == "ElementFilterMember") {
        serializeElementFilterMember(qobject_cast<ElementFilterMember*>(element), stream, indent);
    } else if (type == "ViewRenderingMember") {
        serializeViewRenderingMember(qobject_cast<ViewRenderingMember*>(element), stream, indent);
    } else if (type == "FrameConcernMember") {
        serializeFrameConcernMember(qobject_cast<FrameConcernMember*>(element), stream, indent);
    } else if (type == "EndStatement") {
        serializeEndStatement(qobject_cast<EndStatement*>(element), stream, indent);
    } else if (type == "BindStatement") {
        serializeBindStatement(qobject_cast<BindStatement*>(element), stream, indent);
    } else if (type == "ConnectStatement") {
        serializeConnectStatement(qobject_cast<ConnectStatement*>(element), stream, indent);
    } else if (type == "AllocateStatement") {
        serializeAllocateStatement(qobject_cast<AllocateStatement*>(element), stream, indent);
    } else if (type == "FlowDefinition") {
        serializeFlowDefinition(qobject_cast<FlowDefinition*>(element), stream, indent);
    } else if (type == "FlowStatement") {
        serializeFlowStatement(qobject_cast<FlowStatement*>(element), stream, indent);
    } else if (type == "PerformStatement") {
        serializePerformStatement(qobject_cast<PerformStatement*>(element), stream, indent);
    } else if (type == "AssertStatement") {
        serializeAssertStatement(qobject_cast<AssertStatement*>(element), stream, indent);
    } else if (type == "RequireStatement") {
        serializeRequireStatement(qobject_cast<RequireStatement*>(element), stream, indent);
    } else if (type == "AssumeStatement") {
        serializeAssumeStatement(qobject_cast<AssumeStatement*>(element), stream, indent);
    } else if (type == "DocStatement") {
        serializeDocStatement(qobject_cast<DocStatement*>(element), stream, indent);
    } else if (type == "Annotation") {
        serializeAnnotation(qobject_cast<Annotation*>(element), stream, indent);
    } else if (type == "ActionInputOutput") {
        serializeActionInputOutput(qobject_cast<ActionInputOutput*>(element), stream, indent);
    } else if (type == "ExhibitStatement") {
        serializeExhibitStatement(qobject_cast<ExhibitStatement*>(element), stream, indent);
    } else if (type == "Transition") {
        serializeTransition(qobject_cast<Transition*>(element), stream, indent);
    } else if (type == "StateActionMember") {
        serializeStateActionMember(qobject_cast<StateActionMember*>(element), stream, indent);
    } else if (type == "ConstraintStatement") {
        serializeConstraintStatement(qobject_cast<ConstraintStatement*>(element), stream, indent);
    } else if (type == "TransitionTriggerPart") {
        serializeTransitionTriggerPart(qobject_cast<TransitionTriggerPart*>(element), stream, indent);
    } else if (type == "TransitionGuardPart") {
        serializeTransitionGuardPart(qobject_cast<TransitionGuardPart*>(element), stream, indent);
    } else if (type == "TransitionEffectPart") {
        serializeTransitionEffectPart(qobject_cast<TransitionEffectPart*>(element), stream, indent);
    } else if (type == "AcceptClause") {
        serializeAcceptClause(qobject_cast<AcceptClause*>(element), stream, indent);
    } else if (type == "SendStatement") {
        serializeSendStatement(qobject_cast<SendStatement*>(element), stream, indent);
    } else {
        qWarning() << "[SysMLSerializer] Unknown element type:" << type;
    }
}

void SysMLSerializer::serializePackage(Package* package, QTextStream& stream, int indent)
{
    if (!package) {
        return;
    }
    
    QString indentStr = getIndent(indent);
    QString name = package->declaredName();
    if (name.isEmpty()) {
        name = "UnnamedPackage";
    }
    
    // 序列化包名（使用限定名，如果包有父命名空间）
    QString packageName = getQualifiedName(package);
    if (packageName.isEmpty()) {
        packageName = name;
    }
    
    stream << indentStr << "package " << packageName << "{\n";
    
    // 序列化导入（导入语句通常在包的开头）
    for (Relationship* rel : package->ownedRelationships()) {
        if (auto import = qobject_cast<Import*>(rel)) {
            serializeImport(import, stream, indent + 1);
        }
    }
    
    // 序列化子元素（元素之间不加空行）
    for (Element* child : package->ownedElements()) {
        serializeElement(child, stream, indent + 1);
    }
    
    stream << indentStr << "}\n";
}

void SysMLSerializer::serializePartDefinition(PartDefinition* partDef, QTextStream& stream, int indent)
{
    if (!partDef) {
        return;
    }
    
    QString indentStr = getIndent(indent);
    QString name = partDef->declaredName();
    if (name.isEmpty()) {
        name = "UnnamedPart";
    }
    
    // 序列化定义前缀
    QStringList prefixes;
    if (partDef->isAbstract()) {
        prefixes.append("abstract");
    }
    if (partDef->isVariation()) {
        prefixes.append("variation");
    }
    if (partDef->isIndividual()) {
        prefixes.append("individual");
    }
    
    if (!prefixes.isEmpty()) {
        stream << indentStr << prefixes.join(" ") << " ";
    } else {
        stream << indentStr;
    }
    
    stream << "part def " << name;
    
    // 序列化子类化关系
    QList<Subclassification*> subclassifications = findSubclassifications(partDef);
    if (!subclassifications.isEmpty()) {
        QStringList supertypeNames;
        for (Subclassification* subclass : subclassifications) {
            Type* supertype = subclass->supertype();
            if (supertype) {
                QString supertypeName = getQualifiedName(supertype);
                if (!supertypeName.isEmpty()) {
                    supertypeNames.append(supertypeName);
                }
            }
        }
        if (!supertypeNames.isEmpty()) {
            stream << " :> " << supertypeNames.join(", ");
        }
    }
    
    // 检查是否有子元素
    bool hasChildren = !partDef->ownedElements().isEmpty();
    
    if (hasChildren) {
        stream << " {\n";
        
        // 先序列化注解
        for (Element* annotation : partDef->ownedAnnotations()) {
            serializeElement(annotation, stream, indent + 1);
        }
        
        // 序列化子元素（元素之间不加空行）
        for (Element* child : partDef->ownedElements()) {
            // 跳过注解，因为已经序列化了
            if (child->elementType() == "Annotation") {
                continue;
            }
            serializeElement(child, stream, indent + 1);
        }
        
        stream << indentStr << "}\n";
    } else {
        // 即使没有子元素，也要序列化注解
        QList<Element*> annotations = partDef->ownedAnnotations();
        if (!annotations.isEmpty()) {
            stream << " {\n";
            for (Element* annotation : annotations) {
                serializeElement(annotation, stream, indent + 1);
            }
            stream << indentStr << "}\n";
        } else {
            stream << ";\n";
        }
    }
}

void SysMLSerializer::serializePartUsage(PartUsage* partUsage, QTextStream& stream, int indent)
{
    if (!partUsage) {
        return;
    }
    
    QString indentStr = getIndent(indent);
    QString name = partUsage->declaredName();
    if (name.isEmpty()) {
        name = "UnnamedPart";
    }
    
    // 序列化使用前缀
    QStringList prefixes;
    if (partUsage->direction() == PartUsage::DirectionIn) {
        prefixes.append("in");
    } else if (partUsage->direction() == PartUsage::DirectionOut) {
        prefixes.append("out");
    } else if (partUsage->direction() == PartUsage::DirectionInOut) {
        prefixes.append("inout");
    }
    if (partUsage->isDerived()) {
        prefixes.append("derived");
    }
    if (partUsage->isAbstract()) {
        prefixes.append("abstract");
    }
    if (partUsage->isVariation()) {
        prefixes.append("variation");
    }
    if (partUsage->isConstant()) {
        prefixes.append("constant");
    }
    if (partUsage->isRef()) {
        prefixes.append("ref");
    }
    if (partUsage->isIndividual()) {
        prefixes.append("individual");
    }
    if (partUsage->portionKind() == PartUsage::PortionSnapshot) {
        prefixes.append("snapshot");
    } else if (partUsage->portionKind() == PartUsage::PortionTimeslice) {
        prefixes.append("timeslice");
    }
    
    // 如果是 ref 语句，使用特殊格式：ref name : Type;
    if (partUsage->isRef()) {
        stream << indentStr << "ref";
        if (!name.isEmpty()) {
            stream << " " << name;
        }
        
        // 序列化类型
        Type* type = findPartUsageType(partUsage);
        if (type) {
            QString typeName = getQualifiedName(type);
            if (!typeName.isEmpty()) {
                stream << " : " << typeName;
            }
        } else {
            // 如果找不到类型，使用 PartUsage 中保存的类型名称
            QString typeName = partUsage->typeName();
            if (!typeName.isEmpty()) {
                stream << " : " << typeName;
            }
        }
        
        stream << ";\n";
        return;  // ref 语句不需要子元素序列化
    }
    
    // 非 ref 的 PartUsage，使用正常格式
    if (!prefixes.isEmpty()) {
        stream << indentStr << prefixes.join(" ") << " ";
    } else {
        stream << indentStr;
    }
    
    stream << "part " << name;
    
    // 序列化类型（PartUsage 可能不是 Feature，需要特殊处理）
    Type* type = findPartUsageType(partUsage);
    if (type) {
        QString typeName = getQualifiedName(type);
        if (!typeName.isEmpty()) {
            stream << " : " << typeName;
        }
    } else {
        // 如果找不到类型，使用 PartUsage 中保存的类型名称
        QString typeName = partUsage->typeName();
        if (!typeName.isEmpty()) {
            stream << " : " << typeName;
        }
    }
    
    // 检查是否有子元素
    bool hasChildren = !partUsage->ownedElements().isEmpty();
    QList<Element*> annotations = partUsage->ownedAnnotations();
    
    qDebug() << "[SysMLSerializer] serializePartUsage:" << name << "has" << annotations.size() << "annotations," << partUsage->ownedElements().size() << "children";
    
    if (hasChildren || !annotations.isEmpty()) {
        stream << " {\n";
        
        // 先序列化注解
        for (Element* annotation : annotations) {
            qDebug() << "[SysMLSerializer] serializePartUsage: serializing annotation" << qobject_cast<Annotation*>(annotation)->annotationName();
            serializeElement(annotation, stream, indent + 1);
        }
        
        // 序列化子元素（元素之间不加空行）
        for (Element* child : partUsage->ownedElements()) {
            // 跳过注解，因为已经序列化了
            if (child->elementType() == "Annotation") {
                continue;
            }
            serializeElement(child, stream, indent + 1);
        }
        
        stream << indentStr << "}\n";
    } else {
        stream << ";\n";
    }
}

void SysMLSerializer::serializeActionUsage(ActionUsage* actionUsage, QTextStream& stream, int indent)
{
    if (!actionUsage) {
        return;
    }
    
    QString indentStr = getIndent(indent);
    QString name = actionUsage->declaredName();
    if (name.isEmpty()) {
        name = "UnnamedAction";
    }
    
    stream << indentStr << "action " << name;
    
    Type* type = findUsageType(actionUsage);
    if (type) {
        QString typeName = getQualifiedName(type);
        if (!typeName.isEmpty()) {
            stream << " : " << typeName;
        }
    } else {
        QString typeName = actionUsage->typeName();
        if (!typeName.isEmpty()) {
            stream << " : " << typeName;
        }
    }
    
    stream << " {\n";
    
    // 序列化子元素（元素之间不加空行）
    for (Element* child : actionUsage->ownedElements()) {
        serializeElement(child, stream, indent + 1);
    }
    
    stream << indentStr << "}\n";
}

void SysMLSerializer::serializeStateUsage(StateUsage* stateUsage, QTextStream& stream, int indent)
{
    if (!stateUsage) {
        return;
    }
    
    QString indentStr = getIndent(indent);
    QString name = stateUsage->declaredName();
    if (name.isEmpty()) {
        name = "UnnamedState";
    }
    
    stream << indentStr << "state " << name;
    
    Type* type = findUsageType(stateUsage);
    if (type) {
        QString typeName = getQualifiedName(type);
        if (!typeName.isEmpty()) {
            stream << " : " << typeName;
        }
    } else {
        QString typeName = stateUsage->typeName();
        if (!typeName.isEmpty()) {
            stream << " : " << typeName;
        }
    }
    
    // 检查是否有子元素
    bool hasChildren = !stateUsage->ownedElements().isEmpty();
    
    if (hasChildren) {
        stream << " {\n";
        
        // 序列化子元素（按逻辑顺序：ref statements, states, actions, constraints, transitions）
        QList<Element*> refs;
        QList<Element*> states;
        QList<Element*> actions;
        QList<Element*> constraints;
        QList<Element*> transitions;
        QList<Element*> others;
        
        for (Element* child : stateUsage->ownedElements()) {
            QString childType = child->elementType();
            if (childType == "PartUsage") {
                PartUsage* partUsage = qobject_cast<PartUsage*>(child);
                if (partUsage && partUsage->isRef()) {
                    refs.append(child);
                    continue;
                }
            } else if (childType == "StateDefinition" || childType == "StateUsage") {
                states.append(child);
                continue;
            } else if (childType == "StateActionMember") {
                actions.append(child);
                continue;
            } else if (childType == "ConstraintStatement") {
                constraints.append(child);
                continue;
            } else if (childType == "Transition") {
                transitions.append(child);
                continue;
            }
            others.append(child);
        }
        
        // 导出各组
        for (Element* child : refs) {
            serializeElement(child, stream, indent + 1);
            stream << "\n";
        }
        
        for (Element* child : states) {
            serializeElement(child, stream, indent + 1);
            stream << "\n";
        }
        
        for (Element* child : actions) {
            serializeElement(child, stream, indent + 1);
            stream << "\n";
        }
        
        for (Element* child : constraints) {
            serializeElement(child, stream, indent + 1);
            stream << "\n";
        }
        
        for (Element* child : transitions) {
            serializeElement(child, stream, indent + 1);
            stream << "\n";
        }
        
        for (Element* child : others) {
            serializeElement(child, stream, indent + 1);
            stream << "\n";
        }
        
        stream << indentStr << "}\n";
    } else {
        stream << ";\n";
    }
}

void SysMLSerializer::serializeRequirementUsage(RequirementUsage* requirementUsage, QTextStream& stream, int indent)
{
    if (!requirementUsage) {
        return;
    }
    
    QString indentStr = getIndent(indent);
    QString name = requirementUsage->declaredName();
    if (name.isEmpty()) {
        name = "UnnamedRequirement";
    }
    
    stream << indentStr << "requirement " << name;
    
    Type* type = findUsageType(requirementUsage);
    if (type) {
        QString typeName = getQualifiedName(type);
        if (!typeName.isEmpty()) {
            stream << " : " << typeName;
        }
    } else {
        QString typeName = requirementUsage->typeName();
        if (!typeName.isEmpty()) {
            stream << " : " << typeName;
        }
    }
    
    bool hasChildren = !requirementUsage->ownedElements().isEmpty();
    if (hasChildren) {
        stream << " {\n";
        
        // 分离不同类型的子元素
        QList<Element*> regularElements;
        QList<Element*> requirementMembers; // subject, actor, stakeholder
        
        for (Element* child : requirementUsage->ownedElements()) {
            QString childName = child->declaredName();
            // 检查是否是 requirement member（格式为 "subject name", "actor name", "stakeholder name"）
            if (childName.startsWith("subject ") || childName.startsWith("actor ") || childName.startsWith("stakeholder ")) {
                requirementMembers.append(child);
            } else {
                regularElements.append(child);
            }
        }
        
        // 先序列化常规元素（元素之间不加空行）
        for (Element* child : regularElements) {
            serializeElement(child, stream, indent + 1);
        }
        
        // 然后序列化 requirement members（元素之间不加空行）
        for (Element* child : requirementMembers) {
            serializeRequirementMember(child, stream, indent + 1);
        }
        
        stream << indentStr << "}\n";
    } else {
        stream << ";\n";
    }
}

void SysMLSerializer::serializeConstraintUsage(ConstraintUsage* constraintUsage, QTextStream& stream, int indent)
{
    if (!constraintUsage) {
        return;
    }
    
    QString indentStr = getIndent(indent);
    QString name = constraintUsage->declaredName();
    if (name.isEmpty()) {
        name = "UnnamedConstraint";
    }
    
    stream << indentStr << "constraint " << name;
    
    Type* type = findUsageType(constraintUsage);
    if (type) {
        QString typeName = getQualifiedName(type);
        if (!typeName.isEmpty()) {
            stream << " : " << typeName;
        }
    } else {
        QString typeName = constraintUsage->typeName();
        if (!typeName.isEmpty()) {
            stream << " : " << typeName;
        }
    }
    
    stream << " {\n";
    
    // 序列化子元素（元素之间不加空行）
    for (Element* child : constraintUsage->ownedElements()) {
        serializeElement(child, stream, indent + 1);
    }
    
    stream << indentStr << "}\n";
}

void SysMLSerializer::serializePortUsage(PortUsage* portUsage, QTextStream& stream, int indent)
{
    if (!portUsage) {
        return;
    }
    
    QString indentStr = getIndent(indent);
    QString name = portUsage->declaredName();
    if (name.isEmpty()) {
        name = "UnnamedPort";
    }
    
    stream << indentStr << "port " << name;
    
    Type* type = findUsageType(portUsage);
    if (type) {
        QString typeName = getQualifiedName(type);
        if (!typeName.isEmpty()) {
            stream << " : " << typeName;
        }
    } else {
        QString typeName = portUsage->typeName();
        if (!typeName.isEmpty()) {
            stream << " : " << typeName;
        }
    }
    
    // 检查是否有子元素或注解
    bool hasChildren = !portUsage->ownedElements().isEmpty();
    QList<Element*> annotations = portUsage->ownedAnnotations();
    
    qDebug() << "[SysMLSerializer] serializePortUsage:" << name << "has" << annotations.size() << "annotations," << portUsage->ownedElements().size() << "children";
    
    if (hasChildren || !annotations.isEmpty()) {
        stream << " {\n";
        
        // 先序列化注解
        for (Element* annotation : annotations) {
            qDebug() << "[SysMLSerializer] serializePortUsage: serializing annotation" << qobject_cast<Annotation*>(annotation)->annotationName();
            serializeElement(annotation, stream, indent + 1);
        }
        
        // 序列化子元素（元素之间不加空行）
        for (Element* child : portUsage->ownedElements()) {
            // 跳过注解，因为已经序列化了
            if (child->elementType() == "Annotation") {
                continue;
            }
            serializeElement(child, stream, indent + 1);
        }
        
        stream << indentStr << "}\n";
    } else {
        stream << ";\n";
    }
}

void SysMLSerializer::serializeConnectorUsage(ConnectorUsage* connectorUsage, QTextStream& stream, int indent)
{
    if (!connectorUsage) {
        return;
    }
    
    QString indentStr = getIndent(indent);
    QString name = connectorUsage->declaredName();
    if (name.isEmpty()) {
        name = "UnnamedConnector";
    }
    
    stream << indentStr << "connector " << name;
    
    Type* type = findUsageType(connectorUsage);
    if (type) {
        QString typeName = getQualifiedName(type);
        if (!typeName.isEmpty()) {
            stream << " : " << typeName;
        }
    } else {
        QString typeName = connectorUsage->typeName();
        if (!typeName.isEmpty()) {
            stream << " : " << typeName;
        }
    }
    
    stream << " {\n";
    
    // 序列化子元素（元素之间不加空行）
    for (Element* child : connectorUsage->ownedElements()) {
        serializeElement(child, stream, indent + 1);
    }
    
    stream << indentStr << "}\n";
}

void SysMLSerializer::serializeFlowUsage(FlowUsage* flowUsage, QTextStream& stream, int indent)
{
    if (!flowUsage) {
        return;
    }
    
    QString indentStr = getIndent(indent);
    QString name = flowUsage->declaredName();
    if (name.isEmpty()) {
        name = "UnnamedFlow";
    }
    
    stream << indentStr << "flow " << name;
    
    Type* type = findUsageType(flowUsage);
    if (type) {
        QString typeName = getQualifiedName(type);
        if (!typeName.isEmpty()) {
            stream << " : " << typeName;
        }
    } else {
        QString typeName = flowUsage->typeName();
        if (!typeName.isEmpty()) {
            stream << " : " << typeName;
        }
    }
    
    stream << " {\n";
    
    // 序列化子元素（元素之间不加空行）
    for (Element* child : flowUsage->ownedElements()) {
        serializeElement(child, stream, indent + 1);
    }
    
    stream << indentStr << "}\n";
}

void SysMLSerializer::serializeFeature(Feature* feature, QTextStream& stream, int indent)
{
    if (!feature) {
        return;
    }
    
    QString indentStr = getIndent(indent);
    QString name = feature->declaredName();
    if (name.isEmpty()) {
        name = "UnnamedFeature";
    }
    
    // 检查重定义关系（如果存在，可能没有标识符）
    QList<Redefinition*> redefinitions = findRedefinitions(feature);
    bool hasRedefinition = !redefinitions.isEmpty();
    
    // 检查是否为简化属性
    bool isSimplified = feature->isSimplifiedAttribute();
    
    // 如果不是简化属性，输出 attribute 关键字
    if (!isSimplified) {
        stream << indentStr << "attribute ";
    } else {
        stream << indentStr;
    }
    
    // 序列化特征前缀（按标准顺序：abstract, const, derived, direction, ref）
    QStringList prefixes;
    if (feature->isAbstract()) {
        prefixes.append("abstract");
    }
    if (feature->isConst()) {
        prefixes.append("const");
    }
    if (feature->isDerived()) {
        prefixes.append("derived");
    }
    if (feature->direction() == Feature::DirectionIn) {
        prefixes.append("in");
    } else if (feature->direction() == Feature::DirectionOut) {
        prefixes.append("out");
    } else if (feature->direction() == Feature::DirectionInOut) {
        prefixes.append("inout");
    }
    if (feature->isRef()) {
        prefixes.append("ref");
    }
    
    if (!prefixes.isEmpty()) {
        stream << prefixes.join(" ") << " ";
    }
    
    // 输出标识符（如果有）
    if (!name.isEmpty()) {
        stream << name;
    }
    
    // 序列化重定义关系（使用 redefines 关键字）
    // 注意：始终使用 redefines 关键字形式，不使用 :>> 操作符
    // 因为 :>> 操作符在 attribute 上下文中可能不够清晰
    if (hasRedefinition) {
        Redefinition* redefinition = redefinitions.first(); // 通常只有一个
        Feature* redefined = redefinition->redefinedFeature();
        if (redefined) {
            stream << " redefines " << getQualifiedName(redefined);
        } else if (!redefinition->redefinedFeatureName().isEmpty()) {
            stream << " redefines " << redefinition->redefinedFeatureName();
        }
    }
    
    // 序列化子集化关系（在重定义之后，类型化之前）
    QList<Subsetting*> subsets = findSubsettings(feature);
    if (!subsets.isEmpty()) {
        Subsetting* subsetting = subsets.first(); // 通常只有一个
        Feature* subsetted = subsetting->subsettedFeature();
        if (subsetted) {
            stream << " :> " << getQualifiedName(subsetted);
        } else if (!subsetting->subsettedFeatureName().isEmpty()) {
            stream << " :> " << subsetting->subsettedFeatureName();
        }
    }
    
    // 序列化类型（查找 FeatureTyping 关系）
    Type* type = findFeatureType(feature);
    QString typeName;
    bool useSubclassOperator = false; // 是否使用 :> 而不是 :
    
    if (type) {
        // 如果找到了类型，使用类型的限定名
        typeName = getQualifiedName(type);
    } else {
        // 首先从 Feature 自己的 ownedRelationships 中查找（FeatureTyping 现在在 Feature 中）
        for (Relationship* rel : feature->ownedRelationships()) {
            FeatureTyping* featureTyping = qobject_cast<FeatureTyping*>(rel);
            if (featureTyping && featureTyping->typedFeature() == feature) {
                typeName = featureTyping->typeName();
                // 检查是否有 Subsetting 关系：如果没有 Subsetting，说明 :> 是作为类型化使用的
                // 在 attribute 上下文中，:> 表示类型化，应该导出为 :>
                if (subsets.isEmpty()) {
                    useSubclassOperator = true;
                }
                break;
            }
        }
        
        // 如果在 Feature 自己的关系中没找到，尝试从拥有者的关系中查找（向后兼容）
        if (typeName.isEmpty()) {
            Element* owner = feature->owner();
            if (owner) {
                // 在拥有者的关系中查找
                for (Relationship* rel : owner->ownedRelationships()) {
                    FeatureTyping* featureTyping = qobject_cast<FeatureTyping*>(rel);
                    if (featureTyping && featureTyping->typedFeature() == feature) {
                        typeName = featureTyping->typeName();
                        // 检查是否有 Subsetting 关系
                        if (subsets.isEmpty()) {
                            useSubclassOperator = true;
                        }
                        break;
                    }
                }
                
                // 如果在直接所有者中没找到，在父命名空间中查找
                if (typeName.isEmpty()) {
                    Element* current = owner->owner();
                    while (current) {
                        for (Relationship* rel : current->ownedRelationships()) {
                            FeatureTyping* featureTyping = qobject_cast<FeatureTyping*>(rel);
                            if (featureTyping && featureTyping->typedFeature() == feature) {
                                typeName = featureTyping->typeName();
                                // 检查是否有 Subsetting 关系
                                if (subsets.isEmpty()) {
                                    useSubclassOperator = true;
                                }
                                break;
                            }
                        }
                        if (!typeName.isEmpty()) {
                            break;
                        }
                        current = current->owner();
                    }
                }
            }
        }
    }
    
    // 输出类型（如果有）
    if (!typeName.isEmpty()) {
        if (useSubclassOperator) {
            stream << " :> " << typeName;
        } else {
            stream << " : " << typeName;
        }
    }
    
    // 序列化值表达式（如果有）
    QString valueExpr = feature->valueExpression();
    if (!valueExpr.isEmpty()) {
        stream << " = " << valueExpr;
    }
    
    stream << ";\n";
}

void SysMLSerializer::serializeImport(Import* import, QTextStream& stream, int indent)
{
    if (!import) {
        return;
    }
    
    QString indentStr = getIndent(indent);
    
    if (auto nsImport = qobject_cast<NamespaceImport*>(import)) {
        // 序列化导入前缀（可见性）
        QStringList prefixParts;
        if (nsImport->visibility() == NamespaceImport::VisibilityPublic) {
            prefixParts.append("public");
        } else if (nsImport->visibility() == NamespaceImport::VisibilityPrivate) {
            prefixParts.append("private");
        } else if (nsImport->visibility() == NamespaceImport::VisibilityProtected) {
            prefixParts.append("protected");
        }
        
        prefixParts.append("import");
        
        if (nsImport->isAll()) {
            prefixParts.append("all");
        }
        
        QString prefix = prefixParts.join(" ");
        
        // 首先尝试获取导入的命名空间
        Namespace* importedNs = nsImport->importedNamespace();
        QString namespaceName;
        
        if (importedNs) {
            // 尝试获取限定名
            Element* nsElement = dynamic_cast<Element*>(importedNs);
            if (nsElement) {
                namespaceName = getQualifiedName(nsElement);
                if (namespaceName.isEmpty()) {
                    // 如果限定名为空，尝试使用声明名
                    namespaceName = nsElement->declaredName();
                }
            }
        }
        
        // 如果无法获取命名空间，使用保存的原始名称
        if (namespaceName.isEmpty()) {
            namespaceName = nsImport->importedNamespaceName();
        }
        
        if (namespaceName.isEmpty()) {
            qWarning() << "[SysMLSerializer] Cannot determine imported namespace name for import";
            namespaceName = "UnknownNamespace";
        }
        
        // 添加通配符
        if (nsImport->isRecursive()) {
            namespaceName += "::**";
        } else if (nsImport->isWildcard()) {
            namespaceName += "::*";
        }
        
        stream << indentStr << prefix << " " << namespaceName << ";\n";
    } else {
        // 其他类型的 Import（如果有）
        qWarning() << "[SysMLSerializer] Unknown import type";
    }
}

QString SysMLSerializer::getIndent(int level)
{
    return QString(" ").repeated(level * m_indentSize);
}

QString SysMLSerializer::escapeString(const QString& str)
{
    if (str.isEmpty()) {
        return str;
    }
    
    QString result;
    result.reserve(str.length() + 10); // 预分配空间
    
    for (QChar ch : str) {
        ushort unicode = ch.unicode();
        
        // 转义特殊字符
        switch (unicode) {
            case '\\':
                result += "\\\\";
                break;
            case '\"':
                result += "\\\"";
                break;
            case '\'':
                result += "\\'";
                break;
            case '\n':
                result += "\\n";
                break;
            case '\r':
                result += "\\r";
                break;
            case '\t':
                result += "\\t";
                break;
            default:
                // 对于 SysML 标识符，通常只需要字母、数字、下划线
                // 如果包含其他字符，可能需要转义或使用引号
                // 这里暂时不转义，因为 SysML 语法只允许 [a-zA-Z_][a-zA-Z0-9_]*
                // 如果将来需要支持字符串字面量，可以在这里添加转义逻辑
                result += ch;
                break;
        }
    }
    
    return result;
}

Type* SysMLSerializer::findFeatureType(Feature* feature)
{
    if (!feature) {
        return nullptr;
    }
    
    // 查找特征的所有者（可能是 Type 或其他 Element）
    Element* owner = feature->owner();
    if (!owner) {
        return nullptr;
    }
    
    // 在拥有者的关系中查找 FeatureTyping
    // FeatureTyping 关系存储在包含该 Feature 的命名空间中
    for (Relationship* rel : owner->ownedRelationships()) {
        FeatureTyping* featureTyping = qobject_cast<FeatureTyping*>(rel);
        if (featureTyping && featureTyping->typedFeature() == feature) {
            Type* type = featureTyping->type();
            if (type) {
                return type;
            }
            // 如果类型为 nullptr，但保存了类型名称，返回 nullptr（调用者会使用类型名称）
            // 这里返回 nullptr，让调用者使用 featureTyping->typeName()
        }
    }
    
    // 如果直接所有者没有找到，尝试在父命名空间中查找
    // （某些情况下关系可能存储在父命名空间中）
    Element* current = owner->owner();
    while (current) {
        for (Relationship* rel : current->ownedRelationships()) {
            FeatureTyping* featureTyping = qobject_cast<FeatureTyping*>(rel);
            if (featureTyping && featureTyping->typedFeature() == feature) {
                Type* type = featureTyping->type();
                if (type) {
                    return type;
                }
                // 如果类型为 nullptr，但保存了类型名称，返回 nullptr（调用者会使用类型名称）
            }
        }
        current = current->owner();
    }
    
    return nullptr;
}

Type* SysMLSerializer::findPartUsageType(PartUsage* partUsage)
{
    if (!partUsage) {
        return nullptr;
    }
    
    // 首先尝试将 PartUsage 转换为 Feature
    // 在 SysML v2 中，PartUsage 应该是一种 Feature
    Feature* feature = qobject_cast<Feature*>(partUsage);
    if (feature) {
        // 如果可以转换，使用 FeatureTyping 查找
        return findFeatureType(feature);
    }
    
    // 如果 PartUsage 不是 Feature，我们需要查找它的类型化关系
    // 在当前的实现中，PartUsage 的类型化可能通过以下方式存储：
    // 1. 通过 FeatureTyping 关系（如果 PartUsage 可以被视为 Feature）
    // 2. 通过其他类型的关系（如 Subclassification，但那是用于类型继承的）
    
    // 查找 PartUsage 的所有者
    Element* owner = partUsage->owner();
    if (!owner) {
        return nullptr;
    }
    
    // 在拥有者的关系中查找 FeatureTyping
    // 注意：我们需要找到指向这个 PartUsage 的 FeatureTyping
    // 但由于 PartUsage 不是 Feature，我们需要其他匹配方式
    
    // 方法1: 查找所有 FeatureTyping，检查 typedFeature 是否指向同一个对象
    // 由于 PartUsage 不是 Feature，我们需要使用 dynamic_cast 或指针比较
    // 但更安全的方法是检查指针地址（如果 PartUsage 可以被视为 Feature）
    for (Relationship* rel : owner->ownedRelationships()) {
        FeatureTyping* featureTyping = qobject_cast<FeatureTyping*>(rel);
        if (featureTyping) {
            Feature* typedFeature = featureTyping->typedFeature();
            if (typedFeature) {
                // 尝试将 PartUsage 转换为 Feature 进行比较
                // 或者直接比较指针地址（如果它们指向同一个对象）
                Element* typedElement = dynamic_cast<Element*>(typedFeature);
                Element* partUsageElement = dynamic_cast<Element*>(partUsage);
                if (typedElement == partUsageElement) {
                    return featureTyping->type();
                }
            }
        }
    }
    
    // 方法2: 在父命名空间中查找
    Element* current = owner->owner();
    while (current) {
        for (Relationship* rel : current->ownedRelationships()) {
            FeatureTyping* featureTyping = qobject_cast<FeatureTyping*>(rel);
            if (featureTyping) {
                Feature* typedFeature = featureTyping->typedFeature();
                if (typedFeature) {
                    Element* typedElement = dynamic_cast<Element*>(typedFeature);
                    Element* partUsageElement = dynamic_cast<Element*>(partUsage);
                    if (typedElement == partUsageElement) {
                        return featureTyping->type();
                    }
                }
            }
        }
        current = current->owner();
    }
    
    // 方法3: 如果 PartUsage 有名称，尝试通过名称匹配
    // 这需要遍历所有可能的 FeatureTyping，找到名称匹配的
    // 但这种方法不够可靠，因为可能有多个同名的特征
    
    // 注意：在当前的元模型实现中，PartUsage 的类型化可能无法通过 FeatureTyping 找到
    // 这是因为 PartUsage 继承自 Type 而不是 Feature
    // 在 SysML v2 规范中，PartUsage 应该是一种 Feature，所以这个限制是元模型的问题
    // 暂时返回 nullptr，等待元模型调整或实现其他类型化机制
    
    return nullptr;
}

Type* SysMLSerializer::findUsageType(Type* usage)
{
    if (!usage) {
        return nullptr;
    }
    
    // 首先尝试将 Usage 转换为 Feature
    Feature* feature = qobject_cast<Feature*>(usage);
    if (feature) {
        return findFeatureType(feature);
    }
    
    // 如果 Usage 不是 Feature，查找它的类型化关系
    Element* owner = usage->owner();
    if (!owner) {
        return nullptr;
    }
    
    // 在拥有者的关系中查找 FeatureTyping
    for (Relationship* rel : owner->ownedRelationships()) {
        FeatureTyping* featureTyping = qobject_cast<FeatureTyping*>(rel);
        if (featureTyping) {
            Feature* typedFeature = featureTyping->typedFeature();
            if (typedFeature) {
                Element* typedElement = dynamic_cast<Element*>(typedFeature);
                Element* usageElement = dynamic_cast<Element*>(usage);
                if (typedElement == usageElement) {
                    return featureTyping->type();
                }
            }
        }
    }
    
    // 在父命名空间中查找
    Element* current = owner->owner();
    while (current) {
        for (Relationship* rel : current->ownedRelationships()) {
            FeatureTyping* featureTyping = qobject_cast<FeatureTyping*>(rel);
            if (featureTyping) {
                Feature* typedFeature = featureTyping->typedFeature();
                if (typedFeature) {
                    Element* typedElement = dynamic_cast<Element*>(typedFeature);
                    Element* usageElement = dynamic_cast<Element*>(usage);
                    if (typedElement == usageElement) {
                        return featureTyping->type();
                    }
                }
            }
        }
        current = current->owner();
    }
    
    return nullptr;
}

QString SysMLSerializer::getQualifiedName(Element* element)
{
    if (!element) {
        return QString();
    }
    
    // 首先尝试使用 Element 的 qualifiedName() 方法
    // 这个方法应该已经实现了从元素到根元素的路径构建
    QString qualifiedName = element->qualifiedName();
    if (!qualifiedName.isEmpty()) {
        return qualifiedName;
    }
    
    // 如果 qualifiedName() 返回空（不应该发生，但为了健壮性），手动构建
    QStringList parts;
    Element* current = element;
    
    // 从当前元素向上遍历到根元素
    while (current) {
        QString name = current->declaredName();
        if (!name.isEmpty()) {
            parts.prepend(name);
        }
        
        // 移动到父元素
        current = current->owner();
        
        // 避免循环引用（虽然不应该发生）
        if (current == element) {
            qWarning() << "[SysMLSerializer] Circular ownership detected for element:"
                       << element->declaredName();
            break;
        }
    }
    
    // 如果没有任何部分，返回声明名
    if (parts.isEmpty()) {
        QString name = element->declaredName();
        return name.isEmpty() ? "UnnamedElement" : name;
    }
    
    return parts.join("::");
}

// ============================================
// 新定义类型的序列化方法
// ============================================

void SysMLSerializer::serializeActionDefinition(ActionDefinition* actionDef, QTextStream& stream, int indent)
{
    if (!actionDef) {
        return;
    }
    
    QString indentStr = getIndent(indent);
    QString name = actionDef->declaredName();
    if (name.isEmpty()) {
        name = "UnnamedAction";
    }
    
    // 序列化定义前缀
    QStringList prefixes;
    if (actionDef->isAbstract()) {
        prefixes.append("abstract");
    }
    if (actionDef->isVariation()) {
        prefixes.append("variation");
    }
    if (actionDef->isIndividual()) {
        prefixes.append("individual");
    }
    
    if (!prefixes.isEmpty()) {
        stream << indentStr << prefixes.join(" ") << " ";
    }
    
    stream << "action def " << name;
    serializeSubclassifications(actionDef, stream);
    
    bool hasChildren = !actionDef->ownedElements().isEmpty();
    if (hasChildren) {
        stream << " {\n";
        
        // 序列化子元素（元素之间不加空行）
        for (Element* child : actionDef->ownedElements()) {
            serializeElement(child, stream, indent + 1);
        }
        
        stream << indentStr << "}\n";
    } else {
        stream << ";\n";
    }
}

void SysMLSerializer::serializeStateDefinition(StateDefinition* stateDef, QTextStream& stream, int indent)
{
    if (!stateDef) {
        return;
    }
    
    QString indentStr = getIndent(indent);
    QString name = stateDef->declaredName();
    if (name.isEmpty()) {
        name = "UnnamedState";
    }
    
    // 序列化定义前缀
    QStringList prefixes;
    if (stateDef->isAbstract()) {
        prefixes.append("abstract");
    }
    if (stateDef->isVariation()) {
        prefixes.append("variation");
    }
    if (stateDef->isIndividual()) {
        prefixes.append("individual");
    }
    
    if (!prefixes.isEmpty()) {
        stream << indentStr << prefixes.join(" ") << " ";
    }
    
    stream << "state def " << name;
    serializeSubclassifications(stateDef, stream);
    
    bool hasChildren = !stateDef->ownedElements().isEmpty();
    if (hasChildren) {
        stream << " {\n";
        
        // 序列化子元素（按逻辑顺序：ref statements, states, actions, constraints, transitions）
        QList<Element*> refs;
        QList<Element*> states;
        QList<Element*> actions;
        QList<Element*> constraints;
        QList<Element*> transitions;
        QList<Element*> others;
        
        for (Element* child : stateDef->ownedElements()) {
            QString childType = child->elementType();
            if (childType == "PartUsage") {
                PartUsage* partUsage = qobject_cast<PartUsage*>(child);
                if (partUsage && partUsage->isRef()) {
                    refs.append(child);
                    continue;
                }
            } else if (childType == "StateDefinition" || childType == "StateUsage") {
                states.append(child);
                continue;
            } else if (childType == "StateActionMember") {
                actions.append(child);
                continue;
            } else if (childType == "ConstraintStatement") {
                constraints.append(child);
                continue;
            } else if (childType == "Transition") {
                transitions.append(child);
                continue;
            }
            others.append(child);
        }
        
        // 导出各组
        // 导出各组（元素之间不加空行）
        for (Element* child : refs) {
            serializeElement(child, stream, indent + 1);
        }
        
        for (Element* child : states) {
            serializeElement(child, stream, indent + 1);
        }
        
        for (Element* child : actions) {
            serializeElement(child, stream, indent + 1);
        }
        
        for (Element* child : constraints) {
            serializeElement(child, stream, indent + 1);
        }
        
        for (Element* child : transitions) {
            serializeElement(child, stream, indent + 1);
        }
        
        for (Element* child : others) {
            serializeElement(child, stream, indent + 1);
        }
        
        stream << indentStr << "}\n";
    } else {
        stream << ";\n";
    }
}

void SysMLSerializer::serializeRequirementDefinition(RequirementDefinition* reqDef, QTextStream& stream, int indent)
{
    if (!reqDef) {
        return;
    }
    
    QString indentStr = getIndent(indent);
    QString name = reqDef->declaredName();
    if (name.isEmpty()) {
        name = "UnnamedRequirement";
    }
    
    // 序列化定义前缀
    QStringList prefixes;
    if (reqDef->isAbstract()) {
        prefixes.append("abstract");
    }
    if (reqDef->isVariation()) {
        prefixes.append("variation");
    }
    if (reqDef->isIndividual()) {
        prefixes.append("individual");
    }
    
    if (!prefixes.isEmpty()) {
        stream << indentStr << prefixes.join(" ") << " ";
    } else {
        stream << indentStr;
    }
    
    stream << "requirement def " << name;
    serializeSubclassifications(reqDef, stream);
    
    bool hasChildren = !reqDef->ownedElements().isEmpty();
    if (hasChildren) {
        stream << " {\n";
        
        // 分离不同类型的子元素
        QList<Element*> regularElements;
        QList<Element*> requirementMembers; // subject, actor, stakeholder
        
        for (Element* child : reqDef->ownedElements()) {
            QString childName = child->declaredName();
            // 检查是否是 requirement member（格式为 "subject name", "actor name", "stakeholder name"）
            if (childName.startsWith("subject ") || childName.startsWith("actor ") || childName.startsWith("stakeholder ")) {
                requirementMembers.append(child);
            } else {
                regularElements.append(child);
            }
        }
        
        // 先序列化常规元素（元素之间不加空行）
        for (Element* child : regularElements) {
            serializeElement(child, stream, indent + 1);
        }
        
        // 然后序列化 requirement members（元素之间不加空行）
        for (Element* child : requirementMembers) {
            serializeRequirementMember(child, stream, indent + 1);
        }
        
        stream << indentStr << "}\n";
    } else {
        stream << ";\n";
    }
}

void SysMLSerializer::serializeConstraintDefinition(ConstraintDefinition* constraintDef, QTextStream& stream, int indent)
{
    if (!constraintDef) {
        return;
    }
    
    QString indentStr = getIndent(indent);
    QString name = constraintDef->declaredName();
    if (name.isEmpty()) {
        name = "UnnamedConstraint";
    }
    
    // 序列化定义前缀
    QStringList prefixes;
    if (constraintDef->isAbstract()) {
        prefixes.append("abstract");
    }
    if (constraintDef->isVariation()) {
        prefixes.append("variation");
    }
    if (constraintDef->isIndividual()) {
        prefixes.append("individual");
    }
    
    if (!prefixes.isEmpty()) {
        stream << indentStr << prefixes.join(" ") << " ";
    }
    
    stream << "constraint def " << name;
    serializeSubclassifications(constraintDef, stream);
    
    bool hasChildren = !constraintDef->ownedElements().isEmpty();
    if (hasChildren) {
        stream << " {\n";
        
        // 序列化子元素（元素之间不加空行）
        for (Element* child : constraintDef->ownedElements()) {
            serializeElement(child, stream, indent + 1);
        }
        
        stream << indentStr << "}\n";
    } else {
        stream << ";\n";
    }
}

void SysMLSerializer::serializePortDefinition(PortDefinition* portDef, QTextStream& stream, int indent)
{
    if (!portDef) {
        return;
    }
    
    QString indentStr = getIndent(indent);
    QString name = portDef->declaredName();
    if (name.isEmpty()) {
        name = "UnnamedPort";
    }
    
    stream << indentStr << "port def " << name;
    serializeSubclassifications(portDef, stream);
    stream << " {\n";
    
    // 序列化子元素
    // 序列化子元素（元素之间不加空行）
    for (Element* child : portDef->ownedElements()) {
        serializeElement(child, stream, indent + 1);
    }
    
    stream << indentStr << "}\n";
}

void SysMLSerializer::serializeConnectorDefinition(ConnectorDefinition* connDef, QTextStream& stream, int indent)
{
    if (!connDef) {
        return;
    }
    
    QString indentStr = getIndent(indent);
    QString name = connDef->declaredName();
    if (name.isEmpty()) {
        name = "UnnamedConnector";
    }
    
    stream << indentStr << "connector def " << name;
    serializeSubclassifications(connDef, stream);
    stream << " {\n";
    
    // 序列化子元素
    // 序列化子元素（元素之间不加空行）
    for (Element* child : connDef->ownedElements()) {
        serializeElement(child, stream, indent + 1);
    }
    
    stream << indentStr << "}\n";
}

void SysMLSerializer::serializeInterfaceDefinition(InterfaceDefinition* ifaceDef, QTextStream& stream, int indent)
{
    if (!ifaceDef) {
        return;
    }
    
    QString indentStr = getIndent(indent);
    QString name = ifaceDef->declaredName();
    if (name.isEmpty()) {
        name = "UnnamedInterface";
    }
    
    stream << indentStr << "interface def " << name;
    serializeSubclassifications(ifaceDef, stream);
    stream << " {\n";
    
    // 序列化子元素
    // 序列化子元素（元素之间不加空行）
    for (Element* child : ifaceDef->ownedElements()) {
        serializeElement(child, stream, indent + 1);
    }
    
    stream << indentStr << "}\n";
}

void SysMLSerializer::serializeDataTypeDefinition(DataTypeDefinition* dtDef, QTextStream& stream, int indent)
{
    if (!dtDef) {
        return;
    }
    
    QString indentStr = getIndent(indent);
    QString name = dtDef->declaredName();
    if (name.isEmpty()) {
        name = "UnnamedDataType";
    }
    
    stream << indentStr << "datatype def " << name;
    serializeSubclassifications(dtDef, stream);
    stream << " {\n";
    
    // 序列化子元素（特征）
    // 序列化子元素（元素之间不加空行）
    for (Element* child : dtDef->ownedElements()) {
        serializeElement(child, stream, indent + 1);
    }
    
    stream << indentStr << "}\n";
}

void SysMLSerializer::serializeClassDefinition(ClassDefinition* classDef, QTextStream& stream, int indent)
{
    if (!classDef) {
        return;
    }
    
    QString indentStr = getIndent(indent);
    QString name = classDef->declaredName();
    if (name.isEmpty()) {
        name = "UnnamedClass";
    }
    
    stream << indentStr << "class def " << name;
    serializeSubclassifications(classDef, stream);
    stream << " {\n";
    
    // 序列化子元素
    for (Element* child : classDef->ownedElements()) {
        serializeElement(child, stream, indent + 1);
        stream << "\n";
    }
    
    stream << indentStr << "}\n";
}

void SysMLSerializer::serializeEnumDefinition(EnumDefinition* enumDef, QTextStream& stream, int indent)
{
    if (!enumDef) {
        return;
    }
    
    QString indentStr = getIndent(indent);
    QString name = enumDef->declaredName();
    if (name.isEmpty()) {
        name = "UnnamedEnum";
    }
    
    stream << indentStr << "enum def " << name << " {\n";
    
    // 序列化枚举值（使用 Feature 表示）
    for (Element* child : enumDef->ownedElements()) {
        if (auto feature = qobject_cast<Feature*>(child)) {
            QString enumValueName = feature->declaredName();
            stream << indentStr << "    " << enumValueName << ";\n";
        }
    }
    
    stream << indentStr << "}\n";
}

void SysMLSerializer::serializeAttributeDefinition(AttributeDefinition* attrDef, QTextStream& stream, int indent)
{
    if (!attrDef) {
        return;
    }
    
    QString indentStr = getIndent(indent);
    QString name = attrDef->declaredName();
    if (name.isEmpty()) {
        name = "UnnamedAttribute";
    }
    
    // 序列化定义前缀
    QStringList prefixes;
    if (attrDef->isAbstract()) {
        prefixes.append("abstract");
    }
    if (attrDef->isVariation()) {
        prefixes.append("variation");
    }
    if (attrDef->isIndividual()) {
        prefixes.append("individual");
    }
    
    if (!prefixes.isEmpty()) {
        stream << indentStr << prefixes.join(" ") << " ";
    }
    
    stream << "attribute def " << name;
    serializeSubclassifications(attrDef, stream);
    
    bool hasChildren = !attrDef->ownedElements().isEmpty();
    if (hasChildren) {
        stream << " {\n";
        
        for (Element* child : attrDef->ownedElements()) {
            serializeElement(child, stream, indent + 1);
            stream << "\n";
        }
        
        stream << indentStr << "}\n";
    } else {
        stream << ";\n";
    }
}

void SysMLSerializer::serializeConnectionDefinition(ConnectionDefinition* connDef, QTextStream& stream, int indent)
{
    if (!connDef) {
        return;
    }
    
    QString indentStr = getIndent(indent);
    QString name = connDef->declaredName();
    if (name.isEmpty()) {
        name = "UnnamedConnection";
    }
    
    // 序列化定义前缀
    QStringList prefixes;
    if (connDef->isAbstract()) {
        prefixes.append("abstract");
    }
    if (connDef->isVariation()) {
        prefixes.append("variation");
    }
    if (connDef->isIndividual()) {
        prefixes.append("individual");
    }
    
    if (!prefixes.isEmpty()) {
        stream << indentStr << prefixes.join(" ") << " ";
    }
    
    stream << "connection def " << name;
    serializeSubclassifications(connDef, stream);
    
    bool hasChildren = !connDef->ownedElements().isEmpty();
    if (hasChildren) {
        stream << " {\n";
        
        for (Element* child : connDef->ownedElements()) {
            serializeElement(child, stream, indent + 1);
            stream << "\n";
        }
        
        stream << indentStr << "}\n";
    } else {
        stream << ";\n";
    }
}

void SysMLSerializer::serializeConnectionUsage(ConnectionUsage* connUsage, QTextStream& stream, int indent)
{
    if (!connUsage) {
        return;
    }
    
    QString indentStr = getIndent(indent);
    QString name = connUsage->declaredName();
    if (name.isEmpty()) {
        name = "UnnamedConnection";
    }
    
    stream << indentStr << "connection " << name;
    
    // 序列化类型
    Type* type = findUsageType(connUsage);
    if (type) {
        QString typeName = getQualifiedName(type);
        if (!typeName.isEmpty()) {
            stream << " : " << typeName;
        }
    } else {
        QString typeName = connUsage->typeName();
        if (!typeName.isEmpty()) {
            stream << " : " << typeName;
        }
    }
    
    bool hasChildren = !connUsage->ownedElements().isEmpty();
    if (hasChildren) {
        stream << " {\n";
        
        for (Element* child : connUsage->ownedElements()) {
            serializeElement(child, stream, indent + 1);
            stream << "\n";
        }
        
        stream << indentStr << "}\n";
    } else {
        stream << ";\n";
    }
}

void SysMLSerializer::serializeAllocationDefinition(AllocationDefinition* allocDef, QTextStream& stream, int indent)
{
    if (!allocDef) {
        return;
    }
    
    QString indentStr = getIndent(indent);
    QString name = allocDef->declaredName();
    if (name.isEmpty()) {
        name = "UnnamedAllocation";
    }
    
    // 序列化定义前缀
    QStringList prefixes;
    if (allocDef->isAbstract()) {
        prefixes.append("abstract");
    }
    if (allocDef->isVariation()) {
        prefixes.append("variation");
    }
    if (allocDef->isIndividual()) {
        prefixes.append("individual");
    }
    
    if (!prefixes.isEmpty()) {
        stream << indentStr << prefixes.join(" ") << " ";
    }
    
    stream << "allocation def " << name;
    serializeSubclassifications(allocDef, stream);
    
    bool hasChildren = !allocDef->ownedElements().isEmpty();
    if (hasChildren) {
        stream << " {\n";
        
        for (Element* child : allocDef->ownedElements()) {
            serializeElement(child, stream, indent + 1);
            stream << "\n";
        }
        
        stream << indentStr << "}\n";
    } else {
        stream << ";\n";
    }
}

void SysMLSerializer::serializeAllocationUsage(AllocationUsage* allocUsage, QTextStream& stream, int indent)
{
    if (!allocUsage) {
        return;
    }
    
    QString indentStr = getIndent(indent);
    QString name = allocUsage->declaredName();
    if (name.isEmpty()) {
        name = "UnnamedAllocation";
    }
    
    stream << indentStr << "allocation " << name;
    
    // 序列化类型
    Type* type = findUsageType(allocUsage);
    if (type) {
        QString typeName = getQualifiedName(type);
        if (!typeName.isEmpty()) {
            stream << " : " << typeName;
        }
    } else {
        QString typeName = allocUsage->typeName();
        if (!typeName.isEmpty()) {
            stream << " : " << typeName;
        }
    }
    
    bool hasChildren = !allocUsage->ownedElements().isEmpty();
    if (hasChildren) {
        stream << " {\n";
        
        for (Element* child : allocUsage->ownedElements()) {
            serializeElement(child, stream, indent + 1);
            stream << "\n";
        }
        
        stream << indentStr << "}\n";
    } else {
        stream << ";\n";
    }
}

void SysMLSerializer::serializeItemDefinition(ItemDefinition* itemDef, QTextStream& stream, int indent)
{
    if (!itemDef) {
        return;
    }
    
    QString indentStr = getIndent(indent);
    QString name = itemDef->declaredName();
    if (name.isEmpty()) {
        name = "UnnamedItem";
    }
    
    // 序列化定义前缀
    QStringList prefixes;
    if (itemDef->isAbstract()) {
        prefixes.append("abstract");
    }
    if (itemDef->isVariation()) {
        prefixes.append("variation");
    }
    if (itemDef->isIndividual()) {
        prefixes.append("individual");
    }
    
    if (!prefixes.isEmpty()) {
        stream << indentStr << prefixes.join(" ") << " ";
    }
    
    stream << "item def " << name;
    serializeSubclassifications(itemDef, stream);
    
    bool hasChildren = !itemDef->ownedElements().isEmpty();
    if (hasChildren) {
        stream << " {\n";
        
        for (Element* child : itemDef->ownedElements()) {
            serializeElement(child, stream, indent + 1);
            stream << "\n";
        }
        
        stream << indentStr << "}\n";
    } else {
        stream << ";\n";
    }
}

void SysMLSerializer::serializeItemUsage(ItemUsage* itemUsage, QTextStream& stream, int indent)
{
    if (!itemUsage) {
        return;
    }
    
    QString indentStr = getIndent(indent);
    QString name = itemUsage->declaredName();
    if (name.isEmpty()) {
        name = "UnnamedItem";
    }
    
    // 序列化使用前缀
    QStringList prefixes;
    if (itemUsage->direction() == ItemUsage::DirectionIn) {
        prefixes.append("in");
    } else if (itemUsage->direction() == ItemUsage::DirectionOut) {
        prefixes.append("out");
    } else if (itemUsage->direction() == ItemUsage::DirectionInOut) {
        prefixes.append("inout");
    }
    if (itemUsage->isDerived()) {
        prefixes.append("derived");
    }
    if (itemUsage->isConstant()) {
        prefixes.append("constant");
    }
    if (itemUsage->isRef()) {
        prefixes.append("ref");
    }
    if (itemUsage->isIndividual()) {
        prefixes.append("individual");
    }
    if (itemUsage->portionKind() == ItemUsage::PortionSnapshot) {
        prefixes.append("snapshot");
    } else if (itemUsage->portionKind() == ItemUsage::PortionTimeslice) {
        prefixes.append("timeslice");
    }
    
    if (!prefixes.isEmpty()) {
        stream << indentStr << prefixes.join(" ") << " ";
    }
    
    stream << "item " << name;
    
    // 序列化类型
    Type* type = findUsageType(itemUsage);
    if (type) {
        QString typeName = getQualifiedName(type);
        if (!typeName.isEmpty()) {
            stream << " : " << typeName;
        }
    } else {
        QString typeName = itemUsage->typeName();
        if (!typeName.isEmpty()) {
            stream << " : " << typeName;
        }
    }
    
    bool hasChildren = !itemUsage->ownedElements().isEmpty();
    if (hasChildren) {
        stream << " {\n";
        
        for (Element* child : itemUsage->ownedElements()) {
            serializeElement(child, stream, indent + 1);
            stream << "\n";
        }
        
        stream << indentStr << "}\n";
    } else {
        stream << ";\n";
    }
}

void SysMLSerializer::serializeEndStatement(EndStatement* endStmt, QTextStream& stream, int indent)
{
    if (!endStmt) {
        return;
    }
    
    QString indentStr = getIndent(indent);
    QString name = endStmt->declaredName();
    QString tag = endStmt->tag();
    QString typeName = endStmt->typeName();
    bool isNegated = endStmt->isNegated();
    bool usesReferences = endStmt->usesReferences();
    
    stream << indentStr << "end";
    if (!tag.isEmpty()) {
        stream << " #" << tag;
    }
    if (!name.isEmpty()) {
        stream << " " << name;
    }
    
    // 根据 usesReferences 标志选择使用 : 还是 ::>
    if (usesReferences) {
        // 使用 ::> 语法（references）
        stream << " ::>";
    } else {
        // 使用 : 语法（typing）
        stream << " :";
        if (isNegated) {
            stream << " ~";
        }
    }
    
    if (!typeName.isEmpty()) {
        stream << " " << typeName;
    }
    stream << ";\n";
}

void SysMLSerializer::serializeBindStatement(BindStatement* bindStmt, QTextStream& stream, int indent)
{
    if (!bindStmt) {
        return;
    }
    
    QString indentStr = getIndent(indent);
    QString sourceEnd = bindStmt->sourceEnd();
    QString targetEnd = bindStmt->targetEnd();
    
    stream << indentStr << "bind " << sourceEnd << " = " << targetEnd << ";\n";
}

void SysMLSerializer::serializeConnectStatement(ConnectStatement* connectStmt, QTextStream& stream, int indent)
{
    if (!connectStmt) {
        return;
    }
    
    QString indentStr = getIndent(indent);
    QStringList ends = connectStmt->ends();
    bool isBinary = connectStmt->isBinary();
    
    stream << indentStr << "connect ";
    
    if (isBinary && ends.size() >= 2) {
        // 二元连接：connect end1 to end2
        stream << ends[0] << " to " << ends[1];
    } else if (ends.size() > 2) {
        // n元连接：connect (end1, end2, end3)
        stream << "(";
        for (int i = 0; i < ends.size(); ++i) {
            if (i > 0) {
                stream << ", ";
            }
            stream << ends[i];
        }
        stream << ")";
    } else if (ends.size() == 2) {
        // 二元连接（fallback）
        stream << ends[0] << " to " << ends[1];
    }
    
    // 如果 connect 语句有子元素或注解，序列化体
    bool hasChildren = !connectStmt->ownedElements().isEmpty();
    QList<Element*> annotations = connectStmt->ownedAnnotations();
    
    qDebug() << "[SysMLSerializer] serializeConnectStatement:" << ends.join(" to ") << "has" << annotations.size() << "annotations," << connectStmt->ownedElements().size() << "children";
    
    if (hasChildren || !annotations.isEmpty()) {
        stream << " {\n";
        
        // 先序列化注解
        for (Element* annotation : annotations) {
            qDebug() << "[SysMLSerializer] serializeConnectStatement: serializing annotation" << qobject_cast<Annotation*>(annotation)->annotationName();
            serializeElement(annotation, stream, indent + 1);
        }
        
        // 序列化子元素
        for (Element* child : connectStmt->ownedElements()) {
            // 跳过注解，因为已经序列化了
            if (child->elementType() == "Annotation") {
                continue;
            }
            serializeElement(child, stream, indent + 1);
            stream << "\n";
        }
        
        stream << indentStr << "}\n";
    } else {
        stream << ";\n";
    }
}

void SysMLSerializer::serializeAllocateStatement(AllocateStatement* allocateStmt, QTextStream& stream, int indent)
{
    if (!allocateStmt) {
        return;
    }
    
    QString indentStr = getIndent(indent);
    QStringList sourceEnds = allocateStmt->sourceEnds();
    QString targetEnd = allocateStmt->targetEnd();
    bool isBinary = allocateStmt->isBinary();
    
    stream << indentStr << "allocate ";
    
    if (isBinary && sourceEnds.size() >= 1) {
        // 二元分配：allocate sourceEnd to targetEnd
        stream << sourceEnds[0] << " to " << targetEnd;
    } else if (sourceEnds.size() > 1) {
        // n元分配：allocate (sourceEnd1, sourceEnd2) to targetEnd
        stream << "(";
        for (int i = 0; i < sourceEnds.size(); ++i) {
            if (i > 0) {
                stream << ", ";
            }
            stream << sourceEnds[i];
        }
        stream << ") to " << targetEnd;
    } else if (sourceEnds.size() == 1) {
        // 二元分配（fallback）
        stream << sourceEnds[0] << " to " << targetEnd;
    }
    
    // 如果 allocate 语句有子元素，序列化体
    bool hasChildren = !allocateStmt->ownedElements().isEmpty();
    if (hasChildren) {
        stream << " {\n";
        
        for (Element* child : allocateStmt->ownedElements()) {
            serializeElement(child, stream, indent + 1);
            stream << "\n";
        }
        
        stream << indentStr << "};\n";
    } else {
        stream << ";\n";
    }
}

void SysMLSerializer::serializeFlowDefinition(FlowDefinition* flowDef, QTextStream& stream, int indent)
{
    if (!flowDef) {
        return;
    }
    
    QString indentStr = getIndent(indent);
    QString name = flowDef->declaredName();
    if (name.isEmpty()) {
        name = "UnnamedFlow";
    }
    
    // 序列化定义前缀
    QStringList prefixes;
    if (flowDef->isAbstract()) {
        prefixes.append("abstract");
    }
    if (flowDef->isVariation()) {
        prefixes.append("variation");
    }
    if (flowDef->isIndividual()) {
        prefixes.append("individual");
    }
    
    if (!prefixes.isEmpty()) {
        stream << indentStr << prefixes.join(" ") << " ";
    }
    
    stream << "flow def " << name;
    serializeSubclassifications(flowDef, stream);
    
    bool hasChildren = !flowDef->ownedElements().isEmpty();
    if (hasChildren) {
        stream << " {\n";
        
        for (Element* child : flowDef->ownedElements()) {
            serializeElement(child, stream, indent + 1);
            stream << "\n";
        }
        
        stream << indentStr << "}\n";
    } else {
        stream << ";\n";
    }
}

void SysMLSerializer::serializeFlowStatement(FlowStatement* flowStmt, QTextStream& stream, int indent)
{
    if (!flowStmt) {
        return;
    }
    
    QString indentStr = getIndent(indent);
    QString fromEnd = flowStmt->fromEnd();
    QString toEnd = flowStmt->toEnd();
    QString payload = flowStmt->payload();
    QString flowName = flowStmt->flowName();
    QString flowType = flowStmt->flowType();
    bool hasFromKeyword = flowStmt->hasFromKeyword();
    
    stream << indentStr << "flow";
    
    // 如果有使用声明（flowName : FlowType）
    if (!flowName.isEmpty()) {
        stream << " " << flowName;
        if (!flowType.isEmpty()) {
            stream << " : " << flowType;
        }
    }
    
    // 如果有载荷
    if (!payload.isEmpty()) {
        stream << " of " << payload;
    }
    
    // 如果有端点
    if (!fromEnd.isEmpty() && !toEnd.isEmpty()) {
        if (hasFromKeyword) {
            stream << " from " << fromEnd << " to " << toEnd;
        } else {
            stream << " " << fromEnd << " to " << toEnd;
        }
    } else if (!fromEnd.isEmpty()) {
        stream << " " << fromEnd;
    }
    
    stream << ";\n";
}

void SysMLSerializer::serializePerformStatement(PerformStatement* performStmt, QTextStream& stream, int indent)
{
    if (!performStmt) {
        return;
    }
    
    QString indentStr = getIndent(indent);
    QString actionRef = performStmt->actionReference();
    bool isActionKeyword = performStmt->isActionKeyword();
    QString actionName = performStmt->actionName();
    QString actionType = performStmt->actionType();
    QString specializationType = performStmt->specializationType();
    
    stream << indentStr << "perform";
    
    if (isActionKeyword) {
        stream << " action";
        if (!actionName.isEmpty()) {
            stream << " " << actionName;
            if (!actionType.isEmpty()) {
                stream << " : " << actionType;
            }
        } else if (!actionRef.isEmpty()) {
            stream << " " << actionRef;
        }
    } else {
        if (!actionRef.isEmpty()) {
            stream << " " << actionRef;
        }
        // 如果有特征特殊化（:> BaseAction）
        if (!specializationType.isEmpty()) {
            stream << " :> " << specializationType;
        }
    }
    
    // 如果 perform 语句有子元素，序列化体
    bool hasChildren = !performStmt->ownedElements().isEmpty();
    if (hasChildren) {
        stream << " {\n";
        
        for (Element* child : performStmt->ownedElements()) {
            serializeElement(child, stream, indent + 1);
            stream << "\n";
        }
        
        stream << indentStr << "}\n";
    } else {
        stream << ";\n";
    }
}

void SysMLSerializer::serializeActionInputOutput(ActionInputOutput* actionIO, QTextStream& stream, int indent)
{
    if (!actionIO) {
        return;
    }
    
    QString indentStr = getIndent(indent);
    bool isInput = actionIO->isInput();
    bool hasItemKeyword = actionIO->hasItemKeyword();
    QString itemName = actionIO->itemName();
    QString itemType = actionIO->itemType();
    QString defaultValue = actionIO->defaultValue();
    QString redefinesName = actionIO->redefinesName();
    QString multiplicity = actionIO->multiplicity();
    
    qDebug() << "[SysMLSerializer] serializeActionInputOutput: itemName:" << itemName 
             << ", itemType:" << itemType << ", defaultValue:" << defaultValue 
             << ", redefinesName:" << redefinesName << ", multiplicity:" << multiplicity
             << ", hasItemKeyword:" << hasItemKeyword;
    
    stream << indentStr;
    if (isInput) {
        stream << "in";
    } else {
        stream << "out";
    }
    
    if (hasItemKeyword) {
        stream << " item";
        qDebug() << "[SysMLSerializer] serializeActionInputOutput: output 'item' keyword";
    } else {
        qDebug() << "[SysMLSerializer] serializeActionInputOutput: hasItemKeyword is false, skipping 'item' keyword";
    }
    
    // 如果有使用声明（itemName : ItemType）
    if (!itemName.isEmpty()) {
        stream << " " << itemName;
        qDebug() << "[SysMLSerializer] serializeActionInputOutput: output itemName:" << itemName;
        
        // 如果有重定义（redefines qualifiedName [multiplicity]）
        if (!redefinesName.isEmpty()) {
            stream << " redefines " << redefinesName;
            qDebug() << "[SysMLSerializer] serializeActionInputOutput: output redefines:" << redefinesName;
            if (!multiplicity.isEmpty()) {
                stream << " " << multiplicity;
                qDebug() << "[SysMLSerializer] serializeActionInputOutput: output multiplicity:" << multiplicity;
            }
        } else {
            qDebug() << "[SysMLSerializer] serializeActionInputOutput: redefinesName is empty, skipping redefines";
        }
        
        // 如果有类型（: ItemType）
        if (!itemType.isEmpty()) {
            stream << " : " << itemType;
            qDebug() << "[SysMLSerializer] serializeActionInputOutput: output itemType:" << itemType;
        } else {
            qDebug() << "[SysMLSerializer] serializeActionInputOutput: itemType is empty, skipping type";
        }
    } else {
        qDebug() << "[SysMLSerializer] serializeActionInputOutput: itemName is empty, skipping itemName block";
    }
    
    // 如果有默认值（= defaultValue）
    if (!defaultValue.isEmpty()) {
        stream << " = " << defaultValue;
        qDebug() << "[SysMLSerializer] serializeActionInputOutput: output defaultValue:" << defaultValue;
    } else {
        qDebug() << "[SysMLSerializer] serializeActionInputOutput: defaultValue is empty, skipping default value";
    }
    
    // 如果 actionInputOutput 有子元素，序列化体
    bool hasChildren = !actionIO->ownedElements().isEmpty();
    if (hasChildren) {
        stream << " {\n";
        
        for (Element* child : actionIO->ownedElements()) {
            serializeElement(child, stream, indent + 1);
            stream << "\n";
        }
        
        stream << indentStr << "}\n";
    } else {
        stream << ";\n";
    }
}

void SysMLSerializer::serializeAssertStatement(AssertStatement* assertStmt, QTextStream& stream, int indent)
{
    if (!assertStmt) {
        return;
    }
    
    QString indentStr = getIndent(indent);
    QString expression = assertStmt->expression();
    
    // 格式化表达式（在运算符前后添加空格）
    QString formattedExpression = formatExpression(expression);
    
    // assert 语句通常是简单的表达式，不需要 constraint {} 包装
    stream << indentStr << "assert " << formattedExpression << ";\n";
}

void SysMLSerializer::serializeRequireStatement(RequireStatement* requireStmt, QTextStream& stream, int indent)
{
    if (!requireStmt) {
        return;
    }
    
    QString indentStr = getIndent(indent);
    QString expression = requireStmt->expression();
    
    // 格式化表达式（在运算符前后添加空格）
    QString formattedExpression = formatExpression(expression);
    
    // 使用标志来判断是否包含 constraint 关键字
    if (requireStmt->hasConstraintKeyword()) {
        // require constraint {expression} 格式
        // 如果表达式已经包含大括号，直接使用；否则添加大括号
        if (formattedExpression.contains("{") && formattedExpression.contains("}")) {
            stream << indentStr << "require constraint " << formattedExpression << ";\n";
        } else {
            stream << indentStr << "require constraint {" << formattedExpression << "};\n";
        }
    } else {
        // require expression; 格式（简单格式）
        stream << indentStr << "require " << formattedExpression << ";\n";
    }
}

void SysMLSerializer::serializeAssumeStatement(AssumeStatement* assumeStmt, QTextStream& stream, int indent)
{
    if (!assumeStmt) {
        return;
    }
    
    QString indentStr = getIndent(indent);
    QString expression = assumeStmt->expression();
    
    // 格式化表达式（在运算符前后添加空格）
    QString formattedExpression = formatExpression(expression);
    
    // assume 语句通常是 constraint {expression} 格式
    // 如果表达式已经包含大括号，直接使用；否则添加大括号
    if (formattedExpression.contains("{") && formattedExpression.contains("}")) {
        stream << indentStr << "assume constraint " << formattedExpression << ";\n";
    } else {
        stream << indentStr << "assume constraint {" << formattedExpression << "};\n";
    }
}

void SysMLSerializer::serializeDocStatement(DocStatement* docStmt, QTextStream& stream, int indent)
{
    if (!docStmt) {
        return;
    }
    
    QString indentStr = getIndent(indent);
    QString content = docStmt->content();
    
    // 序列化为 doc /*content*/ 格式
    if (!content.isEmpty()) {
        stream << indentStr << "doc /*" << content << "*/\n";
    } else {
        stream << indentStr << "doc\n";
    }
}

QString SysMLSerializer::formatExpression(const QString& expression)
{
    if (expression.isEmpty()) {
        return expression;
    }
    
    QString formatted = expression;
    
    // 在运算符前后添加空格（如果还没有）
    // 注意：需要小心处理，避免在字符串字面量中修改
    
    // 二元运算符（> < >= <= == !=）
    formatted.replace(QRegularExpression("([^\\s=!<>])([><=!])([^\\s=])"), "\\1 \\2 \\3");
    formatted.replace(QRegularExpression("([^\\s])([><=!])(=)([^\\s])"), "\\1 \\2\\3 \\4");
    
    // 逻辑运算符（and or not）- 使用 QRegularExpression 对象设置选项
    QRegularExpression logicalOpRegex("([^\\s])(and|or|not)([^\\s])", QRegularExpression::CaseInsensitiveOption);
    formatted.replace(logicalOpRegex, "\\1 \\2 \\3");
    
    return formatted;
}

bool SysMLSerializer::saveToFile(Resource* resource, const QString& filePath)
{
    if (!resource) {
        return false;
    }
    
    QString text = serializeResource(resource);
    
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "[SysMLSerializer] Failed to open file:" << filePath;
        return false;
    }
    
    QTextStream stream(&file);
    stream << text;
    file.close();
    
    qDebug() << "[SysMLSerializer] Saved to file:" << filePath;
    return true;
}

// ============================================
// 关系查找方法实现
// ============================================

QList<Subclassification*> SysMLSerializer::findSubclassifications(Type* type)
{
    QList<Subclassification*> result;
    if (!type) {
        return result;
    }
    
    // 从类型的所有者及其父命名空间查找子类化关系
    Element* searchElement = type->owner();
    while (searchElement) {
        for (Relationship* rel : searchElement->ownedRelationships()) {
            Subclassification* subclass = qobject_cast<Subclassification*>(rel);
            if (subclass && subclass->subtype() == type) {
                result.append(subclass);
            }
        }
        searchElement = searchElement->owner();
    }
    
    return result;
}

QList<Subsetting*> SysMLSerializer::findSubsettings(Feature* feature)
{
    QList<Subsetting*> result;
    if (!feature) {
        return result;
    }
    
    // 从特征的所有者及其父命名空间查找子集化关系
    Element* searchElement = feature->owner();
    while (searchElement) {
        for (Relationship* rel : searchElement->ownedRelationships()) {
            Subsetting* subsetting = qobject_cast<Subsetting*>(rel);
            if (subsetting && subsetting->subsettingFeature() == feature) {
                result.append(subsetting);
            }
        }
        searchElement = searchElement->owner();
    }
    
    return result;
}

QList<Redefinition*> SysMLSerializer::findRedefinitions(Feature* feature)
{
    QList<Redefinition*> result;
    if (!feature) {
        return result;
    }
    
    // 从特征的所有者及其父命名空间查找重定义关系
    Element* searchElement = feature->owner();
    while (searchElement) {
        for (Relationship* rel : searchElement->ownedRelationships()) {
            Redefinition* redefinition = qobject_cast<Redefinition*>(rel);
            if (redefinition && redefinition->redefiningFeature() == feature) {
                result.append(redefinition);
            }
        }
        searchElement = searchElement->owner();
    }
    
    return result;
}

void SysMLSerializer::serializeRequirementMember(Element* member, QTextStream& stream, int indent)
{
    if (!member) {
        return;
    }
    
    QString indentStr = getIndent(indent);
    QString fullName = member->declaredName();
    
    // 解析名称：格式为 "subject name", "actor name", "stakeholder name"
    QString keyword;
    QString name;
    
    if (fullName.startsWith("subject ")) {
        keyword = "subject";
        name = fullName.mid(8); // 跳过 "subject "
    } else if (fullName.startsWith("actor ")) {
        keyword = "actor";
        name = fullName.mid(6); // 跳过 "actor "
    } else if (fullName.startsWith("stakeholder ")) {
        keyword = "stakeholder";
        name = fullName.mid(12); // 跳过 "stakeholder "
    } else {
        // 如果不是 requirement member 格式，使用默认序列化
        serializeElement(member, stream, indent);
        return;
    }
    
    stream << indentStr << keyword << " " << name;
    
    // 如果是 PartUsage，获取类型
    PartUsage* partUsage = qobject_cast<PartUsage*>(member);
    if (partUsage) {
        Type* type = findPartUsageType(partUsage);
        if (type) {
            QString typeName = getQualifiedName(type);
            if (!typeName.isEmpty()) {
                stream << " : " << typeName;
            }
        } else {
            // 如果找不到类型，使用 PartUsage 中保存的类型名称
            QString typeName = partUsage->typeName();
            if (!typeName.isEmpty()) {
                stream << " : " << typeName;
            }
        }
        
        // 检查是否有值部分（default 值）
        // 注意：当前模型可能没有直接存储值部分，需要从子元素中查找
        // 这里先简单处理，如果有子元素，可能需要进一步处理
    }
    
    stream << ";\n";
}

void SysMLSerializer::serializeSubclassifications(Type* type, QTextStream& stream)
{
    if (!type) {
        return;
    }
    
    QList<Subclassification*> subclassifications = findSubclassifications(type);
    if (!subclassifications.isEmpty()) {
        QStringList supertypeNames;
        for (Subclassification* subclass : subclassifications) {
            Type* supertype = subclass->supertype();
            if (supertype) {
                QString supertypeName = getQualifiedName(supertype);
                if (!supertypeName.isEmpty()) {
                    supertypeNames.append(supertypeName);
                }
            }
        }
        if (!supertypeNames.isEmpty()) {
            stream << " :> " << supertypeNames.join(", ");
        }
    }
}

// ============================================
// 状态机相关元素的序列化方法
// ============================================

void SysMLSerializer::serializeExhibitStatement(ExhibitStatement* exhibitStmt, QTextStream& stream, int indent)
{
    if (!exhibitStmt) {
        return;
    }
    
    QString indentStr = getIndent(indent);
    QString name = exhibitStmt->declaredName();
    QString kind = exhibitStmt->stateMachineKind();
    
    stream << indentStr << "exhibit state";
    if (!name.isEmpty()) {
        stream << " " << name;
    }
    if (kind == "parallel") {
        stream << " parallel";
    }
    stream << " {\n";
    
    // 序列化子元素（按逻辑顺序：ref statements, states, actions, constraints, transitions）
    QList<Element*> refs;
    QList<Element*> states;
    QList<Element*> actions;
    QList<Element*> constraints;
    QList<Element*> transitions;
    QList<Element*> others;
    
    for (Element* child : exhibitStmt->ownedElements()) {
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
        serializeElement(child, stream, indent + 1);
        stream << "\n";
    }
    
    for (Element* child : states) {
        serializeElement(child, stream, indent + 1);
        stream << "\n";
    }
    
    for (Element* child : actions) {
        serializeElement(child, stream, indent + 1);
        stream << "\n";
    }
    
    for (Element* child : constraints) {
        serializeElement(child, stream, indent + 1);
        stream << "\n";
    }
    
    for (Element* child : transitions) {
        serializeElement(child, stream, indent + 1);
        stream << "\n";
    }
    
    for (Element* child : others) {
        serializeElement(child, stream, indent + 1);
        stream << "\n";
    }
    
    stream << indentStr << "}\n";
}

void SysMLSerializer::serializeTransition(Transition* transition, QTextStream& stream, int indent)
{
    if (!transition) {
        return;
    }
    
    QString indentStr = getIndent(indent);
    stream << indentStr << "transition";
    
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
            stream << indentStr << "    first " << sourceState << "\n";
        }
        
        // accept 子句（触发部分）
        TransitionTriggerPart* trigger = transition->triggerPart();
        if (trigger) {
            serializeTransitionTriggerPart(trigger, stream, indent + 1);
        }
        
        // if 子句（守卫部分）
        TransitionGuardPart* guard = transition->guardPart();
        if (guard) {
            serializeTransitionGuardPart(guard, stream, indent + 1);
        }
        
        // do 子句（效果部分）
        TransitionEffectPart* effect = transition->effectPart();
        if (effect) {
            serializeTransitionEffectPart(effect, stream, indent + 1);
        }
        
        // then 子句
        if (!targetState.isEmpty()) {
            stream << indentStr << "    then " << targetState << ";\n";
        }
    }
}

void SysMLSerializer::serializeStateActionMember(StateActionMember* actionMember, QTextStream& stream, int indent)
{
    if (!actionMember) {
        return;
    }
    
    QString indentStr = getIndent(indent);
    QString kind = actionMember->actionKind();  // "entry", "do", "exit"
    QString name = actionMember->actionName();
    bool hasActionKeyword = actionMember->hasActionKeyword();
    
    stream << indentStr << kind;
    
    if (hasActionKeyword) {
        stream << " action";
    }
    
    if (!name.isEmpty()) {
        stream << " " << name;
    }
    
    stream << ";\n";
}

void SysMLSerializer::serializeConstraintStatement(ConstraintStatement* constraintStmt, QTextStream& stream, int indent)
{
    if (!constraintStmt) {
        return;
    }
    
    QString indentStr = getIndent(indent);
    stream << indentStr << "constraint";
    
    QString expression = constraintStmt->expression();
    if (!expression.isEmpty()) {
        stream << " {" << expression << "}\n";
    } else {
        stream << ";\n";
    }
}

void SysMLSerializer::serializeTransitionTriggerPart(TransitionTriggerPart* triggerPart, QTextStream& stream, int indent)
{
    if (!triggerPart) {
        return;
    }
    
    AcceptClause* accept = triggerPart->acceptClause();
    if (accept) {
        serializeAcceptClause(accept, stream, indent);
    }
}

void SysMLSerializer::serializeTransitionGuardPart(TransitionGuardPart* guardPart, QTextStream& stream, int indent)
{
    if (!guardPart) {
        return;
    }
    
    QString indentStr = getIndent(indent);
    QString condition = guardPart->condition();
    if (!condition.isEmpty()) {
        // if 子句通常在 accept 之后，需要额外缩进一级
        stream << indentStr << "    if " << condition << "\n";
    }
}

void SysMLSerializer::serializeTransitionEffectPart(TransitionEffectPart* effectPart, QTextStream& stream, int indent)
{
    if (!effectPart) {
        return;
    }
    
    QString indentStr = getIndent(indent);
    QString actionType = effectPart->actionType();
    
    if (actionType == "send") {
        SendStatement* send = effectPart->sendStatement();
        if (send) {
            stream << indentStr << "do ";
            serializeSendStatement(send, stream, 0);  // sendStatement 自己处理格式
            stream << "\n";
        }
    } else if (actionType == "reference") {
        QString ref = effectPart->actionReference();
        if (!ref.isEmpty()) {
            stream << indentStr << "do " << ref << "\n";
        }
    } else if (actionType == "invocation") {
        QString invocation = effectPart->actionInvocation();
        if (!invocation.isEmpty()) {
            stream << indentStr << "do " << invocation << "\n";
        }
    }
}

void SysMLSerializer::serializeAcceptClause(AcceptClause* acceptClause, QTextStream& stream, int indent)
{
    if (!acceptClause) {
        return;
    }
    
    QString indentStr = getIndent(indent);
    stream << indentStr << "accept";
    
    QString acceptType = acceptClause->acceptType();
    
    if (acceptType == "item") {
        QString usageDecl = acceptClause->usageDeclaration();
        QString viaPort = acceptClause->viaPort();
        
        if (!usageDecl.isEmpty()) {
            stream << " " << usageDecl;
        }
        
        if (!viaPort.isEmpty()) {
            stream << " via " << viaPort;
        }
    } else if (acceptType == "when") {
        QString condition = acceptClause->condition();
        if (!condition.isEmpty()) {
            stream << " when " << condition;
        }
    } else if (acceptType == "at") {
        QString condition = acceptClause->condition();
        if (!condition.isEmpty()) {
            stream << " at " << condition;
        }
    } else if (acceptType == "event") {
        QString condition = acceptClause->condition();  // 事件名称
        if (!condition.isEmpty()) {
            stream << " " << condition;
        }
    }
    
    stream << "\n";
}

void SysMLSerializer::serializeSendStatement(SendStatement* sendStmt, QTextStream& stream, int indent)
{
    Q_UNUSED(indent)  // 这个方法由调用者处理缩进
    
    if (!sendStmt) {
        return;
    }
    
    stream << "send";
    
    bool hasNew = sendStmt->hasNew();
    QString actionInv = sendStmt->actionInvocation();
    QString target = sendStmt->target();
    
    if (hasNew && !actionInv.isEmpty()) {
        stream << " new " << actionInv;
    } else if (!actionInv.isEmpty()) {
        stream << " " << actionInv;
    }
    
    if (!target.isEmpty()) {
        stream << " to " << target;
    }
}

// ============================================
// 视图相关序列化方法
// ============================================

void SysMLSerializer::serializeViewpointDefinition(ViewpointDefinition* viewpointDef, QTextStream& stream, int indent)
{
    if (!viewpointDef) {
        return;
    }
    
    QString indentStr = getIndent(indent);
    QString name = viewpointDef->declaredName();
    if (name.isEmpty()) {
        name = "UnnamedViewpoint";
    }
    
    // 序列化定义前缀
    QStringList prefixes;
    if (viewpointDef->isAbstract()) {
        prefixes.append("abstract");
    }
    if (viewpointDef->isVariation()) {
        prefixes.append("variation");
    }
    if (viewpointDef->isIndividual()) {
        prefixes.append("individual");
    }
    
    if (!prefixes.isEmpty()) {
        stream << indentStr << prefixes.join(" ") << " ";
    } else {
        stream << indentStr;
    }
    
    stream << "viewpoint def " << name;
    serializeSubclassifications(viewpointDef, stream);
    
    bool hasChildren = !viewpointDef->ownedElements().isEmpty();
    if (hasChildren) {
        stream << "{\n";
        
        // 序列化子元素（元素之间不加空行）
        for (Element* child : viewpointDef->ownedElements()) {
            serializeElement(child, stream, indent + 1);
        }
        
        stream << indentStr << "}\n";
    } else {
        stream << ";\n";
    }
}

void SysMLSerializer::serializeViewDefinition(ViewDefinition* viewDef, QTextStream& stream, int indent)
{
    if (!viewDef) {
        return;
    }
    
    QString indentStr = getIndent(indent);
    QString name = viewDef->declaredName();
    if (name.isEmpty()) {
        name = "UnnamedView";
    }
    
    // 序列化定义前缀
    QStringList prefixes;
    if (viewDef->isAbstract()) {
        prefixes.append("abstract");
    }
    if (viewDef->isVariation()) {
        prefixes.append("variation");
    }
    if (viewDef->isIndividual()) {
        prefixes.append("individual");
    }
    
    if (!prefixes.isEmpty()) {
        stream << indentStr << prefixes.join(" ") << " ";
    } else {
        stream << indentStr;
    }
    
    stream << "view def " << name;
    serializeSubclassifications(viewDef, stream);
    
    bool hasChildren = !viewDef->ownedElements().isEmpty();
    if (hasChildren) {
        stream << "{\n";
        
        // 序列化子元素（元素之间不加空行）
        for (Element* child : viewDef->ownedElements()) {
            serializeElement(child, stream, indent + 1);
        }
        
        stream << indentStr << "}\n";
    } else {
        stream << ";\n";
    }
}

void SysMLSerializer::serializeViewUsage(ViewUsage* viewUsage, QTextStream& stream, int indent)
{
    if (!viewUsage) {
        return;
    }
    
    QString indentStr = getIndent(indent);
    QString name = viewUsage->declaredName();
    if (name.isEmpty()) {
        name = "UnnamedView";
    }
    
    stream << indentStr << "view " << name;
    
    // 序列化类型（如果有）
    QString typeName = viewUsage->typeName();
    if (!typeName.isEmpty()) {
        stream << ":" << typeName;
    } else if (viewUsage->viewDefinition()) {
        stream << ":" << viewUsage->viewDefinition()->declaredName();
    }
    
    // 检查是否有需要序列化的内容
    bool hasChildren = !viewUsage->ownedElements().isEmpty() || !viewUsage->exposedElements().isEmpty();
    bool hasRenderMethod = !viewUsage->renderMethod().isEmpty();
    bool hasFilterExpression = !viewUsage->filterExpression().isEmpty();
    bool hasSatisfiedViewpoint = viewUsage->satisfiedViewpoint() != nullptr;
    
    // 如果有任何内容需要序列化，输出大括号
    if (hasChildren || hasRenderMethod || hasFilterExpression || hasSatisfiedViewpoint) {
        stream << "{\n";
        
        QString childIndentStr = getIndent(indent + 1);
        
        // 序列化 satisfy 语句（如果有）
        // 从 ownedElements 中查找 SatisfyStatement
        bool hasSatisfyStmt = false;
        for (Element* child : viewUsage->ownedElements()) {
            if (child->elementType() == "SatisfyStatement") {
                serializeElement(child, stream, indent + 1);
                hasSatisfyStmt = true;
                break; // 通常只有一个 satisfy 语句
            }
        }
        
        // 如果没有找到 SatisfyStatement，但 satisfiedViewpoint 存在，则序列化它
        if (!hasSatisfyStmt && viewUsage->satisfiedViewpoint()) {
            stream << childIndentStr << "satisfy requirement ";
            QString identifier = viewUsage->satisfiedViewpointIdentifier();
            if (!identifier.isEmpty()) {
                // 形式：sv:SafetyViewpoint
                stream << identifier << ":" << viewUsage->satisfiedViewpoint()->declaredName();
            } else {
                // 形式：SafetyViewpoint（只有类型）
                stream << viewUsage->satisfiedViewpoint()->declaredName();
            }
            stream << ";\n";
        }
        
        // 序列化 expose 语句（从 ownedElements 中查找 ExposeStatement）
        for (Element* child : viewUsage->ownedElements()) {
            if (child->elementType() == "ExposeStatement") {
                serializeElement(child, stream, indent + 1);
            }
        }
        
        // 序列化 filter 语句（如果有）
        // 从 ownedElements 中查找 ElementFilterMember
        bool hasFilterMember = false;
        for (Element* child : viewUsage->ownedElements()) {
            if (child->elementType() == "ElementFilterMember") {
                serializeElement(child, stream, indent + 1);
                hasFilterMember = true;
            }
        }
        
        // 如果没有找到 ElementFilterMember，但 filterExpression 存在，则序列化它
        if (!hasFilterMember && !viewUsage->filterExpression().isEmpty()) {
            stream << childIndentStr << "filter " << viewUsage->filterExpression() << ";\n";
        }
        
        // 序列化 render 语句（如果有）
        // 从 ownedElements 中查找 ViewRenderingMember
        bool hasRenderMember = false;
        for (Element* child : viewUsage->ownedElements()) {
            if (child->elementType() == "ViewRenderingMember") {
                serializeElement(child, stream, indent + 1);
                hasRenderMember = true;
                break; // 通常只有一个 render 语句
            }
        }
        
        // 如果没有找到 ViewRenderingMember，但 renderMethod 存在，则序列化它
        if (!hasRenderMember && !viewUsage->renderMethod().isEmpty()) {
            stream << childIndentStr << "render " << viewUsage->renderMethod() << ";\n";
        }
        
        // 序列化其他子元素
        for (Element* child : viewUsage->ownedElements()) {
            QString childType = child->elementType();
            // 跳过已经序列化的元素类型
            if (childType != "ExposeStatement" && childType != "ElementFilterMember" && 
                childType != "SatisfyStatement" && childType != "ViewRenderingMember") {
                serializeElement(child, stream, indent + 1);
            }
        }
        
        stream << indentStr << "}\n";
    } else {
        stream << ";\n";
    }
}

void SysMLSerializer::serializeConcernDefinition(ConcernDefinition* concernDef, QTextStream& stream, int indent)
{
    if (!concernDef) {
        return;
    }
    
    QString indentStr = getIndent(indent);
    QString name = concernDef->declaredName();
    if (name.isEmpty()) {
        name = "UnnamedConcern";
    }
    
    // 序列化定义前缀
    QStringList prefixes;
    if (concernDef->isAbstract()) {
        prefixes.append("abstract");
    }
    if (concernDef->isVariation()) {
        prefixes.append("variation");
    }
    if (concernDef->isIndividual()) {
        prefixes.append("individual");
    }
    
    if (!prefixes.isEmpty()) {
        stream << indentStr << prefixes.join(" ") << " ";
    } else {
        stream << indentStr;
    }
    
    stream << "concern def " << name;
    serializeSubclassifications(concernDef, stream);
    
    bool hasChildren = !concernDef->ownedElements().isEmpty();
    if (hasChildren) {
        stream << " {\n";
        
        // 序列化子元素（元素之间不加空行）
        // 检查是否是 stakeholder/subject/actor（requirement member 格式）
        for (Element* child : concernDef->ownedElements()) {
            QString childName = child->declaredName();
            // 检查是否是 requirement member（格式为 "subject name", "actor name", "stakeholder name"）
            if (childName.startsWith("subject ") || childName.startsWith("actor ") || childName.startsWith("stakeholder ")) {
                serializeRequirementMember(child, stream, indent + 1);
            } else {
                serializeElement(child, stream, indent + 1);
            }
        }
        
        stream << indentStr << "}\n";
    } else {
        stream << ";\n";
    }
}

void SysMLSerializer::serializeExposeStatement(ExposeStatement* exposeStmt, QTextStream& stream, int indent)
{
    if (!exposeStmt) {
        return;
    }
    
    QString indentStr = getIndent(indent);
    QString qualifiedName = exposeStmt->exposedQualifiedName();
    
    stream << indentStr << "expose " << qualifiedName;
    
    // 如果 qualifiedName 已经包含 ::**，不需要再添加
    // 否则，根据类型和递归标记添加
    if (!qualifiedName.endsWith("::**")) {
        if (exposeStmt->isRecursive()) {
            stream << "::**";
        } else if (exposeStmt->exposeType() == ExposeStatement::NamespaceExpose && !qualifiedName.endsWith("::*")) {
            stream << "::*";
        }
    }
    
    stream << ";\n";
}

void SysMLSerializer::serializeElementFilterMember(ElementFilterMember* filterMember, QTextStream& stream, int indent)
{
    if (!filterMember) {
        return;
    }
    
    QString indentStr = getIndent(indent);
    QString filterExpr = filterMember->filterExpression();
    
    stream << indentStr << "filter " << filterExpr << ";\n";
}

void SysMLSerializer::serializeViewRenderingMember(ViewRenderingMember* renderMember, QTextStream& stream, int indent)
{
    if (!renderMember) {
        return;
    }
    
    QString indentStr = getIndent(indent);
    stream << indentStr << "render ";
    
    // 检查父元素是否为 ViewUsage，如果是，使用 ViewUsage 的 renderMethod
    Element* parent = renderMember->owner();
    if (ViewUsage* viewUsage = qobject_cast<ViewUsage*>(parent)) {
        QString renderMethod = viewUsage->renderMethod();
        if (!renderMethod.isEmpty()) {
            stream << renderMethod << ";\n";
            return;
        }
    }
    
    if (renderMember->renderingUsage()) {
        stream << getQualifiedName(renderMember->renderingUsage());
    } else {
        stream << "asTable"; // 默认渲染方式
    }
    
    stream << ";\n";
}

void SysMLSerializer::serializeFrameConcernMember(FrameConcernMember* frameMember, QTextStream& stream, int indent)
{
    if (!frameMember) {
        return;
    }
    
    QString indentStr = getIndent(indent);
    
    stream << indentStr << "frame concern ";
    
    // 序列化标识符和类型（如果有）
    QString identifier = frameMember->identifier();
    Element* concern = frameMember->concern();
    
    if (!identifier.isEmpty() && concern) {
        // 形式：vs:VehicleSafety
        stream << identifier << ":" << getQualifiedName(concern);
    } else if (concern) {
        // 形式：VehicleSafety（只有类型）
        stream << getQualifiedName(concern);
    }
    
    stream << ";\n";
}

void SysMLSerializer::serializeAnnotation(Annotation* annotation, QTextStream& stream, int indent)
{
    if (!annotation) {
        return;
    }
    
    QString indentStr = getIndent(indent);
    QString annotationName = annotation->annotationName();
    QVariantMap assignments = annotation->metadataAssignments();
    
    // 序列化注解：@GraphicMetadata { ... } （使用 '@' 语法）
    stream << indentStr << "@" << annotationName << " {\n";
    
    // 序列化元数据赋值（使用 SysML 语法：key = value;）
    QStringList keys = assignments.keys();
    for (int i = 0; i < keys.size(); ++i) {
        QString key = keys[i];
        QVariant value = assignments[key];
        
        stream << indentStr << "    " << key << " = ";
        
        // 根据值的类型格式化输出
        if (value.typeId() == QMetaType::QString) {
            // 字符串需要用引号包围
            stream << "\"" << escapeString(value.toString()) << "\"";
        } else if (value.typeId() == QMetaType::Double || value.typeId() == QMetaType::Int || 
                   value.typeId() == QMetaType::LongLong) {
            // 数字直接输出
            stream << value.toString();
        } else if (value.typeId() == QMetaType::Bool) {
            // 布尔值：true 或 false
            stream << (value.toBool() ? "true" : "false");
        } else {
            // 其他类型，尝试转换为字符串（加引号）
            stream << "\"" << escapeString(value.toString()) << "\"";
        }
        
        // 每个赋值语句以分号结尾
        stream << ";\n";
    }
    
    stream << indentStr << "}\n";
}

SYSML_NAMESPACE_END

