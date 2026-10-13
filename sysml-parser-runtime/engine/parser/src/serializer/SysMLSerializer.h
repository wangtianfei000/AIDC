#ifndef SYSMLSERIALIZER_H
#define SYSMLSERIALIZER_H

#include <QString>
#include <QTextStream>
#include "../../../core/model/Element.h"
#include "../../../core/infrastructure/Resource.h"
#include "../../../core/common/Global.h"

SYSML_NAMESPACE_BEGIN

class Package;
class PartDefinition;
class PartUsage;
class ActionUsage;
class StateUsage;
class RequirementUsage;
class ConstraintUsage;
class PortUsage;
class ConnectorUsage;
class FlowUsage;
class Feature;
class Import;
class NamespaceImport;
class Type;
class Namespace;
class FeatureTyping;
class ActionDefinition;
class StateDefinition;
class RequirementDefinition;
class ConstraintDefinition;
class PortDefinition;
class ConnectorDefinition;
class InterfaceDefinition;
class DataTypeDefinition;
class ClassDefinition;
class EnumDefinition;
class AttributeDefinition;
class ConnectionDefinition;
class ConnectionUsage;
class AllocationDefinition;
class AllocationUsage;
class ItemDefinition;
class ItemUsage;
class ViewpointDefinition;
class ViewDefinition;
class ViewUsage;
class ConcernDefinition;
class ExposeStatement;
class ElementFilterMember;
class ViewRenderingMember;
class FrameConcernMember;
class EndStatement;
class BindStatement;
class ConnectStatement;
class AllocateStatement;
class FlowDefinition;
class FlowStatement;
class PerformStatement;
class AssertStatement;
class RequireStatement;
class AssumeStatement;
class DocStatement;
class ActionInputOutput;
class Subclassification;
class Subsetting;
class Redefinition;
class Conjugation;
class Dependency;
// 状态机相关类
class ExhibitStatement;
class Transition;
class StateActionMember;
class ConstraintStatement;
class TransitionTriggerPart;
class TransitionGuardPart;
class TransitionEffectPart;
class AcceptClause;
class SendStatement;
class Annotation;

/**
 * @brief SysMLSerializer SysML 文本序列化器
 * 
 * 功能:
 * - 将模型对象序列化为 SysML 文本
 * - 保持格式化和可读性
 * - 支持缩进和换行
 */
class SysMLSerializer {
public:
    explicit SysMLSerializer();
    ~SysMLSerializer();
    
    // 序列化
    QString serializeResource(Resource* resource);
    QString serializeElement(Element* element);
    
    // 保存到文件
    bool saveToFile(Resource* resource, const QString& filePath);
    
    // 格式化选项
    void setIndentSize(int size) { m_indentSize = size; }
    int indentSize() const { return m_indentSize; }
    
private:
    // 序列化方法
    void serializeElement(Element* element, QTextStream& stream, int indent = 0);
    void serializePackage(Package* package, QTextStream& stream, int indent = 0);
    void serializePartDefinition(PartDefinition* partDef, QTextStream& stream, int indent = 0);
    void serializePartUsage(PartUsage* partUsage, QTextStream& stream, int indent = 0);
    void serializeActionUsage(ActionUsage* actionUsage, QTextStream& stream, int indent = 0);
    void serializeStateUsage(StateUsage* stateUsage, QTextStream& stream, int indent = 0);
    void serializeRequirementUsage(RequirementUsage* requirementUsage, QTextStream& stream, int indent = 0);
    void serializeConstraintUsage(ConstraintUsage* constraintUsage, QTextStream& stream, int indent = 0);
    void serializePortUsage(PortUsage* portUsage, QTextStream& stream, int indent = 0);
    void serializeConnectorUsage(ConnectorUsage* connectorUsage, QTextStream& stream, int indent = 0);
    void serializeFlowUsage(FlowUsage* flowUsage, QTextStream& stream, int indent = 0);
    void serializeFeature(Feature* feature, QTextStream& stream, int indent = 0);
    void serializeImport(Import* import, QTextStream& stream, int indent = 0);
    
