#ifndef MODELBUILDERVISITOR_H
#define MODELBUILDERVISITOR_H

// ANTLR4 Runtime 头文件
#ifndef ANTLR4CPP_STATIC
#define ANTLR4CPP_STATIC
#endif
#include "Token.h"
#include <antlr4-runtime.h>

// 生成的访问者头文件（直接包含，因为 generated 目录已在包含路径中）
#include "SysMLBaseVisitor.h"
#include "SysMLParser.h"

#include "../../core/model/Element.h"
#include "../../core/common/Global.h"
#include <QStack>
#include <QStringList>

SYSML_NAMESPACE_BEGIN

class Resource;
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
class NamespaceImport;
class Type;
class Namespace;
class FeatureTyping;
class Subclassification;
class Subsetting;
class Redefinition;
class Conjugation;
class Dependency;
class ActionDefinition;
class CalculationDefinition;
class AnalysisCase;
class RenderingUsage;
class ReturnParameter;
class AnalysisSubject;
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
class EndStatement;
class BindStatement;
class ConnectStatement;
class AllocateStatement;
class AssertStatement;
class RequireStatement;

/**
 * @brief ModelBuilderVisitor 模型构建访问者
 * 
 * 功能:
 * - 遍历 ANTLR4 AST
 * - 构建模型对象
 * - 建立元素关系
 */
class ModelBuilderVisitor : public SysML::SysMLBaseVisitor {
public:
    explicit ModelBuilderVisitor(Resource* resource);
    ~ModelBuilderVisitor();
    
    // 访问规则
    std::any visitFile(SysML::SysMLParser::FileContext* ctx) override;
    std::any visitPackage(SysML::SysMLParser::PackageContext* ctx) override;
    std::any visitPackageBody(SysML::SysMLParser::PackageBodyContext* ctx) override;
    
    // 定义类型访问
    std::any visitDefinition(SysML::SysMLParser::DefinitionContext* ctx) override;
    std::any visitPartDefinition(SysML::SysMLParser::PartDefinitionContext* ctx) override;
    std::any visitActionDefinition(SysML::SysMLParser::ActionDefinitionContext* ctx) override;
    std::any visitCalculationDefinition(SysML::SysMLParser::CalculationDefinitionContext* ctx) override;
    std::any visitStateDefinition(SysML::SysMLParser::StateDefinitionContext* ctx) override;
    std::any visitRequirementDefinition(SysML::SysMLParser::RequirementDefinitionContext* ctx) override;
    std::any visitConstraintDefinition(SysML::SysMLParser::ConstraintDefinitionContext* ctx) override;
    std::any visitPortDefinition(SysML::SysMLParser::PortDefinitionContext* ctx) override;
    std::any visitConnectorDefinition(SysML::SysMLParser::ConnectorDefinitionContext* ctx) override;
    std::any visitInterfaceDefinition(SysML::SysMLParser::InterfaceDefinitionContext* ctx) override;
    std::any visitDatatypeDefinition(SysML::SysMLParser::DatatypeDefinitionContext* ctx) override;
    std::any visitClassDefinition(SysML::SysMLParser::ClassDefinitionContext* ctx) override;
    std::any visitEnumDefinition(SysML::SysMLParser::EnumDefinitionContext* ctx) override;
    std::any visitAttributeDefinition(SysML::SysMLParser::AttributeDefinitionContext* ctx) override;
    std::any visitConnectionDefinition(SysML::SysMLParser::ConnectionDefinitionContext* ctx) override;
    std::any visitAllocationDefinition(SysML::SysMLParser::AllocationDefinitionContext* ctx) override;
    std::any visitItemDefinition(SysML::SysMLParser::ItemDefinitionContext* ctx) override;
    std::any visitFlowDefinition(SysML::SysMLParser::FlowDefinitionContext* ctx) override;
    std::any visitViewpointDefinition(SysML::SysMLParser::ViewpointDefinitionContext* ctx) override;
    std::any visitViewDefinition(SysML::SysMLParser::ViewDefinitionContext* ctx) override;
    std::any visitConcernDefinition(SysML::SysMLParser::ConcernDefinitionContext* ctx) override;
    
