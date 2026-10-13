
// Generated from D:/Document/7.AWorks/BlockSim/2.Development/SysML/parser/grammar/SysML.g4 by ANTLR 4.13.2

#pragma once


#include "antlr4-runtime.h"
#include "SysMLParser.h"


namespace SysML {

/**
 * This class defines an abstract visitor for a parse tree
 * produced by SysMLParser.
 */
class  SysMLVisitor : public antlr4::tree::AbstractParseTreeVisitor {
public:

  /**
   * Visit parse trees produced by SysMLParser.
   */
    virtual std::any visitFile(SysMLParser::FileContext *context) = 0;

    virtual std::any visitPackage(SysMLParser::PackageContext *context) = 0;

    virtual std::any visitPackageBody(SysMLParser::PackageBodyContext *context) = 0;

    virtual std::any visitBasicDefinitionPrefix(SysMLParser::BasicDefinitionPrefixContext *context) = 0;

    virtual std::any visitDefinitionExtensionKeyword(SysMLParser::DefinitionExtensionKeywordContext *context) = 0;

    virtual std::any visitDefinitionPrefix(SysMLParser::DefinitionPrefixContext *context) = 0;

    virtual std::any visitOccurrenceDefinitionPrefix(SysMLParser::OccurrenceDefinitionPrefixContext *context) = 0;

    virtual std::any visitEmptyMultiplicityMember(SysMLParser::EmptyMultiplicityMemberContext *context) = 0;

    virtual std::any visitFeatureDirection(SysMLParser::FeatureDirectionContext *context) = 0;

    virtual std::any visitRefPrefix(SysMLParser::RefPrefixContext *context) = 0;

    virtual std::any visitBasicUsagePrefix(SysMLParser::BasicUsagePrefixContext *context) = 0;

    virtual std::any visitEndUsagePrefix(SysMLParser::EndUsagePrefixContext *context) = 0;

    virtual std::any visitOwnedCrossFeatureMember(SysMLParser::OwnedCrossFeatureMemberContext *context) = 0;

    virtual std::any visitUsageExtensionKeyword(SysMLParser::UsageExtensionKeywordContext *context) = 0;

    virtual std::any visitUnextendedUsagePrefix(SysMLParser::UnextendedUsagePrefixContext *context) = 0;

    virtual std::any visitUsagePrefix(SysMLParser::UsagePrefixContext *context) = 0;

    virtual std::any visitOccurrenceUsagePrefix(SysMLParser::OccurrenceUsagePrefixContext *context) = 0;

    virtual std::any visitPortionKind(SysMLParser::PortionKindContext *context) = 0;

    virtual std::any visitIdentification(SysMLParser::IdentificationContext *context) = 0;

    virtual std::any visitName(SysMLParser::NameContext *context) = 0;

    virtual std::any visitDefinitionDeclaration(SysMLParser::DefinitionDeclarationContext *context) = 0;

    virtual std::any visitSubclassificationPart(SysMLParser::SubclassificationPartContext *context) = 0;

    virtual std::any visitOwnedSubclassification(SysMLParser::OwnedSubclassificationContext *context) = 0;

    virtual std::any visitDefinitionBody(SysMLParser::DefinitionBodyContext *context) = 0;

    virtual std::any visitDefinitionBodyItem(SysMLParser::DefinitionBodyItemContext *context) = 0;

    virtual std::any visitMetadataAnnotation(SysMLParser::MetadataAnnotationContext *context) = 0;

    virtual std::any visitMetadataAssignment(SysMLParser::MetadataAssignmentContext *context) = 0;

    virtual std::any visitSimplifiedAttribute(SysMLParser::SimplifiedAttributeContext *context) = 0;

    virtual std::any visitDefinitionMember(SysMLParser::DefinitionMemberContext *context) = 0;

    virtual std::any visitUsageMember(SysMLParser::UsageMemberContext *context) = 0;

    virtual std::any visitVisibilityIndicator(SysMLParser::VisibilityIndicatorContext *context) = 0;

