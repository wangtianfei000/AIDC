
// Generated from D:/Document/7.AWorks/BlockSim/2.Development/SysML/parser/grammar/SysML.g4 by ANTLR 4.13.2

#pragma once


#include "antlr4-runtime.h"
#include "SysMLVisitor.h"


namespace SysML {

/**
 * This class provides an empty implementation of SysMLVisitor, which can be
 * extended to create a visitor which only needs to handle a subset of the available methods.
 */
class  SysMLBaseVisitor : public SysMLVisitor {
public:

  virtual std::any visitFile(SysMLParser::FileContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPackage(SysMLParser::PackageContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPackageBody(SysMLParser::PackageBodyContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitBasicDefinitionPrefix(SysMLParser::BasicDefinitionPrefixContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDefinitionExtensionKeyword(SysMLParser::DefinitionExtensionKeywordContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDefinitionPrefix(SysMLParser::DefinitionPrefixContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitOccurrenceDefinitionPrefix(SysMLParser::OccurrenceDefinitionPrefixContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitEmptyMultiplicityMember(SysMLParser::EmptyMultiplicityMemberContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFeatureDirection(SysMLParser::FeatureDirectionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitRefPrefix(SysMLParser::RefPrefixContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitBasicUsagePrefix(SysMLParser::BasicUsagePrefixContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitEndUsagePrefix(SysMLParser::EndUsagePrefixContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitOwnedCrossFeatureMember(SysMLParser::OwnedCrossFeatureMemberContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUsageExtensionKeyword(SysMLParser::UsageExtensionKeywordContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUnextendedUsagePrefix(SysMLParser::UnextendedUsagePrefixContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUsagePrefix(SysMLParser::UsagePrefixContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitOccurrenceUsagePrefix(SysMLParser::OccurrenceUsagePrefixContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPortionKind(SysMLParser::PortionKindContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitIdentification(SysMLParser::IdentificationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitName(SysMLParser::NameContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDefinitionDeclaration(SysMLParser::DefinitionDeclarationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSubclassificationPart(SysMLParser::SubclassificationPartContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitOwnedSubclassification(SysMLParser::OwnedSubclassificationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDefinitionBody(SysMLParser::DefinitionBodyContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDefinitionBodyItem(SysMLParser::DefinitionBodyItemContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitMetadataAnnotation(SysMLParser::MetadataAnnotationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitMetadataAssignment(SysMLParser::MetadataAssignmentContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSimplifiedAttribute(SysMLParser::SimplifiedAttributeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDefinitionMember(SysMLParser::DefinitionMemberContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUsageMember(SysMLParser::UsageMemberContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitVisibilityIndicator(SysMLParser::VisibilityIndicatorContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUsageDeclaration(SysMLParser::UsageDeclarationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFeatureDeclaration(SysMLParser::FeatureDeclarationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFeatureSpecializationPart(SysMLParser::FeatureSpecializationPartContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUsageCompletion(SysMLParser::UsageCompletionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUsageBody(SysMLParser::UsageBodyContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitValuePart(SysMLParser::ValuePartContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFeatureValue(SysMLParser::FeatureValueContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitExpression(SysMLParser::ExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLogicalOrExpression(SysMLParser::LogicalOrExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLogicalAndExpression(SysMLParser::LogicalAndExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitComparisonExpression(SysMLParser::ComparisonExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitComparisonOp(SysMLParser::ComparisonOpContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAdditiveExpression(SysMLParser::AdditiveExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitMultiplicativeExpression(SysMLParser::MultiplicativeExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUnaryExpression(SysMLParser::UnaryExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPrimaryExpression(SysMLParser::PrimaryExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUnitExpression(SysMLParser::UnitExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUnitTerm(SysMLParser::UnitTermContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLiteralExpression(SysMLParser::LiteralExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFeatureReferenceExpression(SysMLParser::FeatureReferenceExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDefinition(SysMLParser::DefinitionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPartDefinition(SysMLParser::PartDefinitionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitActionDefinition(SysMLParser::ActionDefinitionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCalculationDefinition(SysMLParser::CalculationDefinitionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCalculationBody(SysMLParser::CalculationBodyContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCalculationBodyItem(SysMLParser::CalculationBodyItemContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitReturnParameter(SysMLParser::ReturnParameterContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStateDefinition(SysMLParser::StateDefinitionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitRequirementDefinition(SysMLParser::RequirementDefinitionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConstraintDefinition(SysMLParser::ConstraintDefinitionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPortDefinition(SysMLParser::PortDefinitionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConnectorDefinition(SysMLParser::ConnectorDefinitionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitInterfaceDefinition(SysMLParser::InterfaceDefinitionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDatatypeDefinition(SysMLParser::DatatypeDefinitionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitClassDefinition(SysMLParser::ClassDefinitionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitEnumDefinition(SysMLParser::EnumDefinitionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAttributeDefinition(SysMLParser::AttributeDefinitionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConnectionDefinition(SysMLParser::ConnectionDefinitionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAllocationDefinition(SysMLParser::AllocationDefinitionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitItemDefinition(SysMLParser::ItemDefinitionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitViewpointDefinition(SysMLParser::ViewpointDefinitionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitViewDefinition(SysMLParser::ViewDefinitionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConcernDefinition(SysMLParser::ConcernDefinitionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPartUsage(SysMLParser::PartUsageContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitActionUsage(SysMLParser::ActionUsageContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAnalysisUsage(SysMLParser::AnalysisUsageContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAnalysisBody(SysMLParser::AnalysisBodyContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAnalysisBodyItem(SysMLParser::AnalysisBodyItemContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSubjectAssignment(SysMLParser::SubjectAssignmentContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitRenderingUsage(SysMLParser::RenderingUsageContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStateUsage(SysMLParser::StateUsageContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitRequirementUsage(SysMLParser::RequirementUsageContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConstraintUsage(SysMLParser::ConstraintUsageContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPortUsage(SysMLParser::PortUsageContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConnectorUsage(SysMLParser::ConnectorUsageContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFlowUsage(SysMLParser::FlowUsageContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConnectionUsage(SysMLParser::ConnectionUsageContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAllocationUsage(SysMLParser::AllocationUsageContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitItemUsage(SysMLParser::ItemUsageContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitViewUsage(SysMLParser::ViewUsageContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPartBody(SysMLParser::PartBodyContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitActionBody(SysMLParser::ActionBodyContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitActionBodyItem(SysMLParser::ActionBodyItemContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStateBodyOld(SysMLParser::StateBodyOldContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitExhibitStatement(SysMLParser::ExhibitStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStateMachineKind(SysMLParser::StateMachineKindContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStateBody(SysMLParser::StateBodyContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStateDefinitionBody(SysMLParser::StateDefinitionBodyContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStateBodyItem(SysMLParser::StateBodyItemContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConstraintStatement(SysMLParser::ConstraintStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStateMember(SysMLParser::StateMemberContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTransitionMember(SysMLParser::TransitionMemberContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitRefStatement(SysMLParser::RefStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStateActionMember(SysMLParser::StateActionMemberContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitEntryActionMember(SysMLParser::EntryActionMemberContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDoActionMember(SysMLParser::DoActionMemberContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitExitActionMember(SysMLParser::ExitActionMemberContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStateActionUsage(SysMLParser::StateActionUsageContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitRequirementBody(SysMLParser::RequirementBodyContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitViewpointBody(SysMLParser::ViewpointBodyContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitViewpointBodyItem(SysMLParser::ViewpointBodyItemContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFrameConcernMember(SysMLParser::FrameConcernMemberContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitViewDefinitionBody(SysMLParser::ViewDefinitionBodyContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitViewDefinitionBodyItem(SysMLParser::ViewDefinitionBodyItemContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitElementFilterMember(SysMLParser::ElementFilterMemberContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFilterExpression(SysMLParser::FilterExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitMetadataAccessExpression(SysMLParser::MetadataAccessExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitViewRenderingMember(SysMLParser::ViewRenderingMemberContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitViewRenderingUsage(SysMLParser::ViewRenderingUsageContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitViewBody(SysMLParser::ViewBodyContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitViewBodyItem(SysMLParser::ViewBodyItemContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitExposeStatement(SysMLParser::ExposeStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitMembershipExpose(SysMLParser::MembershipExposeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitNamespaceExpose(SysMLParser::NamespaceExposeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConcernBody(SysMLParser::ConcernBodyContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConcernBodyItem(SysMLParser::ConcernBodyItemContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitRequirementMember(SysMLParser::RequirementMemberContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitRequirementConstraintMember(SysMLParser::RequirementConstraintMemberContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPortBody(SysMLParser::PortBodyContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConnectorBody(SysMLParser::ConnectorBodyContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFlowBody(SysMLParser::FlowBodyContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitInterfaceBody(SysMLParser::InterfaceBodyContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDatatypeBody(SysMLParser::DatatypeBodyContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitClassBody(SysMLParser::ClassBodyContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitEnumBody(SysMLParser::EnumBodyContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAnnotatingMember(SysMLParser::AnnotatingMemberContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDocStatement(SysMLParser::DocStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCommentStatement(SysMLParser::CommentStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUsage(SysMLParser::UsageContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStatement(SysMLParser::StatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitBindStatement(SysMLParser::BindStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConnectStatement(SysMLParser::ConnectStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAllocateStatement(SysMLParser::AllocateStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAllocationPart(SysMLParser::AllocationPartContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitBinaryAllocationPart(SysMLParser::BinaryAllocationPartContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitNaryAllocationPart(SysMLParser::NaryAllocationPartContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitEndStatement(SysMLParser::EndStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFlowDefinition(SysMLParser::FlowDefinitionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFlowStatement(SysMLParser::FlowStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFlowDeclaration(SysMLParser::FlowDeclarationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFlowEndMember(SysMLParser::FlowEndMemberContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPayloadFeatureMember(SysMLParser::PayloadFeatureMemberContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPerformStatement(SysMLParser::PerformStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAssertStatement(SysMLParser::AssertStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitRequireStatement(SysMLParser::RequireStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSatisfyStatement(SysMLParser::SatisfyStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitRequirementBodyItem(SysMLParser::RequirementBodyItemContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAssumeStatement(SysMLParser::AssumeStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitRedefinesStatement(SysMLParser::RedefinesStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitActionInputOutput(SysMLParser::ActionInputOutputContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConnectorPart(SysMLParser::ConnectorPartContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitBinaryConnectorPart(SysMLParser::BinaryConnectorPartContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitNaryConnectorPart(SysMLParser::NaryConnectorPartContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConnectorEndMember(SysMLParser::ConnectorEndMemberContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitOwnedReferenceSubsetting(SysMLParser::OwnedReferenceSubsettingContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitReferencesKeyword(SysMLParser::ReferencesKeywordContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitOwnedMultiplicity(SysMLParser::OwnedMultiplicityContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTransition(SysMLParser::TransitionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTransitionBody(SysMLParser::TransitionBodyContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSimpleTransitionBody(SysMLParser::SimpleTransitionBodyContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitComplexTransitionBody(SysMLParser::ComplexTransitionBodyContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTransitionTriggerPart(SysMLParser::TransitionTriggerPartContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAcceptClause(SysMLParser::AcceptClauseContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAcceptItem(SysMLParser::AcceptItemContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAcceptExpression(SysMLParser::AcceptExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTransitionGuardPart(SysMLParser::TransitionGuardPartContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTransitionEffectPart(SysMLParser::TransitionEffectPartContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTransitionAction(SysMLParser::TransitionActionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSendStatement(SysMLParser::SendStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSendActionPart(SysMLParser::SendActionPartContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitActionReference(SysMLParser::ActionReferenceContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitActionInvocation(SysMLParser::ActionInvocationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitActionArgument(SysMLParser::ActionArgumentContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStateReference(SysMLParser::StateReferenceContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTriggerExpression(SysMLParser::TriggerExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitGuardExpression(SysMLParser::GuardExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitEffectExpression(SysMLParser::EffectExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitEnumValue(SysMLParser::EnumValueContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAttribute(SysMLParser::AttributeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFeaturePrefix(SysMLParser::FeaturePrefixContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAttributeContent(SysMLParser::AttributeContentContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAttributeExpressions(SysMLParser::AttributeExpressionsContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSubsettingExpression(SysMLParser::SubsettingExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitRedefinitionExpression(SysMLParser::RedefinitionExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTypingExpression(SysMLParser::TypingExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSubclassificationExpression(SysMLParser::SubclassificationExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAttributeBody(SysMLParser::AttributeBodyContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitImportStatement(SysMLParser::ImportStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitMembershipImport(SysMLParser::MembershipImportContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitImportedMembership(SysMLParser::ImportedMembershipContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitNamespaceImport(SysMLParser::NamespaceImportContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitImportedNamespace(SysMLParser::ImportedNamespaceContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFilterPackage(SysMLParser::FilterPackageContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitImportPrefix(SysMLParser::ImportPrefixContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitRelationshipBody(SysMLParser::RelationshipBodyContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitRelationshipBodyItem(SysMLParser::RelationshipBodyItemContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitRelationship(SysMLParser::RelationshipContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSubclassification(SysMLParser::SubclassificationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSubsetting(SysMLParser::SubsettingContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitRedefinition(SysMLParser::RedefinitionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConjugation(SysMLParser::ConjugationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDependency(SysMLParser::DependencyContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTypeExpression(SysMLParser::TypeExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFeatureExpression(SysMLParser::FeatureExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitElementExpression(SysMLParser::ElementExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitQualifiedName(SysMLParser::QualifiedNameContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitMemberAccessExpression(SysMLParser::MemberAccessExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitIdentifier(SysMLParser::IdentifierContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitBlockComment(SysMLParser::BlockCommentContext *ctx) override {
    return visitChildren(ctx);
  }


};

}  // namespace SysML