    // 使用类型访问
    std::any visitUsage(SysML::SysMLParser::UsageContext* ctx) override;
    std::any visitPartUsage(SysML::SysMLParser::PartUsageContext* ctx) override;
    std::any visitActionUsage(SysML::SysMLParser::ActionUsageContext* ctx) override;
    std::any visitAnalysisUsage(SysML::SysMLParser::AnalysisUsageContext* ctx) override;
    std::any visitRenderingUsage(SysML::SysMLParser::RenderingUsageContext* ctx) override;
    std::any visitStateUsage(SysML::SysMLParser::StateUsageContext* ctx) override;
    std::any visitRequirementUsage(SysML::SysMLParser::RequirementUsageContext* ctx) override;
    std::any visitConstraintUsage(SysML::SysMLParser::ConstraintUsageContext* ctx) override;
    std::any visitPortUsage(SysML::SysMLParser::PortUsageContext* ctx) override;
    std::any visitConnectorUsage(SysML::SysMLParser::ConnectorUsageContext* ctx) override;
    std::any visitFlowUsage(SysML::SysMLParser::FlowUsageContext* ctx) override;
    std::any visitConnectionUsage(SysML::SysMLParser::ConnectionUsageContext* ctx) override;
    std::any visitAllocationUsage(SysML::SysMLParser::AllocationUsageContext* ctx) override;
    std::any visitItemUsage(SysML::SysMLParser::ItemUsageContext* ctx) override;
    std::any visitViewUsage(SysML::SysMLParser::ViewUsageContext* ctx) override;
    
    // 语句访问
    std::any visitStatement(SysML::SysMLParser::StatementContext* ctx) override;
    std::any visitBindStatement(SysML::SysMLParser::BindStatementContext* ctx) override;
    std::any visitConnectStatement(SysML::SysMLParser::ConnectStatementContext* ctx) override;
    std::any visitAllocateStatement(SysML::SysMLParser::AllocateStatementContext* ctx) override;
    std::any visitEndStatement(SysML::SysMLParser::EndStatementContext* ctx) override;
    std::any visitFlowStatement(SysML::SysMLParser::FlowStatementContext* ctx) override;
    std::any visitPerformStatement(SysML::SysMLParser::PerformStatementContext* ctx) override;
    std::any visitAssertStatement(SysML::SysMLParser::AssertStatementContext* ctx) override;
    std::any visitRequireStatement(SysML::SysMLParser::RequireStatementContext* ctx) override;
    std::any visitAssumeStatement(SysML::SysMLParser::AssumeStatementContext* ctx) override;
    std::any visitRedefinesStatement(SysML::SysMLParser::RedefinesStatementContext* ctx) override;
    std::any visitActionInputOutput(SysML::SysMLParser::ActionInputOutputContext* ctx) override;
    std::any visitCalculationBody(SysML::SysMLParser::CalculationBodyContext* ctx) override;
    std::any visitReturnParameter(SysML::SysMLParser::ReturnParameterContext* ctx) override;
    std::any visitAnalysisBody(SysML::SysMLParser::AnalysisBodyContext* ctx) override;
    std::any visitSubjectAssignment(SysML::SysMLParser::SubjectAssignmentContext* ctx) override;
    