    virtual std::any visitUsageDeclaration(SysMLParser::UsageDeclarationContext *context) = 0;

    virtual std::any visitFeatureDeclaration(SysMLParser::FeatureDeclarationContext *context) = 0;

    virtual std::any visitFeatureSpecializationPart(SysMLParser::FeatureSpecializationPartContext *context) = 0;

    virtual std::any visitUsageCompletion(SysMLParser::UsageCompletionContext *context) = 0;

    virtual std::any visitUsageBody(SysMLParser::UsageBodyContext *context) = 0;

    virtual std::any visitValuePart(SysMLParser::ValuePartContext *context) = 0;

    virtual std::any visitFeatureValue(SysMLParser::FeatureValueContext *context) = 0;

    virtual std::any visitExpression(SysMLParser::ExpressionContext *context) = 0;

    virtual std::any visitLogicalOrExpression(SysMLParser::LogicalOrExpressionContext *context) = 0;

    virtual std::any visitLogicalAndExpression(SysMLParser::LogicalAndExpressionContext *context) = 0;

    virtual std::any visitComparisonExpression(SysMLParser::ComparisonExpressionContext *context) = 0;

    virtual std::any visitComparisonOp(SysMLParser::ComparisonOpContext *context) = 0;

    virtual std::any visitAdditiveExpression(SysMLParser::AdditiveExpressionContext *context) = 0;

    virtual std::any visitMultiplicativeExpression(SysMLParser::MultiplicativeExpressionContext *context) = 0;

    virtual std::any visitUnaryExpression(SysMLParser::UnaryExpressionContext *context) = 0;

    virtual std::any visitPrimaryExpression(SysMLParser::PrimaryExpressionContext *context) = 0;

    virtual std::any visitUnitExpression(SysMLParser::UnitExpressionContext *context) = 0;

    virtual std::any visitUnitTerm(SysMLParser::UnitTermContext *context) = 0;

    virtual std::any visitLiteralExpression(SysMLParser::LiteralExpressionContext *context) = 0;

    virtual std::any visitFeatureReferenceExpression(SysMLParser::FeatureReferenceExpressionContext *context) = 0;

    virtual std::any visitDefinition(SysMLParser::DefinitionContext *context) = 0;

    virtual std::any visitPartDefinition(SysMLParser::PartDefinitionContext *context) = 0;

    virtual std::any visitActionDefinition(SysMLParser::ActionDefinitionContext *context) = 0;

    virtual std::any visitCalculationDefinition(SysMLParser::CalculationDefinitionContext *context) = 0;

    virtual std::any visitCalculationBody(SysMLParser::CalculationBodyContext *context) = 0;

    virtual std::any visitCalculationBodyItem(SysMLParser::CalculationBodyItemContext *context) = 0;

    virtual std::any visitReturnParameter(SysMLParser::ReturnParameterContext *context) = 0;

    virtual std::any visitStateDefinition(SysMLParser::StateDefinitionContext *context) = 0;

    virtual std::any visitRequirementDefinition(SysMLParser::RequirementDefinitionContext *context) = 0;

    virtual std::any visitConstraintDefinition(SysMLParser::ConstraintDefinitionContext *context) = 0;

    virtual std::any visitPortDefinition(SysMLParser::PortDefinitionContext *context) = 0;

    virtual std::any visitConnectorDefinition(SysMLParser::ConnectorDefinitionContext *context) = 0;

    virtual std::any visitInterfaceDefinition(SysMLParser::InterfaceDefinitionContext *context) = 0;

    virtual std::any visitDatatypeDefinition(SysMLParser::DatatypeDefinitionContext *context) = 0;

    virtual std::any visitClassDefinition(SysMLParser::ClassDefinitionContext *context) = 0;

    virtual std::any visitEnumDefinition(SysMLParser::EnumDefinitionContext *context) = 0;

    virtual std::any visitAttributeDefinition(SysMLParser::AttributeDefinitionContext *context) = 0;