    // 新定义类型的序列化方法
    void serializeActionDefinition(ActionDefinition* actionDef, QTextStream& stream, int indent = 0);
    void serializeStateDefinition(StateDefinition* stateDef, QTextStream& stream, int indent = 0);
    void serializeRequirementDefinition(RequirementDefinition* reqDef, QTextStream& stream, int indent = 0);
    void serializeConstraintDefinition(ConstraintDefinition* constraintDef, QTextStream& stream, int indent = 0);
    void serializePortDefinition(PortDefinition* portDef, QTextStream& stream, int indent = 0);
    void serializeConnectorDefinition(ConnectorDefinition* connDef, QTextStream& stream, int indent = 0);
    void serializeInterfaceDefinition(InterfaceDefinition* ifaceDef, QTextStream& stream, int indent = 0);
    void serializeDataTypeDefinition(DataTypeDefinition* dtDef, QTextStream& stream, int indent = 0);
    void serializeClassDefinition(ClassDefinition* classDef, QTextStream& stream, int indent = 0);
    void serializeEnumDefinition(EnumDefinition* enumDef, QTextStream& stream, int indent = 0);
    void serializeAttributeDefinition(AttributeDefinition* attrDef, QTextStream& stream, int indent = 0);
    void serializeConnectionDefinition(ConnectionDefinition* connDef, QTextStream& stream, int indent = 0);
    void serializeConnectionUsage(ConnectionUsage* connUsage, QTextStream& stream, int indent = 0);
    void serializeAllocationDefinition(AllocationDefinition* allocDef, QTextStream& stream, int indent = 0);
    void serializeAllocationUsage(AllocationUsage* allocUsage, QTextStream& stream, int indent = 0);
    void serializeItemDefinition(ItemDefinition* itemDef, QTextStream& stream, int indent = 0);
    void serializeItemUsage(ItemUsage* itemUsage, QTextStream& stream, int indent = 0);
    void serializeViewpointDefinition(ViewpointDefinition* viewpointDef, QTextStream& stream, int indent = 0);
    void serializeViewDefinition(ViewDefinition* viewDef, QTextStream& stream, int indent = 0);
    void serializeViewUsage(ViewUsage* viewUsage, QTextStream& stream, int indent = 0);
    void serializeConcernDefinition(ConcernDefinition* concernDef, QTextStream& stream, int indent = 0);
    void serializeExposeStatement(ExposeStatement* exposeStmt, QTextStream& stream, int indent = 0);
    void serializeElementFilterMember(ElementFilterMember* filterMember, QTextStream& stream, int indent = 0);
    void serializeViewRenderingMember(ViewRenderingMember* renderMember, QTextStream& stream, int indent = 0);
    void serializeFrameConcernMember(FrameConcernMember* frameMember, QTextStream& stream, int indent = 0);
    void serializeEndStatement(EndStatement* endStmt, QTextStream& stream, int indent = 0);
    void serializeBindStatement(BindStatement* bindStmt, QTextStream& stream, int indent = 0);
    void serializeConnectStatement(ConnectStatement* connectStmt, QTextStream& stream, int indent = 0);
    void serializeAllocateStatement(AllocateStatement* allocateStmt, QTextStream& stream, int indent = 0);
    void serializeFlowDefinition(FlowDefinition* flowDef, QTextStream& stream, int indent = 0);
    void serializeFlowStatement(FlowStatement* flowStmt, QTextStream& stream, int indent = 0);
    void serializePerformStatement(PerformStatement* performStmt, QTextStream& stream, int indent = 0);
    void serializeAssertStatement(AssertStatement* assertStmt, QTextStream& stream, int indent = 0);
    void serializeRequireStatement(RequireStatement* requireStmt, QTextStream& stream, int indent = 0);
    void serializeAssumeStatement(AssumeStatement* assumeStmt, QTextStream& stream, int indent = 0);
    void serializeDocStatement(DocStatement* docStmt, QTextStream& stream, int indent = 0);
    void serializeActionInputOutput(ActionInputOutput* actionIO, QTextStream& stream, int indent = 0);
    void serializeAnnotation(Annotation* annotation, QTextStream& stream, int indent = 0);
    
    // 状态机相关元素的序列化方法
    void serializeExhibitStatement(class ExhibitStatement* exhibitStmt, QTextStream& stream, int indent = 0);
    void serializeTransition(class Transition* transition, QTextStream& stream, int indent = 0);
    void serializeStateActionMember(class StateActionMember* actionMember, QTextStream& stream, int indent = 0);
    void serializeConstraintStatement(class ConstraintStatement* constraintStmt, QTextStream& stream, int indent = 0);
    void serializeTransitionTriggerPart(class TransitionTriggerPart* triggerPart, QTextStream& stream, int indent = 0);
    void serializeTransitionGuardPart(class TransitionGuardPart* guardPart, QTextStream& stream, int indent = 0);
    void serializeTransitionEffectPart(class TransitionEffectPart* effectPart, QTextStream& stream, int indent = 0);
    void serializeAcceptClause(class AcceptClause* acceptClause, QTextStream& stream, int indent = 0);
    void serializeSendStatement(class SendStatement* sendStmt, QTextStream& stream, int indent = 0);
    
    // 辅助方法
    QString getIndent(int level);
    QString escapeString(const QString& str);
    void serializeSubclassifications(Type* type, QTextStream& stream);
    void serializeRequirementMember(Element* member, QTextStream& stream, int indent = 0);
    QString formatExpression(const QString& expression);  // 格式化表达式，在运算符前后添加空格
    
    // 查找和获取方法
    Type* findFeatureType(Feature* feature);
    Type* findPartUsageType(PartUsage* partUsage);
    Type* findUsageType(Type* usage);
    QString getQualifiedName(Element* element);
    
    // 关系查找方法
    QList<Subclassification*> findSubclassifications(Type* type);
    QList<Subsetting*> findSubsettings(Feature* feature);
    QList<Redefinition*> findRedefinitions(Feature* feature);
    
    int m_indentSize = 4;
};

SYSML_NAMESPACE_END

#endif // SYSMLSERIALIZER_H