    // 体访问
    std::any visitDefinitionBody(SysML::SysMLParser::DefinitionBodyContext* ctx) override;
    std::any visitUsageBody(SysML::SysMLParser::UsageBodyContext* ctx) override;
    std::any visitPartBody(SysML::SysMLParser::PartBodyContext* ctx) override;
    std::any visitActionBody(SysML::SysMLParser::ActionBodyContext* ctx) override;
    std::any visitStateBody(SysML::SysMLParser::StateBodyContext* ctx) override;
    std::any visitStateDefinitionBody(SysML::SysMLParser::StateDefinitionBodyContext* ctx) override;
    std::any visitStateBodyItem(SysML::SysMLParser::StateBodyItemContext* ctx) override;
    std::any visitExhibitStatement(SysML::SysMLParser::ExhibitStatementContext* ctx) override;
    std::any visitStateMember(SysML::SysMLParser::StateMemberContext* ctx) override;
    std::any visitTransitionMember(SysML::SysMLParser::TransitionMemberContext* ctx) override;
    std::any visitRefStatement(SysML::SysMLParser::RefStatementContext* ctx) override;
    std::any visitConstraintStatement(SysML::SysMLParser::ConstraintStatementContext* ctx) override;
    std::any visitRequirementBody(SysML::SysMLParser::RequirementBodyContext* ctx) override;
    std::any visitRequirementMember(SysML::SysMLParser::RequirementMemberContext* ctx) override;
    std::any visitRequirementConstraintMember(SysML::SysMLParser::RequirementConstraintMemberContext* ctx) override;
    std::any visitViewpointBody(SysML::SysMLParser::ViewpointBodyContext* ctx) override;
    std::any visitViewDefinitionBody(SysML::SysMLParser::ViewDefinitionBodyContext* ctx) override;
    std::any visitViewBody(SysML::SysMLParser::ViewBodyContext* ctx) override;
    std::any visitConcernBody(SysML::SysMLParser::ConcernBodyContext* ctx) override;
    std::any visitExposeStatement(SysML::SysMLParser::ExposeStatementContext* ctx) override;
    std::any visitElementFilterMember(SysML::SysMLParser::ElementFilterMemberContext* ctx) override;
    std::any visitViewRenderingMember(SysML::SysMLParser::ViewRenderingMemberContext* ctx) override;
    std::any visitFrameConcernMember(SysML::SysMLParser::FrameConcernMemberContext* ctx) override;
    std::any visitSatisfyStatement(SysML::SysMLParser::SatisfyStatementContext* ctx) override;
    std::any visitPortBody(SysML::SysMLParser::PortBodyContext* ctx) override;
    std::any visitConnectorBody(SysML::SysMLParser::ConnectorBodyContext* ctx) override;
    std::any visitFlowBody(SysML::SysMLParser::FlowBodyContext* ctx) override;
    std::any visitInterfaceBody(SysML::SysMLParser::InterfaceBodyContext* ctx) override;
    std::any visitDatatypeBody(SysML::SysMLParser::DatatypeBodyContext* ctx) override;
    std::any visitClassBody(SysML::SysMLParser::ClassBodyContext* ctx) override;
    std::any visitEnumBody(SysML::SysMLParser::EnumBodyContext* ctx) override;
    std::any visitAttributeBody(SysML::SysMLParser::AttributeBodyContext* ctx) override;
    std::any visitDocStatement(SysML::SysMLParser::DocStatementContext* ctx) override;
    std::any visitCommentStatement(SysML::SysMLParser::CommentStatementContext* ctx) override;
    std::any visitMetadataAnnotation(SysML::SysMLParser::MetadataAnnotationContext* ctx) override;
    