    virtual std::any visitConnectionDefinition(SysMLParser::ConnectionDefinitionContext *context) = 0;

    virtual std::any visitAllocationDefinition(SysMLParser::AllocationDefinitionContext *context) = 0;

    virtual std::any visitItemDefinition(SysMLParser::ItemDefinitionContext *context) = 0;

    virtual std::any visitViewpointDefinition(SysMLParser::ViewpointDefinitionContext *context) = 0;

    virtual std::any visitViewDefinition(SysMLParser::ViewDefinitionContext *context) = 0;

    virtual std::any visitConcernDefinition(SysMLParser::ConcernDefinitionContext *context) = 0;

    virtual std::any visitPartUsage(SysMLParser::PartUsageContext *context) = 0;

    virtual std::any visitActionUsage(SysMLParser::ActionUsageContext *context) = 0;

    virtual std::any visitAnalysisUsage(SysMLParser::AnalysisUsageContext *context) = 0;

    virtual std::any visitAnalysisBody(SysMLParser::AnalysisBodyContext *context) = 0;

    virtual std::any visitAnalysisBodyItem(SysMLParser::AnalysisBodyItemContext *context) = 0;

    virtual std::any visitSubjectAssignment(SysMLParser::SubjectAssignmentContext *context) = 0;

    virtual std::any visitRenderingUsage(SysMLParser::RenderingUsageContext *context) = 0;

    virtual std::any visitStateUsage(SysMLParser::StateUsageContext *context) = 0;

    virtual std::any visitRequirementUsage(SysMLParser::RequirementUsageContext *context) = 0;

    virtual std::any visitConstraintUsage(SysMLParser::ConstraintUsageContext *context) = 0;

    virtual std::any visitPortUsage(SysMLParser::PortUsageContext *context) = 0;

    virtual std::any visitConnectorUsage(SysMLParser::ConnectorUsageContext *context) = 0;

    virtual std::any visitFlowUsage(SysMLParser::FlowUsageContext *context) = 0;

    virtual std::any visitConnectionUsage(SysMLParser::ConnectionUsageContext *context) = 0;

    virtual std::any visitAllocationUsage(SysMLParser::AllocationUsageContext *context) = 0;

    virtual std::any visitItemUsage(SysMLParser::ItemUsageContext *context) = 0;

    virtual std::any visitViewUsage(SysMLParser::ViewUsageContext *context) = 0;

    virtual std::any visitPartBody(SysMLParser::PartBodyContext *context) = 0;

    virtual std::any visitActionBody(SysMLParser::ActionBodyContext *context) = 0;

    virtual std::any visitActionBodyItem(SysMLParser::ActionBodyItemContext *context) = 0;

    virtual std::any visitStateBodyOld(SysMLParser::StateBodyOldContext *context) = 0;

    virtual std::any visitExhibitStatement(SysMLParser::ExhibitStatementContext *context) = 0;

    virtual std::any visitStateMachineKind(SysMLParser::StateMachineKindContext *context) = 0;

    virtual std::any visitStateBody(SysMLParser::StateBodyContext *context) = 0;

    virtual std::any visitStateDefinitionBody(SysMLParser::StateDefinitionBodyContext *context) = 0;

    virtual std::any visitStateBodyItem(SysMLParser::StateBodyItemContext *context) = 0;

    virtual std::any visitConstraintStatement(SysMLParser::ConstraintStatementContext *context) = 0;

    virtual std::any visitStateMember(SysMLParser::StateMemberContext *context) = 0;

    virtual std::any visitTransitionMember(SysMLParser::TransitionMemberContext *context) = 0;

    virtual std::any visitRefStatement(SysMLParser::RefStatementContext *context) = 0;

    virtual std::any visitStateActionMember(SysMLParser::StateActionMemberContext *context) = 0;

    virtual std::any visitEntryActionMember(SysMLParser::EntryActionMemberContext *context) = 0;

    virtual std::any visitDoActionMember(SysMLParser::DoActionMemberContext *context) = 0;

    virtual std::any visitExitActionMember(SysMLParser::ExitActionMemberContext *context) = 0;

    virtual std::any visitStateActionUsage(SysMLParser::StateActionUsageContext *context) = 0;

    virtual std::any visitRequirementBody(SysMLParser::RequirementBodyContext *context) = 0;

    virtual std::any visitViewpointBody(SysMLParser::ViewpointBodyContext *context) = 0;

    virtual std::any visitViewpointBodyItem(SysMLParser::ViewpointBodyItemContext *context) = 0;

    virtual std::any visitFrameConcernMember(SysMLParser::FrameConcernMemberContext *context) = 0;

    virtual std::any visitViewDefinitionBody(SysMLParser::ViewDefinitionBodyContext *context) = 0;

    virtual std::any visitViewDefinitionBodyItem(SysMLParser::ViewDefinitionBodyItemContext *context) = 0;

    virtual std::any visitElementFilterMember(SysMLParser::ElementFilterMemberContext *context) = 0;

    virtual std::any visitFilterExpression(SysMLParser::FilterExpressionContext *context) = 0;

    virtual std::any visitMetadataAccessExpression(SysMLParser::MetadataAccessExpressionContext *context) = 0;

    virtual std::any visitViewRenderingMember(SysMLParser::ViewRenderingMemberContext *context) = 0;

    virtual std::any visitViewRenderingUsage(SysMLParser::ViewRenderingUsageContext *context) = 0;

    virtual std::any visitViewBody(SysMLParser::ViewBodyContext *context) = 0;

    virtual std::any visitViewBodyItem(SysMLParser::ViewBodyItemContext *context) = 0;

    virtual std::any visitExposeStatement(SysMLParser::ExposeStatementContext *context) = 0;

    virtual std::any visitMembershipExpose(SysMLParser::MembershipExposeContext *context) = 0;

    virtual std::any visitNamespaceExpose(SysMLParser::NamespaceExposeContext *context) = 0;

    virtual std::any visitConcernBody(SysMLParser::ConcernBodyContext *context) = 0;

    virtual std::any visitConcernBodyItem(SysMLParser::ConcernBodyItemContext *context) = 0;

    virtual std::any visitRequirementMember(SysMLParser::RequirementMemberContext *context) = 0;

    virtual std::any visitRequirementConstraintMember(SysMLParser::RequirementConstraintMemberContext *context) = 0;

    virtual std::any visitPortBody(SysMLParser::PortBodyContext *context) = 0;

    virtual std::any visitConnectorBody(SysMLParser::ConnectorBodyContext *context) = 0;

    virtual std::any visitFlowBody(SysMLParser::FlowBodyContext *context) = 0;

    virtual std::any visitInterfaceBody(SysMLParser::InterfaceBodyContext *context) = 0;

    virtual std::any visitDatatypeBody(SysMLParser::DatatypeBodyContext *context) = 0;

    virtual std::any visitClassBody(SysMLParser::ClassBodyContext *context) = 0;

    virtual std::any visitEnumBody(SysMLParser::EnumBodyContext *context) = 0;

    virtual std::any visitAnnotatingMember(SysMLParser::AnnotatingMemberContext *context) = 0;

    virtual std::any visitDocStatement(SysMLParser::DocStatementContext *context) = 0;

    virtual std::any visitCommentStatement(SysMLParser::CommentStatementContext *context) = 0;

    virtual std::any visitUsage(SysMLParser::UsageContext *context) = 0;

    virtual std::any visitStatement(SysMLParser::StatementContext *context) = 0;

    virtual std::any visitBindStatement(SysMLParser::BindStatementContext *context) = 0;

    virtual std::any visitConnectStatement(SysMLParser::ConnectStatementContext *context) = 0;

    virtual std::any visitAllocateStatement(SysMLParser::AllocateStatementContext *context) = 0;