    // 其他访问
    std::any visitAttribute(SysML::SysMLParser::AttributeContext* ctx) override;
    std::any visitSimplifiedAttribute(SysML::SysMLParser::SimplifiedAttributeContext* ctx) override;
    std::any visitAttributeContent(SysML::SysMLParser::AttributeContentContext* ctx) override;
    std::any visitAttributeExpressions(SysML::SysMLParser::AttributeExpressionsContext* ctx) override;
    std::any visitFeaturePrefix(SysML::SysMLParser::FeaturePrefixContext* ctx) override;
    std::any visitSubsettingExpression(SysML::SysMLParser::SubsettingExpressionContext* ctx) override;
    std::any visitRedefinitionExpression(SysML::SysMLParser::RedefinitionExpressionContext* ctx) override;
    std::any visitTypingExpression(SysML::SysMLParser::TypingExpressionContext* ctx) override;
    std::any visitSubclassificationExpression(SysML::SysMLParser::SubclassificationExpressionContext* ctx) override;
    std::any visitImportStatement(SysML::SysMLParser::ImportStatementContext* ctx) override;
    std::any visitQualifiedName(SysML::SysMLParser::QualifiedNameContext* ctx) override;
    std::any visitTransition(SysML::SysMLParser::TransitionContext* ctx) override;
    std::any visitTransitionBody(SysML::SysMLParser::TransitionBodyContext* ctx) override;
    std::any visitSimpleTransitionBody(SysML::SysMLParser::SimpleTransitionBodyContext* ctx) override;
    std::any visitComplexTransitionBody(SysML::SysMLParser::ComplexTransitionBodyContext* ctx) override;
    std::any visitStateReference(SysML::SysMLParser::StateReferenceContext* ctx) override;
    std::any visitTransitionTriggerPart(SysML::SysMLParser::TransitionTriggerPartContext* ctx) override;
    std::any visitAcceptClause(SysML::SysMLParser::AcceptClauseContext* ctx) override;
    std::any visitAcceptItem(SysML::SysMLParser::AcceptItemContext* ctx) override;
    std::any visitAcceptExpression(SysML::SysMLParser::AcceptExpressionContext* ctx) override;
    std::any visitTransitionGuardPart(SysML::SysMLParser::TransitionGuardPartContext* ctx) override;
    std::any visitTransitionEffectPart(SysML::SysMLParser::TransitionEffectPartContext* ctx) override;
    std::any visitTransitionAction(SysML::SysMLParser::TransitionActionContext* ctx) override;
    std::any visitSendStatement(SysML::SysMLParser::SendStatementContext* ctx) override;
    std::any visitStateActionMember(SysML::SysMLParser::StateActionMemberContext* ctx) override;
    std::any visitEntryActionMember(SysML::SysMLParser::EntryActionMemberContext* ctx) override;
    std::any visitDoActionMember(SysML::SysMLParser::DoActionMemberContext* ctx) override;
    std::any visitExitActionMember(SysML::SysMLParser::ExitActionMemberContext* ctx) override;
    std::any visitStateActionUsage(SysML::SysMLParser::StateActionUsageContext* ctx) override;
    std::any visitEnumValue(SysML::SysMLParser::EnumValueContext* ctx) override;
    
    // 表达式访问
    std::any visitExpression(SysML::SysMLParser::ExpressionContext* ctx) override;
    std::any visitPrimaryExpression(SysML::SysMLParser::PrimaryExpressionContext* ctx) override;
    std::any visitLogicalOrExpression(SysML::SysMLParser::LogicalOrExpressionContext* ctx) override;
    std::any visitLogicalAndExpression(SysML::SysMLParser::LogicalAndExpressionContext* ctx) override;
    std::any visitComparisonExpression(SysML::SysMLParser::ComparisonExpressionContext* ctx) override;
    std::any visitAdditiveExpression(SysML::SysMLParser::AdditiveExpressionContext* ctx) override;
    std::any visitMultiplicativeExpression(SysML::SysMLParser::MultiplicativeExpressionContext* ctx) override;
    std::any visitUnaryExpression(SysML::SysMLParser::UnaryExpressionContext* ctx) override;
    std::any visitUnitExpression(SysML::SysMLParser::UnitExpressionContext* ctx) override;
    std::any visitUnitTerm(SysML::SysMLParser::UnitTermContext* ctx) override;
    std::any visitLiteralExpression(SysML::SysMLParser::LiteralExpressionContext* ctx) override;
    std::any visitFeatureReferenceExpression(SysML::SysMLParser::FeatureReferenceExpressionContext* ctx) override;
    