    virtual std::any visitAllocationPart(SysMLParser::AllocationPartContext *context) = 0;

    virtual std::any visitBinaryAllocationPart(SysMLParser::BinaryAllocationPartContext *context) = 0;

    virtual std::any visitNaryAllocationPart(SysMLParser::NaryAllocationPartContext *context) = 0;

    virtual std::any visitEndStatement(SysMLParser::EndStatementContext *context) = 0;

    virtual std::any visitFlowDefinition(SysMLParser::FlowDefinitionContext *context) = 0;

    virtual std::any visitFlowStatement(SysMLParser::FlowStatementContext *context) = 0;

    virtual std::any visitFlowDeclaration(SysMLParser::FlowDeclarationContext *context) = 0;

    virtual std::any visitFlowEndMember(SysMLParser::FlowEndMemberContext *context) = 0;

    virtual std::any visitPayloadFeatureMember(SysMLParser::PayloadFeatureMemberContext *context) = 0;

    virtual std::any visitPerformStatement(SysMLParser::PerformStatementContext *context) = 0;

    virtual std::any visitAssertStatement(SysMLParser::AssertStatementContext *context) = 0;

    virtual std::any visitRequireStatement(SysMLParser::RequireStatementContext *context) = 0;

    virtual std::any visitSatisfyStatement(SysMLParser::SatisfyStatementContext *context) = 0;

    virtual std::any visitRequirementBodyItem(SysMLParser::RequirementBodyItemContext *context) = 0;

    virtual std::any visitAssumeStatement(SysMLParser::AssumeStatementContext *context) = 0;

    virtual std::any visitRedefinesStatement(SysMLParser::RedefinesStatementContext *context) = 0;

    virtual std::any visitActionInputOutput(SysMLParser::ActionInputOutputContext *context) = 0;

    virtual std::any visitConnectorPart(SysMLParser::ConnectorPartContext *context) = 0;

    virtual std::any visitBinaryConnectorPart(SysMLParser::BinaryConnectorPartContext *context) = 0;

    virtual std::any visitNaryConnectorPart(SysMLParser::NaryConnectorPartContext *context) = 0;

    virtual std::any visitConnectorEndMember(SysMLParser::ConnectorEndMemberContext *context) = 0;

    virtual std::any visitOwnedReferenceSubsetting(SysMLParser::OwnedReferenceSubsettingContext *context) = 0;

    virtual std::any visitReferencesKeyword(SysMLParser::ReferencesKeywordContext *context) = 0;

    virtual std::any visitOwnedMultiplicity(SysMLParser::OwnedMultiplicityContext *context) = 0;

    virtual std::any visitTransition(SysMLParser::TransitionContext *context) = 0;

    virtual std::any visitTransitionBody(SysMLParser::TransitionBodyContext *context) = 0;

    virtual std::any visitSimpleTransitionBody(SysMLParser::SimpleTransitionBodyContext *context) = 0;

    virtual std::any visitComplexTransitionBody(SysMLParser::ComplexTransitionBodyContext *context) = 0;

    virtual std::any visitTransitionTriggerPart(SysMLParser::TransitionTriggerPartContext *context) = 0;

    virtual std::any visitAcceptClause(SysMLParser::AcceptClauseContext *context) = 0;

    virtual std::any visitAcceptItem(SysMLParser::AcceptItemContext *context) = 0;

    virtual std::any visitAcceptExpression(SysMLParser::AcceptExpressionContext *context) = 0;

    virtual std::any visitTransitionGuardPart(SysMLParser::TransitionGuardPartContext *context) = 0;

    virtual std::any visitTransitionEffectPart(SysMLParser::TransitionEffectPartContext *context) = 0;

    virtual std::any visitTransitionAction(SysMLParser::TransitionActionContext *context) = 0;

    virtual std::any visitSendStatement(SysMLParser::SendStatementContext *context) = 0;

    virtual std::any visitSendActionPart(SysMLParser::SendActionPartContext *context) = 0;