    // 关系类型访问
    std::any visitRelationship(SysML::SysMLParser::RelationshipContext* ctx) override;
    std::any visitSubclassification(SysML::SysMLParser::SubclassificationContext* ctx) override;
    std::any visitSubsetting(SysML::SysMLParser::SubsettingContext* ctx) override;
    std::any visitRedefinition(SysML::SysMLParser::RedefinitionContext* ctx) override;
    std::any visitConjugation(SysML::SysMLParser::ConjugationContext* ctx) override;
    std::any visitDependency(SysML::SysMLParser::DependencyContext* ctx) override;
    
    // 错误处理
    void reportError(const QString& message, antlr4::ParserRuleContext* ctx = nullptr);
    void reportWarning(const QString& message, antlr4::ParserRuleContext* ctx = nullptr);
    bool hasErrors() const { return !m_errors.isEmpty(); }
    bool hasWarnings() const { return !m_warnings.isEmpty(); }
    QStringList errors() const { return m_errors; }
    QStringList warnings() const { return m_warnings; }
    int errorCount() const { return m_errors.size(); }
    int warningCount() const { return m_warnings.size(); }
    
private:
    // 辅助方法
    QString getText(antlr4::tree::TerminalNode* node) const;
    QStringList getQualifiedNameParts(SysML::SysMLParser::QualifiedNameContext* ctx);
    QString getConnectorEndText(SysML::SysMLParser::ConnectorEndMemberContext* ctx);
    Element* getCurrentParent();
    void pushParent(Element* element);
    void popParent();
    
    // 特征前缀处理（重载方法，接受 Feature 参数）
    void visitFeaturePrefix(SysML::SysMLParser::FeaturePrefixContext* ctx, Feature* feature);
    
    // 查找方法
    Type* findType(const QStringList& nameParts);
    Namespace* findNamespace(const QStringList& nameParts);
    Element* findElementByQualifiedName(const QStringList& nameParts);
    Element* findElementByQualifiedNameFromScope(const QStringList& nameParts, Element* startScope);
    
    // 关系创建方法
    void createFeatureTyping(Feature* feature, const QStringList& typeNameParts);
    void createPartUsageTyping(PartUsage* partUsage, const QStringList& typeNameParts);
    void createUsageTyping(Type* usage, const QStringList& typeNameParts);
    void createSubclassification(Type* subtype, const QStringList& supertypeNameParts);
    void createSubsetting(Feature* subsettingFeature, const QStringList& subsettedFeatureNameParts);
    void createRedefinition(Feature* redefiningFeature, const QStringList& redefinedFeatureNameParts);
    void createConjugation(Type* conjugatedType, const QStringList& originalTypeNameParts);
    void createDependency(Element* clientElement, const QStringList& supplierElementNameParts);
    void setImportedNamespace(NamespaceImport* import, const QStringList& namespaceNameParts);
    
    // 辅助方法（错误格式化）
    QString formatError(const QString& message, antlr4::ParserRuleContext* ctx, const QString& severity);
    
    // 处理 usageCompletion 中的 valuePart
    void processUsageCompletionValuePart(SysML::SysMLParser::UsageCompletionContext* completionCtx);
    
    // 注解解析辅助方法
    QString getMetadataAssignmentKey(SysML::SysMLParser::MetadataAssignmentContext* ctx) const;
    QVariant getMetadataAssignmentValue(SysML::SysMLParser::MetadataAssignmentContext* ctx) const;
    
    Resource* m_resource;
    QStack<Element*> m_parentStack;
    QStringList m_errors;
    QStringList m_warnings;
    
    // 语义检查辅助
    bool checkDuplicateName(Element* element, const QString& name, antlr4::ParserRuleContext* ctx);
    void validateTypeReference(const QStringList& typeNameParts, antlr4::ParserRuleContext* ctx);
};

SYSML_NAMESPACE_END

#endif // MODELBUILDERVISITOR_H