    virtual std::any visitActionReference(SysMLParser::ActionReferenceContext *context) = 0;

    virtual std::any visitActionInvocation(SysMLParser::ActionInvocationContext *context) = 0;

    virtual std::any visitActionArgument(SysMLParser::ActionArgumentContext *context) = 0;

    virtual std::any visitStateReference(SysMLParser::StateReferenceContext *context) = 0;

    virtual std::any visitTriggerExpression(SysMLParser::TriggerExpressionContext *context) = 0;

    virtual std::any visitGuardExpression(SysMLParser::GuardExpressionContext *context) = 0;

    virtual std::any visitEffectExpression(SysMLParser::EffectExpressionContext *context) = 0;

    virtual std::any visitEnumValue(SysMLParser::EnumValueContext *context) = 0;

    virtual std::any visitAttribute(SysMLParser::AttributeContext *context) = 0;

    virtual std::any visitFeaturePrefix(SysMLParser::FeaturePrefixContext *context) = 0;

    virtual std::any visitAttributeContent(SysMLParser::AttributeContentContext *context) = 0;

    virtual std::any visitAttributeExpressions(SysMLParser::AttributeExpressionsContext *context) = 0;

    virtual std::any visitSubsettingExpression(SysMLParser::SubsettingExpressionContext *context) = 0;

    virtual std::any visitRedefinitionExpression(SysMLParser::RedefinitionExpressionContext *context) = 0;

    virtual std::any visitTypingExpression(SysMLParser::TypingExpressionContext *context) = 0;

    virtual std::any visitSubclassificationExpression(SysMLParser::SubclassificationExpressionContext *context) = 0;

    virtual std::any visitAttributeBody(SysMLParser::AttributeBodyContext *context) = 0;

    virtual std::any visitImportStatement(SysMLParser::ImportStatementContext *context) = 0;

    virtual std::any visitMembershipImport(SysMLParser::MembershipImportContext *context) = 0;

    virtual std::any visitImportedMembership(SysMLParser::ImportedMembershipContext *context) = 0;

    virtual std::any visitNamespaceImport(SysMLParser::NamespaceImportContext *context) = 0;

    virtual std::any visitImportedNamespace(SysMLParser::ImportedNamespaceContext *context) = 0;

    virtual std::any visitFilterPackage(SysMLParser::FilterPackageContext *context) = 0;

    virtual std::any visitImportPrefix(SysMLParser::ImportPrefixContext *context) = 0;

    virtual std::any visitRelationshipBody(SysMLParser::RelationshipBodyContext *context) = 0;

    virtual std::any visitRelationshipBodyItem(SysMLParser::RelationshipBodyItemContext *context) = 0;

    virtual std::any visitRelationship(SysMLParser::RelationshipContext *context) = 0;

    virtual std::any visitSubclassification(SysMLParser::SubclassificationContext *context) = 0;

    virtual std::any visitSubsetting(SysMLParser::SubsettingContext *context) = 0;

    virtual std::any visitRedefinition(SysMLParser::RedefinitionContext *context) = 0;

    virtual std::any visitConjugation(SysMLParser::ConjugationContext *context) = 0;

    virtual std::any visitDependency(SysMLParser::DependencyContext *context) = 0;

    virtual std::any visitTypeExpression(SysMLParser::TypeExpressionContext *context) = 0;

    virtual std::any visitFeatureExpression(SysMLParser::FeatureExpressionContext *context) = 0;

    virtual std::any visitElementExpression(SysMLParser::ElementExpressionContext *context) = 0;

    virtual std::any visitQualifiedName(SysMLParser::QualifiedNameContext *context) = 0;

    virtual std::any visitMemberAccessExpression(SysMLParser::MemberAccessExpressionContext *context) = 0;

    virtual std::any visitIdentifier(SysMLParser::IdentifierContext *context) = 0;

    virtual std::any visitBlockComment(SysMLParser::BlockCommentContext *context) = 0;


};

}  // namespace SysML
