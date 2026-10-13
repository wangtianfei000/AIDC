
// Generated from D:/Document/7.AWorks/BlockSim/2.Development/SysML/parser/grammar/SysML.g4 by ANTLR 4.13.2

#pragma once


#include "antlr4-runtime.h"


namespace SysML {


class  SysMLParser : public antlr4::Parser {
public:
  enum {
    T__0 = 1, T__1 = 2, T__2 = 3, T__3 = 4, T__4 = 5, T__5 = 6, T__6 = 7, 
    T__7 = 8, T__8 = 9, T__9 = 10, T__10 = 11, T__11 = 12, T__12 = 13, T__13 = 14, 
    T__14 = 15, T__15 = 16, T__16 = 17, T__17 = 18, T__18 = 19, T__19 = 20, 
    T__20 = 21, T__21 = 22, T__22 = 23, T__23 = 24, T__24 = 25, T__25 = 26, 
    T__26 = 27, T__27 = 28, T__28 = 29, T__29 = 30, T__30 = 31, T__31 = 32, 
    T__32 = 33, T__33 = 34, T__34 = 35, T__35 = 36, T__36 = 37, T__37 = 38, 
    T__38 = 39, T__39 = 40, T__40 = 41, T__41 = 42, T__42 = 43, T__43 = 44, 
    T__44 = 45, T__45 = 46, T__46 = 47, T__47 = 48, T__48 = 49, T__49 = 50, 
    T__50 = 51, T__51 = 52, T__52 = 53, T__53 = 54, T__54 = 55, T__55 = 56, 
    T__56 = 57, T__57 = 58, T__58 = 59, T__59 = 60, T__60 = 61, T__61 = 62, 
    T__62 = 63, T__63 = 64, T__64 = 65, T__65 = 66, T__66 = 67, T__67 = 68, 
    T__68 = 69, T__69 = 70, T__70 = 71, T__71 = 72, T__72 = 73, T__73 = 74, 
    T__74 = 75, T__75 = 76, T__76 = 77, T__77 = 78, T__78 = 79, T__79 = 80, 
    T__80 = 81, T__81 = 82, T__82 = 83, T__83 = 84, T__84 = 85, T__85 = 86, 
    T__86 = 87, T__87 = 88, T__88 = 89, T__89 = 90, T__90 = 91, T__91 = 92, 
    T__92 = 93, T__93 = 94, T__94 = 95, T__95 = 96, T__96 = 97, T__97 = 98, 
    T__98 = 99, T__99 = 100, T__100 = 101, T__101 = 102, T__102 = 103, T__103 = 104, 
    T__104 = 105, T__105 = 106, T__106 = 107, T__107 = 108, T__108 = 109, 
    T__109 = 110, T__110 = 111, T__111 = 112, T__112 = 113, T__113 = 114, 
    T__114 = 115, T__115 = 116, T__116 = 117, T__117 = 118, T__118 = 119, 
    T__119 = 120, T__120 = 121, T__121 = 122, Real = 123, Boolean = 124, 
    String = 125, IDENTIFIER = 126, Integer = 127, SUBSETS_OP = 128, REDEFINES_OP = 129, 
    COLON = 130, WS = 131, LINE_COMMENT = 132, BLOCK_COMMENT_TOKEN = 133, 
    BLOCK_COMMENT = 134
  };

  enum {
    RuleFile = 0, RulePackage = 1, RulePackageBody = 2, RuleBasicDefinitionPrefix = 3, 
    RuleDefinitionExtensionKeyword = 4, RuleDefinitionPrefix = 5, RuleOccurrenceDefinitionPrefix = 6, 
    RuleEmptyMultiplicityMember = 7, RuleFeatureDirection = 8, RuleRefPrefix = 9, 
    RuleBasicUsagePrefix = 10, RuleEndUsagePrefix = 11, RuleOwnedCrossFeatureMember = 12, 
    RuleUsageExtensionKeyword = 13, RuleUnextendedUsagePrefix = 14, RuleUsagePrefix = 15, 
    RuleOccurrenceUsagePrefix = 16, RulePortionKind = 17, RuleIdentification = 18, 
    RuleName = 19, RuleDefinitionDeclaration = 20, RuleSubclassificationPart = 21, 
    RuleOwnedSubclassification = 22, RuleDefinitionBody = 23, RuleDefinitionBodyItem = 24, 
    RuleMetadataAnnotation = 25, RuleMetadataAssignment = 26, RuleSimplifiedAttribute = 27, 
    RuleDefinitionMember = 28, RuleUsageMember = 29, RuleVisibilityIndicator = 30, 
    RuleUsageDeclaration = 31, RuleFeatureDeclaration = 32, RuleFeatureSpecializationPart = 33, 
    RuleUsageCompletion = 34, RuleUsageBody = 35, RuleValuePart = 36, RuleFeatureValue = 37, 
    RuleExpression = 38, RuleLogicalOrExpression = 39, RuleLogicalAndExpression = 40, 
    RuleComparisonExpression = 41, RuleComparisonOp = 42, RuleAdditiveExpression = 43, 
    RuleMultiplicativeExpression = 44, RuleUnaryExpression = 45, RulePrimaryExpression = 46, 
    RuleUnitExpression = 47, RuleUnitTerm = 48, RuleLiteralExpression = 49, 
    RuleFeatureReferenceExpression = 50, RuleDefinition = 51, RulePartDefinition = 52, 
    RuleActionDefinition = 53, RuleCalculationDefinition = 54, RuleCalculationBody = 55, 
    RuleCalculationBodyItem = 56, RuleReturnParameter = 57, RuleStateDefinition = 58, 
    RuleRequirementDefinition = 59, RuleConstraintDefinition = 60, RulePortDefinition = 61, 
    RuleConnectorDefinition = 62, RuleInterfaceDefinition = 63, RuleDatatypeDefinition = 64, 
    RuleClassDefinition = 65, RuleEnumDefinition = 66, RuleAttributeDefinition = 67, 
    RuleConnectionDefinition = 68, RuleAllocationDefinition = 69, RuleItemDefinition = 70, 
    RuleViewpointDefinition = 71, RuleViewDefinition = 72, RuleConcernDefinition = 73, 
    RulePartUsage = 74, RuleActionUsage = 75, RuleAnalysisUsage = 76, RuleAnalysisBody = 77, 
    RuleAnalysisBodyItem = 78, RuleSubjectAssignment = 79, RuleRenderingUsage = 80, 
    RuleStateUsage = 81, RuleRequirementUsage = 82, RuleConstraintUsage = 83, 
    RulePortUsage = 84, RuleConnectorUsage = 85, RuleFlowUsage = 86, RuleConnectionUsage = 87, 
    RuleAllocationUsage = 88, RuleItemUsage = 89, RuleViewUsage = 90, RulePartBody = 91, 
    RuleActionBody = 92, RuleActionBodyItem = 93, RuleStateBodyOld = 94, 
    RuleExhibitStatement = 95, RuleStateMachineKind = 96, RuleStateBody = 97, 
    RuleStateDefinitionBody = 98, RuleStateBodyItem = 99, RuleConstraintStatement = 100, 
    RuleStateMember = 101, RuleTransitionMember = 102, RuleRefStatement = 103, 
    RuleStateActionMember = 104, RuleEntryActionMember = 105, RuleDoActionMember = 106, 
    RuleExitActionMember = 107, RuleStateActionUsage = 108, RuleRequirementBody = 109, 
    RuleViewpointBody = 110, RuleViewpointBodyItem = 111, RuleFrameConcernMember = 112, 
    RuleViewDefinitionBody = 113, RuleViewDefinitionBodyItem = 114, RuleElementFilterMember = 115, 
    RuleFilterExpression = 116, RuleMetadataAccessExpression = 117, RuleViewRenderingMember = 118, 
    RuleViewRenderingUsage = 119, RuleViewBody = 120, RuleViewBodyItem = 121, 
    RuleExposeStatement = 122, RuleMembershipExpose = 123, RuleNamespaceExpose = 124, 
    RuleConcernBody = 125, RuleConcernBodyItem = 126, RuleRequirementMember = 127, 
    RuleRequirementConstraintMember = 128, RulePortBody = 129, RuleConnectorBody = 130, 
    RuleFlowBody = 131, RuleInterfaceBody = 132, RuleDatatypeBody = 133, 
    RuleClassBody = 134, RuleEnumBody = 135, RuleAnnotatingMember = 136, 
    RuleDocStatement = 137, RuleCommentStatement = 138, RuleUsage = 139, 
    RuleStatement = 140, RuleBindStatement = 141, RuleConnectStatement = 142, 
    RuleAllocateStatement = 143, RuleAllocationPart = 144, RuleBinaryAllocationPart = 145, 
    RuleNaryAllocationPart = 146, RuleEndStatement = 147, RuleFlowDefinition = 148, 
    RuleFlowStatement = 149, RuleFlowDeclaration = 150, RuleFlowEndMember = 151, 
    RulePayloadFeatureMember = 152, RulePerformStatement = 153, RuleAssertStatement = 154, 
    RuleRequireStatement = 155, RuleSatisfyStatement = 156, RuleRequirementBodyItem = 157, 
    RuleAssumeStatement = 158, RuleRedefinesStatement = 159, RuleActionInputOutput = 160, 
    RuleConnectorPart = 161, RuleBinaryConnectorPart = 162, RuleNaryConnectorPart = 163, 
    RuleConnectorEndMember = 164, RuleOwnedReferenceSubsetting = 165, RuleReferencesKeyword = 166, 
    RuleOwnedMultiplicity = 167, RuleTransition = 168, RuleTransitionBody = 169, 
    RuleSimpleTransitionBody = 170, RuleComplexTransitionBody = 171, RuleTransitionTriggerPart = 172, 
    RuleAcceptClause = 173, RuleAcceptItem = 174, RuleAcceptExpression = 175, 
    RuleTransitionGuardPart = 176, RuleTransitionEffectPart = 177, RuleTransitionAction = 178, 
    RuleSendStatement = 179, RuleSendActionPart = 180, RuleActionReference = 181, 
    RuleActionInvocation = 182, RuleActionArgument = 183, RuleStateReference = 184, 
    RuleTriggerExpression = 185, RuleGuardExpression = 186, RuleEffectExpression = 187, 
    RuleEnumValue = 188, RuleAttribute = 189, RuleFeaturePrefix = 190, RuleAttributeContent = 191, 
    RuleAttributeExpressions = 192, RuleSubsettingExpression = 193, RuleRedefinitionExpression = 194, 
    RuleTypingExpression = 195, RuleSubclassificationExpression = 196, RuleAttributeBody = 197, 
    RuleImportStatement = 198, RuleMembershipImport = 199, RuleImportedMembership = 200, 
    RuleNamespaceImport = 201, RuleImportedNamespace = 202, RuleFilterPackage = 203, 
    RuleImportPrefix = 204, RuleRelationshipBody = 205, RuleRelationshipBodyItem = 206, 
    RuleRelationship = 207, RuleSubclassification = 208, RuleSubsetting = 209, 
    RuleRedefinition = 210, RuleConjugation = 211, RuleDependency = 212, 
    RuleTypeExpression = 213, RuleFeatureExpression = 214, RuleElementExpression = 215, 
    RuleQualifiedName = 216, RuleMemberAccessExpression = 217, RuleIdentifier = 218, 
    RuleBlockComment = 219
  };

  explicit SysMLParser(antlr4::TokenStream *input);

  SysMLParser(antlr4::TokenStream *input, const antlr4::atn::ParserATNSimulatorOptions &options);

  ~SysMLParser() override;

  std::string getGrammarFileName() const override;

  const antlr4::atn::ATN& getATN() const override;

  const std::vector<std::string>& getRuleNames() const override;

  const antlr4::dfa::Vocabulary& getVocabulary() const override;

  antlr4::atn::SerializedATNView getSerializedATN() const override;


  class FileContext;
  class PackageContext;
  class PackageBodyContext;
  class BasicDefinitionPrefixContext;
  class DefinitionExtensionKeywordContext;
  class DefinitionPrefixContext;
  class OccurrenceDefinitionPrefixContext;
  class EmptyMultiplicityMemberContext;
  class FeatureDirectionContext;
  class RefPrefixContext;
  class BasicUsagePrefixContext;
  class EndUsagePrefixContext;
  class OwnedCrossFeatureMemberContext;
  class UsageExtensionKeywordContext;
  class UnextendedUsagePrefixContext;
  class UsagePrefixContext;
  class OccurrenceUsagePrefixContext;
  class PortionKindContext;
  class IdentificationContext;
  class NameContext;
  class DefinitionDeclarationContext;
  class SubclassificationPartContext;
  class OwnedSubclassificationContext;
  class DefinitionBodyContext;
  class DefinitionBodyItemContext;
  class MetadataAnnotationContext;
  class MetadataAssignmentContext;
  class SimplifiedAttributeContext;
  class DefinitionMemberContext;
  class UsageMemberContext;
  class VisibilityIndicatorContext;
  class UsageDeclarationContext;
  class FeatureDeclarationContext;
  class FeatureSpecializationPartContext;
  class UsageCompletionContext;
  class UsageBodyContext;
  class ValuePartContext;
  class FeatureValueContext;
  class ExpressionContext;
  class LogicalOrExpressionContext;
  class LogicalAndExpressionContext;
  class ComparisonExpressionContext;
  class ComparisonOpContext;
  class AdditiveExpressionContext;
  class MultiplicativeExpressionContext;
  class UnaryExpressionContext;
  class PrimaryExpressionContext;
  class UnitExpressionContext;
  class UnitTermContext;
  class LiteralExpressionContext;
  class FeatureReferenceExpressionContext;
  class DefinitionContext;
  class PartDefinitionContext;
  class ActionDefinitionContext;
  class CalculationDefinitionContext;
  class CalculationBodyContext;
  class CalculationBodyItemContext;
  class ReturnParameterContext;
  class StateDefinitionContext;
  class RequirementDefinitionContext;
  class ConstraintDefinitionContext;
  class PortDefinitionContext;
  class ConnectorDefinitionContext;
  class InterfaceDefinitionContext;
  class DatatypeDefinitionContext;
  class ClassDefinitionContext;
  class EnumDefinitionContext;
  class AttributeDefinitionContext;
  class ConnectionDefinitionContext;
  class AllocationDefinitionContext;
  class ItemDefinitionContext;
  class ViewpointDefinitionContext;
  class ViewDefinitionContext;
  class ConcernDefinitionContext;
  class PartUsageContext;
  class ActionUsageContext;
  class AnalysisUsageContext;
  class AnalysisBodyContext;
  class AnalysisBodyItemContext;
  class SubjectAssignmentContext;
  class RenderingUsageContext;
  class StateUsageContext;
  class RequirementUsageContext;
  class ConstraintUsageContext;
  class PortUsageContext;
  class ConnectorUsageContext;
  class FlowUsageContext;
  class ConnectionUsageContext;
  class AllocationUsageContext;
  class ItemUsageContext;
  class ViewUsageContext;
  class PartBodyContext;
  class ActionBodyContext;
  class ActionBodyItemContext;
  class StateBodyOldContext;
  class ExhibitStatementContext;
  class StateMachineKindContext;
  class StateBodyContext;
  class StateDefinitionBodyContext;
  class StateBodyItemContext;
  class ConstraintStatementContext;
  class StateMemberContext;
  class TransitionMemberContext;
  class RefStatementContext;
  class StateActionMemberContext;
  class EntryActionMemberContext;
  class DoActionMemberContext;
  class ExitActionMemberContext;
  class StateActionUsageContext;
  class RequirementBodyContext;
  class ViewpointBodyContext;
  class ViewpointBodyItemContext;
  class FrameConcernMemberContext;
  class ViewDefinitionBodyContext;
  class ViewDefinitionBodyItemContext;
  class ElementFilterMemberContext;
  class FilterExpressionContext;
  class MetadataAccessExpressionContext;
  class ViewRenderingMemberContext;
  class ViewRenderingUsageContext;
  class ViewBodyContext;
  class ViewBodyItemContext;
  class ExposeStatementContext;
  class MembershipExposeContext;
  class NamespaceExposeContext;
  class ConcernBodyContext;
  class ConcernBodyItemContext;
  class RequirementMemberContext;
  class RequirementConstraintMemberContext;
  class PortBodyContext;
  class ConnectorBodyContext;
  class FlowBodyContext;
  class InterfaceBodyContext;
  class DatatypeBodyContext;
  class ClassBodyContext;
  class EnumBodyContext;
  class AnnotatingMemberContext;
  class DocStatementContext;
  class CommentStatementContext;
  class UsageContext;
  class StatementContext;
  class BindStatementContext;
  class ConnectStatementContext;
  class AllocateStatementContext;
  class AllocationPartContext;
  class BinaryAllocationPartContext;
  class NaryAllocationPartContext;
  class EndStatementContext;
  class FlowDefinitionContext;
  class FlowStatementContext;
  class FlowDeclarationContext;
  class FlowEndMemberContext;
  class PayloadFeatureMemberContext;
  class PerformStatementContext;
  class AssertStatementContext;
  class RequireStatementContext;
  class SatisfyStatementContext;
  class RequirementBodyItemContext;
  class AssumeStatementContext;
  class RedefinesStatementContext;
  class ActionInputOutputContext;
  class ConnectorPartContext;
  class BinaryConnectorPartContext;
  class NaryConnectorPartContext;
  class ConnectorEndMemberContext;
  class OwnedReferenceSubsettingContext;
  class ReferencesKeywordContext;
  class OwnedMultiplicityContext;
  class TransitionContext;
  class TransitionBodyContext;
  class SimpleTransitionBodyContext;
  class ComplexTransitionBodyContext;
  class TransitionTriggerPartContext;
  class AcceptClauseContext;
  class AcceptItemContext;
  class AcceptExpressionContext;
  class TransitionGuardPartContext;
  class TransitionEffectPartContext;
  class TransitionActionContext;
  class SendStatementContext;
  class SendActionPartContext;
  class ActionReferenceContext;
  class ActionInvocationContext;
  class ActionArgumentContext;
  class StateReferenceContext;
  class TriggerExpressionContext;
  class GuardExpressionContext;
  class EffectExpressionContext;
  class EnumValueContext;
  class AttributeContext;
  class FeaturePrefixContext;
  class AttributeContentContext;
  class AttributeExpressionsContext;
  class SubsettingExpressionContext;
  class RedefinitionExpressionContext;
  class TypingExpressionContext;
  class SubclassificationExpressionContext;
  class AttributeBodyContext;
  class ImportStatementContext;
  class MembershipImportContext;
  class ImportedMembershipContext;
  class NamespaceImportContext;
  class ImportedNamespaceContext;
  class FilterPackageContext;
  class ImportPrefixContext;
  class RelationshipBodyContext;
  class RelationshipBodyItemContext;
  class RelationshipContext;
  class SubclassificationContext;
  class SubsettingContext;
  class RedefinitionContext;
  class ConjugationContext;
  class DependencyContext;
  class TypeExpressionContext;
  class FeatureExpressionContext;
  class ElementExpressionContext;
  class QualifiedNameContext;
  class MemberAccessExpressionContext;
  class IdentifierContext;
  class BlockCommentContext; 

  class  FileContext : public antlr4::ParserRuleContext {
  public:
    FileContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *EOF();
    std::vector<PackageContext *> package();
    PackageContext* package(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  FileContext* file();

  class  PackageContext : public antlr4::ParserRuleContext {
  public:
    PackageContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    QualifiedNameContext *qualifiedName();
    PackageBodyContext *packageBody();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  PackageContext* package();

  class  PackageBodyContext : public antlr4::ParserRuleContext {
  public:
    PackageBodyContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<PackageContext *> package();
    PackageContext* package(size_t i);
    std::vector<DefinitionContext *> definition();
    DefinitionContext* definition(size_t i);
    std::vector<UsageContext *> usage();
    UsageContext* usage(size_t i);
    std::vector<ImportStatementContext *> importStatement();
    ImportStatementContext* importStatement(size_t i);
    std::vector<RelationshipContext *> relationship();
    RelationshipContext* relationship(size_t i);
    std::vector<DependencyContext *> dependency();
    DependencyContext* dependency(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  PackageBodyContext* packageBody();

  class  BasicDefinitionPrefixContext : public antlr4::ParserRuleContext {
  public:
    BasicDefinitionPrefixContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  BasicDefinitionPrefixContext* basicDefinitionPrefix();

  class  DefinitionExtensionKeywordContext : public antlr4::ParserRuleContext {
  public:
    DefinitionExtensionKeywordContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    QualifiedNameContext *qualifiedName();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  DefinitionExtensionKeywordContext* definitionExtensionKeyword();

  class  DefinitionPrefixContext : public antlr4::ParserRuleContext {
  public:
    DefinitionPrefixContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    BasicDefinitionPrefixContext *basicDefinitionPrefix();
    std::vector<DefinitionExtensionKeywordContext *> definitionExtensionKeyword();
    DefinitionExtensionKeywordContext* definitionExtensionKeyword(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  DefinitionPrefixContext* definitionPrefix();

  class  OccurrenceDefinitionPrefixContext : public antlr4::ParserRuleContext {
  public:
    OccurrenceDefinitionPrefixContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    BasicDefinitionPrefixContext *basicDefinitionPrefix();
    EmptyMultiplicityMemberContext *emptyMultiplicityMember();
    std::vector<DefinitionExtensionKeywordContext *> definitionExtensionKeyword();
    DefinitionExtensionKeywordContext* definitionExtensionKeyword(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  OccurrenceDefinitionPrefixContext* occurrenceDefinitionPrefix();

  class  EmptyMultiplicityMemberContext : public antlr4::ParserRuleContext {
  public:
    EmptyMultiplicityMemberContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  EmptyMultiplicityMemberContext* emptyMultiplicityMember();

  class  FeatureDirectionContext : public antlr4::ParserRuleContext {
  public:
    FeatureDirectionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  FeatureDirectionContext* featureDirection();

  class  RefPrefixContext : public antlr4::ParserRuleContext {
  public:
    RefPrefixContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    FeatureDirectionContext *featureDirection();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  RefPrefixContext* refPrefix();

  class  BasicUsagePrefixContext : public antlr4::ParserRuleContext {
  public:
    BasicUsagePrefixContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    RefPrefixContext *refPrefix();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  BasicUsagePrefixContext* basicUsagePrefix();

  class  EndUsagePrefixContext : public antlr4::ParserRuleContext {
  public:
    EndUsagePrefixContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    OwnedCrossFeatureMemberContext *ownedCrossFeatureMember();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  EndUsagePrefixContext* endUsagePrefix();

  class  OwnedCrossFeatureMemberContext : public antlr4::ParserRuleContext {
  public:
    OwnedCrossFeatureMemberContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  OwnedCrossFeatureMemberContext* ownedCrossFeatureMember();

  class  UsageExtensionKeywordContext : public antlr4::ParserRuleContext {
  public:
    UsageExtensionKeywordContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    QualifiedNameContext *qualifiedName();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  UsageExtensionKeywordContext* usageExtensionKeyword();

  class  UnextendedUsagePrefixContext : public antlr4::ParserRuleContext {
  public:
    UnextendedUsagePrefixContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    EndUsagePrefixContext *endUsagePrefix();
    BasicUsagePrefixContext *basicUsagePrefix();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  UnextendedUsagePrefixContext* unextendedUsagePrefix();

  class  UsagePrefixContext : public antlr4::ParserRuleContext {
  public:
    UsagePrefixContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    UnextendedUsagePrefixContext *unextendedUsagePrefix();
    std::vector<UsageExtensionKeywordContext *> usageExtensionKeyword();
    UsageExtensionKeywordContext* usageExtensionKeyword(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  UsagePrefixContext* usagePrefix();

  class  OccurrenceUsagePrefixContext : public antlr4::ParserRuleContext {
  public:
    OccurrenceUsagePrefixContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    EndUsagePrefixContext *endUsagePrefix();
    BasicUsagePrefixContext *basicUsagePrefix();
    std::vector<UsageExtensionKeywordContext *> usageExtensionKeyword();
    UsageExtensionKeywordContext* usageExtensionKeyword(size_t i);
    PortionKindContext *portionKind();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  OccurrenceUsagePrefixContext* occurrenceUsagePrefix();

  class  PortionKindContext : public antlr4::ParserRuleContext {
  public:
    PortionKindContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  PortionKindContext* portionKind();

  class  IdentificationContext : public antlr4::ParserRuleContext {
  public:
    IdentificationContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<IdentifierContext *> identifier();
    IdentifierContext* identifier(size_t i);
    antlr4::tree::TerminalNode *String();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  IdentificationContext* identification();

  class  NameContext : public antlr4::ParserRuleContext {
  public:
    NameContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    IdentifierContext *identifier();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  NameContext* name();

  class  DefinitionDeclarationContext : public antlr4::ParserRuleContext {
  public:
    DefinitionDeclarationContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    IdentificationContext *identification();
    SubclassificationPartContext *subclassificationPart();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  DefinitionDeclarationContext* definitionDeclaration();

  class  SubclassificationPartContext : public antlr4::ParserRuleContext {
  public:
    SubclassificationPartContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<OwnedSubclassificationContext *> ownedSubclassification();
    OwnedSubclassificationContext* ownedSubclassification(size_t i);
    antlr4::tree::TerminalNode *SUBSETS_OP();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  SubclassificationPartContext* subclassificationPart();

  class  OwnedSubclassificationContext : public antlr4::ParserRuleContext {
  public:
    OwnedSubclassificationContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    QualifiedNameContext *qualifiedName();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  OwnedSubclassificationContext* ownedSubclassification();

  class  DefinitionBodyContext : public antlr4::ParserRuleContext {
  public:
    DefinitionBodyContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<DefinitionBodyItemContext *> definitionBodyItem();
    DefinitionBodyItemContext* definitionBodyItem(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  DefinitionBodyContext* definitionBody();

  class  DefinitionBodyItemContext : public antlr4::ParserRuleContext {
  public:
    DefinitionBodyItemContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    ActionInputOutputContext *actionInputOutput();
    AttributeContext *attribute();
    SimplifiedAttributeContext *simplifiedAttribute();
    DefinitionMemberContext *definitionMember();
    UsageMemberContext *usageMember();
    ImportStatementContext *importStatement();
    RelationshipContext *relationship();
    StatementContext *statement();
    DocStatementContext *docStatement();
    CommentStatementContext *commentStatement();
    RequirementMemberContext *requirementMember();
    RequirementConstraintMemberContext *requirementConstraintMember();
    MetadataAnnotationContext *metadataAnnotation();
    ExhibitStatementContext *exhibitStatement();
    RefStatementContext *refStatement();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  DefinitionBodyItemContext* definitionBodyItem();

  class  MetadataAnnotationContext : public antlr4::ParserRuleContext {
  public:
    MetadataAnnotationContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    IdentifierContext *identifier();
    std::vector<DefinitionBodyItemContext *> definitionBodyItem();
    DefinitionBodyItemContext* definitionBodyItem(size_t i);
    std::vector<MetadataAssignmentContext *> metadataAssignment();
    MetadataAssignmentContext* metadataAssignment(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  MetadataAnnotationContext* metadataAnnotation();

  class  MetadataAssignmentContext : public antlr4::ParserRuleContext {
  public:
    MetadataAssignmentContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    IdentifierContext *identifier();
    ExpressionContext *expression();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  MetadataAssignmentContext* metadataAssignment();

  class  SimplifiedAttributeContext : public antlr4::ParserRuleContext {
  public:
    SimplifiedAttributeContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    IdentifierContext *identifier();
    antlr4::tree::TerminalNode *COLON();
    QualifiedNameContext *qualifiedName();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  SimplifiedAttributeContext* simplifiedAttribute();

  class  DefinitionMemberContext : public antlr4::ParserRuleContext {
  public:
    DefinitionMemberContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    DefinitionContext *definition();
    VisibilityIndicatorContext *visibilityIndicator();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  DefinitionMemberContext* definitionMember();

  class  UsageMemberContext : public antlr4::ParserRuleContext {
  public:
    UsageMemberContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    UsageContext *usage();
    VisibilityIndicatorContext *visibilityIndicator();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  UsageMemberContext* usageMember();

  class  VisibilityIndicatorContext : public antlr4::ParserRuleContext {
  public:
    VisibilityIndicatorContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  VisibilityIndicatorContext* visibilityIndicator();

  class  UsageDeclarationContext : public antlr4::ParserRuleContext {
  public:
    UsageDeclarationContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    FeatureDeclarationContext *featureDeclaration();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  UsageDeclarationContext* usageDeclaration();

  class  FeatureDeclarationContext : public antlr4::ParserRuleContext {
  public:
    FeatureDeclarationContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    IdentificationContext *identification();
    FeatureSpecializationPartContext *featureSpecializationPart();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  FeatureDeclarationContext* featureDeclaration();

  class  FeatureSpecializationPartContext : public antlr4::ParserRuleContext {
  public:
    FeatureSpecializationPartContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    TypingExpressionContext *typingExpression();
    SubsettingExpressionContext *subsettingExpression();
    RedefinitionExpressionContext *redefinitionExpression();
    AttributeExpressionsContext *attributeExpressions();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  FeatureSpecializationPartContext* featureSpecializationPart();

  class  UsageCompletionContext : public antlr4::ParserRuleContext {
  public:
    UsageCompletionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    UsageBodyContext *usageBody();
    ValuePartContext *valuePart();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  UsageCompletionContext* usageCompletion();

  class  UsageBodyContext : public antlr4::ParserRuleContext {
  public:
    UsageBodyContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    StateBodyContext *stateBody();
    DefinitionBodyContext *definitionBody();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  UsageBodyContext* usageBody();

  class  ValuePartContext : public antlr4::ParserRuleContext {
  public:
    ValuePartContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    FeatureValueContext *featureValue();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ValuePartContext* valuePart();

  class  FeatureValueContext : public antlr4::ParserRuleContext {
  public:
    FeatureValueContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    ExpressionContext *expression();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  FeatureValueContext* featureValue();

  class  ExpressionContext : public antlr4::ParserRuleContext {
  public:
    ExpressionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    LogicalOrExpressionContext *logicalOrExpression();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ExpressionContext* expression();

  class  LogicalOrExpressionContext : public antlr4::ParserRuleContext {
  public:
    LogicalOrExpressionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<LogicalAndExpressionContext *> logicalAndExpression();
    LogicalAndExpressionContext* logicalAndExpression(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  LogicalOrExpressionContext* logicalOrExpression();

  class  LogicalAndExpressionContext : public antlr4::ParserRuleContext {
  public:
    LogicalAndExpressionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<ComparisonExpressionContext *> comparisonExpression();
    ComparisonExpressionContext* comparisonExpression(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  LogicalAndExpressionContext* logicalAndExpression();

  class  ComparisonExpressionContext : public antlr4::ParserRuleContext {
  public:
    ComparisonExpressionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<AdditiveExpressionContext *> additiveExpression();
    AdditiveExpressionContext* additiveExpression(size_t i);
    ComparisonOpContext *comparisonOp();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ComparisonExpressionContext* comparisonExpression();

  class  ComparisonOpContext : public antlr4::ParserRuleContext {
  public:
    ComparisonOpContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ComparisonOpContext* comparisonOp();

  class  AdditiveExpressionContext : public antlr4::ParserRuleContext {
  public:
    AdditiveExpressionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<MultiplicativeExpressionContext *> multiplicativeExpression();
    MultiplicativeExpressionContext* multiplicativeExpression(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  AdditiveExpressionContext* additiveExpression();

  class  MultiplicativeExpressionContext : public antlr4::ParserRuleContext {
  public:
    MultiplicativeExpressionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<UnaryExpressionContext *> unaryExpression();
    UnaryExpressionContext* unaryExpression(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  MultiplicativeExpressionContext* multiplicativeExpression();

  class  UnaryExpressionContext : public antlr4::ParserRuleContext {
  public:
    UnaryExpressionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    PrimaryExpressionContext *primaryExpression();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  UnaryExpressionContext* unaryExpression();

  class  PrimaryExpressionContext : public antlr4::ParserRuleContext {
  public:
    PrimaryExpressionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    ActionInvocationContext *actionInvocation();
    LiteralExpressionContext *literalExpression();
    FeatureReferenceExpressionContext *featureReferenceExpression();
    ExpressionContext *expression();
    UnitExpressionContext *unitExpression();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  PrimaryExpressionContext* primaryExpression();

  class  UnitExpressionContext : public antlr4::ParserRuleContext {
  public:
    UnitExpressionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<UnitTermContext *> unitTerm();
    UnitTermContext* unitTerm(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  UnitExpressionContext* unitExpression();

  class  UnitTermContext : public antlr4::ParserRuleContext {
  public:
    UnitTermContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *IDENTIFIER();
    antlr4::tree::TerminalNode *Integer();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  UnitTermContext* unitTerm();

  class  LiteralExpressionContext : public antlr4::ParserRuleContext {
  public:
    LiteralExpressionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *Integer();
    antlr4::tree::TerminalNode *Real();
    antlr4::tree::TerminalNode *Boolean();
    antlr4::tree::TerminalNode *String();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  LiteralExpressionContext* literalExpression();

  class  FeatureReferenceExpressionContext : public antlr4::ParserRuleContext {
  public:
    FeatureReferenceExpressionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    QualifiedNameContext *qualifiedName();
    MemberAccessExpressionContext *memberAccessExpression();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  FeatureReferenceExpressionContext* featureReferenceExpression();

  class  DefinitionContext : public antlr4::ParserRuleContext {
  public:
    DefinitionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    PartDefinitionContext *partDefinition();
    CalculationDefinitionContext *calculationDefinition();
    ActionDefinitionContext *actionDefinition();
    StateDefinitionContext *stateDefinition();
    RequirementDefinitionContext *requirementDefinition();
    ConstraintDefinitionContext *constraintDefinition();
    PortDefinitionContext *portDefinition();
    ConnectorDefinitionContext *connectorDefinition();
    InterfaceDefinitionContext *interfaceDefinition();
    DatatypeDefinitionContext *datatypeDefinition();
    ClassDefinitionContext *classDefinition();
    EnumDefinitionContext *enumDefinition();
    AttributeDefinitionContext *attributeDefinition();
    ConnectionDefinitionContext *connectionDefinition();
    AllocationDefinitionContext *allocationDefinition();
    ItemDefinitionContext *itemDefinition();
    FlowDefinitionContext *flowDefinition();
    ViewpointDefinitionContext *viewpointDefinition();
    ViewDefinitionContext *viewDefinition();
    ConcernDefinitionContext *concernDefinition();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  DefinitionContext* definition();

  class  PartDefinitionContext : public antlr4::ParserRuleContext {
  public:
    PartDefinitionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    OccurrenceDefinitionPrefixContext *occurrenceDefinitionPrefix();
    DefinitionDeclarationContext *definitionDeclaration();
    DefinitionBodyContext *definitionBody();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  PartDefinitionContext* partDefinition();

  class  ActionDefinitionContext : public antlr4::ParserRuleContext {
  public:
    ActionDefinitionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    OccurrenceDefinitionPrefixContext *occurrenceDefinitionPrefix();
    DefinitionDeclarationContext *definitionDeclaration();
    DefinitionBodyContext *definitionBody();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ActionDefinitionContext* actionDefinition();

  class  CalculationDefinitionContext : public antlr4::ParserRuleContext {
  public:
    CalculationDefinitionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    OccurrenceDefinitionPrefixContext *occurrenceDefinitionPrefix();
    DefinitionDeclarationContext *definitionDeclaration();
    CalculationBodyContext *calculationBody();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  CalculationDefinitionContext* calculationDefinition();

  class  CalculationBodyContext : public antlr4::ParserRuleContext {
  public:
    CalculationBodyContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<CalculationBodyItemContext *> calculationBodyItem();
    CalculationBodyItemContext* calculationBodyItem(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  CalculationBodyContext* calculationBody();

  class  CalculationBodyItemContext : public antlr4::ParserRuleContext {
  public:
    CalculationBodyItemContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    ReturnParameterContext *returnParameter();
    DefinitionBodyItemContext *definitionBodyItem();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  CalculationBodyItemContext* calculationBodyItem();

  class  ReturnParameterContext : public antlr4::ParserRuleContext {
  public:
    ReturnParameterContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    UsageDeclarationContext *usageDeclaration();
    ValuePartContext *valuePart();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ReturnParameterContext* returnParameter();

  class  StateDefinitionContext : public antlr4::ParserRuleContext {
  public:
    StateDefinitionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    OccurrenceDefinitionPrefixContext *occurrenceDefinitionPrefix();
    DefinitionDeclarationContext *definitionDeclaration();
    StateDefinitionBodyContext *stateDefinitionBody();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  StateDefinitionContext* stateDefinition();

  class  RequirementDefinitionContext : public antlr4::ParserRuleContext {
  public:
    RequirementDefinitionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    OccurrenceDefinitionPrefixContext *occurrenceDefinitionPrefix();
    DefinitionDeclarationContext *definitionDeclaration();
    DefinitionBodyContext *definitionBody();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  RequirementDefinitionContext* requirementDefinition();

  class  ConstraintDefinitionContext : public antlr4::ParserRuleContext {
  public:
    ConstraintDefinitionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    OccurrenceDefinitionPrefixContext *occurrenceDefinitionPrefix();
    DefinitionDeclarationContext *definitionDeclaration();
    DefinitionBodyContext *definitionBody();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ConstraintDefinitionContext* constraintDefinition();

  class  PortDefinitionContext : public antlr4::ParserRuleContext {
  public:
    PortDefinitionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    DefinitionPrefixContext *definitionPrefix();
    DefinitionDeclarationContext *definitionDeclaration();
    DefinitionBodyContext *definitionBody();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  PortDefinitionContext* portDefinition();

  class  ConnectorDefinitionContext : public antlr4::ParserRuleContext {
  public:
    ConnectorDefinitionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    DefinitionPrefixContext *definitionPrefix();
    DefinitionDeclarationContext *definitionDeclaration();
    DefinitionBodyContext *definitionBody();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ConnectorDefinitionContext* connectorDefinition();

  class  InterfaceDefinitionContext : public antlr4::ParserRuleContext {
  public:
    InterfaceDefinitionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    OccurrenceDefinitionPrefixContext *occurrenceDefinitionPrefix();
    DefinitionDeclarationContext *definitionDeclaration();
    DefinitionBodyContext *definitionBody();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  InterfaceDefinitionContext* interfaceDefinition();

  class  DatatypeDefinitionContext : public antlr4::ParserRuleContext {
  public:
    DatatypeDefinitionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    DefinitionPrefixContext *definitionPrefix();
    DefinitionDeclarationContext *definitionDeclaration();
    DefinitionBodyContext *definitionBody();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  DatatypeDefinitionContext* datatypeDefinition();

  class  ClassDefinitionContext : public antlr4::ParserRuleContext {
  public:
    ClassDefinitionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    DefinitionPrefixContext *definitionPrefix();
    DefinitionDeclarationContext *definitionDeclaration();
    DefinitionBodyContext *definitionBody();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ClassDefinitionContext* classDefinition();

  class  EnumDefinitionContext : public antlr4::ParserRuleContext {
  public:
    EnumDefinitionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    DefinitionDeclarationContext *definitionDeclaration();
    EnumBodyContext *enumBody();
    std::vector<DefinitionExtensionKeywordContext *> definitionExtensionKeyword();
    DefinitionExtensionKeywordContext* definitionExtensionKeyword(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  EnumDefinitionContext* enumDefinition();

  class  AttributeDefinitionContext : public antlr4::ParserRuleContext {
  public:
    AttributeDefinitionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    DefinitionPrefixContext *definitionPrefix();
    DefinitionDeclarationContext *definitionDeclaration();
    DefinitionBodyContext *definitionBody();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  AttributeDefinitionContext* attributeDefinition();

  class  ConnectionDefinitionContext : public antlr4::ParserRuleContext {
  public:
    ConnectionDefinitionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    OccurrenceDefinitionPrefixContext *occurrenceDefinitionPrefix();
    DefinitionDeclarationContext *definitionDeclaration();
    DefinitionBodyContext *definitionBody();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ConnectionDefinitionContext* connectionDefinition();

  class  AllocationDefinitionContext : public antlr4::ParserRuleContext {
  public:
    AllocationDefinitionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    OccurrenceDefinitionPrefixContext *occurrenceDefinitionPrefix();
    DefinitionDeclarationContext *definitionDeclaration();
    DefinitionBodyContext *definitionBody();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  AllocationDefinitionContext* allocationDefinition();

  class  ItemDefinitionContext : public antlr4::ParserRuleContext {
  public:
    ItemDefinitionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    OccurrenceDefinitionPrefixContext *occurrenceDefinitionPrefix();
    DefinitionDeclarationContext *definitionDeclaration();
    DefinitionBodyContext *definitionBody();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ItemDefinitionContext* itemDefinition();

  class  ViewpointDefinitionContext : public antlr4::ParserRuleContext {
  public:
    ViewpointDefinitionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    OccurrenceDefinitionPrefixContext *occurrenceDefinitionPrefix();
    DefinitionDeclarationContext *definitionDeclaration();
    ViewpointBodyContext *viewpointBody();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ViewpointDefinitionContext* viewpointDefinition();

  class  ViewDefinitionContext : public antlr4::ParserRuleContext {
  public:
    ViewDefinitionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    OccurrenceDefinitionPrefixContext *occurrenceDefinitionPrefix();
    DefinitionDeclarationContext *definitionDeclaration();
    ViewDefinitionBodyContext *viewDefinitionBody();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ViewDefinitionContext* viewDefinition();

  class  ConcernDefinitionContext : public antlr4::ParserRuleContext {
  public:
    ConcernDefinitionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    OccurrenceDefinitionPrefixContext *occurrenceDefinitionPrefix();
    DefinitionDeclarationContext *definitionDeclaration();
    ConcernBodyContext *concernBody();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ConcernDefinitionContext* concernDefinition();

  class  PartUsageContext : public antlr4::ParserRuleContext {
  public:
    PartUsageContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    OccurrenceUsagePrefixContext *occurrenceUsagePrefix();
    UsageCompletionContext *usageCompletion();
    UsageDeclarationContext *usageDeclaration();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  PartUsageContext* partUsage();

  class  ActionUsageContext : public antlr4::ParserRuleContext {
  public:
    ActionUsageContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    OccurrenceUsagePrefixContext *occurrenceUsagePrefix();
    UsageCompletionContext *usageCompletion();
    UsageDeclarationContext *usageDeclaration();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ActionUsageContext* actionUsage();

  class  AnalysisUsageContext : public antlr4::ParserRuleContext {
  public:
    AnalysisUsageContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    OccurrenceUsagePrefixContext *occurrenceUsagePrefix();
    AnalysisBodyContext *analysisBody();
    UsageDeclarationContext *usageDeclaration();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  AnalysisUsageContext* analysisUsage();

  class  AnalysisBodyContext : public antlr4::ParserRuleContext {
  public:
    AnalysisBodyContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<AnalysisBodyItemContext *> analysisBodyItem();
    AnalysisBodyItemContext* analysisBodyItem(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  AnalysisBodyContext* analysisBody();

  class  AnalysisBodyItemContext : public antlr4::ParserRuleContext {
  public:
    AnalysisBodyItemContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    SubjectAssignmentContext *subjectAssignment();
    DefinitionBodyItemContext *definitionBodyItem();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  AnalysisBodyItemContext* analysisBodyItem();

  class  SubjectAssignmentContext : public antlr4::ParserRuleContext {
  public:
    SubjectAssignmentContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    ExpressionContext *expression();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  SubjectAssignmentContext* subjectAssignment();

  class  RenderingUsageContext : public antlr4::ParserRuleContext {
  public:
    RenderingUsageContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    OccurrenceUsagePrefixContext *occurrenceUsagePrefix();
    DefinitionBodyContext *definitionBody();
    UsageDeclarationContext *usageDeclaration();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  RenderingUsageContext* renderingUsage();

  class  StateUsageContext : public antlr4::ParserRuleContext {
  public:
    StateUsageContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    OccurrenceUsagePrefixContext *occurrenceUsagePrefix();
    UsageCompletionContext *usageCompletion();
    UsageDeclarationContext *usageDeclaration();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  StateUsageContext* stateUsage();

  class  RequirementUsageContext : public antlr4::ParserRuleContext {
  public:
    RequirementUsageContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    OccurrenceUsagePrefixContext *occurrenceUsagePrefix();
    UsageCompletionContext *usageCompletion();
    UsageDeclarationContext *usageDeclaration();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  RequirementUsageContext* requirementUsage();

  class  ConstraintUsageContext : public antlr4::ParserRuleContext {
  public:
    ConstraintUsageContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    OccurrenceUsagePrefixContext *occurrenceUsagePrefix();
    UsageCompletionContext *usageCompletion();
    UsageDeclarationContext *usageDeclaration();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ConstraintUsageContext* constraintUsage();

  class  PortUsageContext : public antlr4::ParserRuleContext {
  public:
    PortUsageContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    OccurrenceUsagePrefixContext *occurrenceUsagePrefix();
    UsageCompletionContext *usageCompletion();
    UsageDeclarationContext *usageDeclaration();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  PortUsageContext* portUsage();

  class  ConnectorUsageContext : public antlr4::ParserRuleContext {
  public:
    ConnectorUsageContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    UsagePrefixContext *usagePrefix();
    UsageCompletionContext *usageCompletion();
    UsageDeclarationContext *usageDeclaration();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ConnectorUsageContext* connectorUsage();

  class  FlowUsageContext : public antlr4::ParserRuleContext {
  public:
    FlowUsageContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    OccurrenceUsagePrefixContext *occurrenceUsagePrefix();
    UsageCompletionContext *usageCompletion();
    UsageDeclarationContext *usageDeclaration();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  FlowUsageContext* flowUsage();

  class  ConnectionUsageContext : public antlr4::ParserRuleContext {
  public:
    ConnectionUsageContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    OccurrenceUsagePrefixContext *occurrenceUsagePrefix();
    UsageCompletionContext *usageCompletion();
    UsageDeclarationContext *usageDeclaration();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ConnectionUsageContext* connectionUsage();

  class  AllocationUsageContext : public antlr4::ParserRuleContext {
  public:
    AllocationUsageContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    OccurrenceUsagePrefixContext *occurrenceUsagePrefix();
    UsageCompletionContext *usageCompletion();
    UsageDeclarationContext *usageDeclaration();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  AllocationUsageContext* allocationUsage();

  class  ItemUsageContext : public antlr4::ParserRuleContext {
  public:
    ItemUsageContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    OccurrenceUsagePrefixContext *occurrenceUsagePrefix();
    UsageCompletionContext *usageCompletion();
    UsageDeclarationContext *usageDeclaration();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ItemUsageContext* itemUsage();

  class  ViewUsageContext : public antlr4::ParserRuleContext {
  public:
    ViewUsageContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    OccurrenceUsagePrefixContext *occurrenceUsagePrefix();
    ViewBodyContext *viewBody();
    UsageDeclarationContext *usageDeclaration();
    ValuePartContext *valuePart();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ViewUsageContext* viewUsage();

  class  PartBodyContext : public antlr4::ParserRuleContext {
  public:
    PartBodyContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<DefinitionContext *> definition();
    DefinitionContext* definition(size_t i);
    std::vector<UsageContext *> usage();
    UsageContext* usage(size_t i);
    std::vector<AttributeContext *> attribute();
    AttributeContext* attribute(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  PartBodyContext* partBody();

  class  ActionBodyContext : public antlr4::ParserRuleContext {
  public:
    ActionBodyContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<ActionBodyItemContext *> actionBodyItem();
    ActionBodyItemContext* actionBodyItem(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ActionBodyContext* actionBody();

  class  ActionBodyItemContext : public antlr4::ParserRuleContext {
  public:
    ActionBodyItemContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    DefinitionBodyItemContext *definitionBodyItem();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ActionBodyItemContext* actionBodyItem();

  class  StateBodyOldContext : public antlr4::ParserRuleContext {
  public:
    StateBodyOldContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<DefinitionContext *> definition();
    DefinitionContext* definition(size_t i);
    std::vector<UsageContext *> usage();
    UsageContext* usage(size_t i);
    std::vector<AttributeContext *> attribute();
    AttributeContext* attribute(size_t i);
    std::vector<TransitionContext *> transition();
    TransitionContext* transition(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  StateBodyOldContext* stateBodyOld();

  class  ExhibitStatementContext : public antlr4::ParserRuleContext {
  public:
    ExhibitStatementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    StateBodyContext *stateBody();
    UsageDeclarationContext *usageDeclaration();
    StateMachineKindContext *stateMachineKind();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ExhibitStatementContext* exhibitStatement();

  class  StateMachineKindContext : public antlr4::ParserRuleContext {
  public:
    StateMachineKindContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  StateMachineKindContext* stateMachineKind();

  class  StateBodyContext : public antlr4::ParserRuleContext {
  public:
    StateBodyContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<StateBodyItemContext *> stateBodyItem();
    StateBodyItemContext* stateBodyItem(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  StateBodyContext* stateBody();

  class  StateDefinitionBodyContext : public antlr4::ParserRuleContext {
  public:
    StateDefinitionBodyContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<StateBodyItemContext *> stateBodyItem();
    StateBodyItemContext* stateBodyItem(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  StateDefinitionBodyContext* stateDefinitionBody();

  class  StateBodyItemContext : public antlr4::ParserRuleContext {
  public:
    StateBodyItemContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    StateMemberContext *stateMember();
    TransitionMemberContext *transitionMember();
    StateActionMemberContext *stateActionMember();
    RefStatementContext *refStatement();
    ConstraintStatementContext *constraintStatement();
    DefinitionBodyItemContext *definitionBodyItem();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  StateBodyItemContext* stateBodyItem();

  class  ConstraintStatementContext : public antlr4::ParserRuleContext {
  public:
    ConstraintStatementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    ExpressionContext *expression();
    IdentifierContext *identifier();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ConstraintStatementContext* constraintStatement();

  class  StateMemberContext : public antlr4::ParserRuleContext {
  public:
    StateMemberContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    StateUsageContext *stateUsage();
    StateDefinitionContext *stateDefinition();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  StateMemberContext* stateMember();

  class  TransitionMemberContext : public antlr4::ParserRuleContext {
  public:
    TransitionMemberContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    TransitionContext *transition();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  TransitionMemberContext* transitionMember();

  class  RefStatementContext : public antlr4::ParserRuleContext {
  public:
    RefStatementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    UsageCompletionContext *usageCompletion();
    UsageDeclarationContext *usageDeclaration();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  RefStatementContext* refStatement();

  class  StateActionMemberContext : public antlr4::ParserRuleContext {
  public:
    StateActionMemberContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    EntryActionMemberContext *entryActionMember();
    DoActionMemberContext *doActionMember();
    ExitActionMemberContext *exitActionMember();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  StateActionMemberContext* stateActionMember();

  class  EntryActionMemberContext : public antlr4::ParserRuleContext {
  public:
    EntryActionMemberContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    StateActionUsageContext *stateActionUsage();
    VisibilityIndicatorContext *visibilityIndicator();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  EntryActionMemberContext* entryActionMember();

  class  DoActionMemberContext : public antlr4::ParserRuleContext {
  public:
    DoActionMemberContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    StateActionUsageContext *stateActionUsage();
    VisibilityIndicatorContext *visibilityIndicator();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  DoActionMemberContext* doActionMember();

  class  ExitActionMemberContext : public antlr4::ParserRuleContext {
  public:
    ExitActionMemberContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    StateActionUsageContext *stateActionUsage();
    VisibilityIndicatorContext *visibilityIndicator();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ExitActionMemberContext* exitActionMember();

  class  StateActionUsageContext : public antlr4::ParserRuleContext {
  public:
    StateActionUsageContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    IdentifierContext *identifier();
    OwnedReferenceSubsettingContext *ownedReferenceSubsetting();
    ActionBodyContext *actionBody();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  StateActionUsageContext* stateActionUsage();

  class  RequirementBodyContext : public antlr4::ParserRuleContext {
  public:
    RequirementBodyContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    DefinitionBodyContext *definitionBody();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  RequirementBodyContext* requirementBody();

  class  ViewpointBodyContext : public antlr4::ParserRuleContext {
  public:
    ViewpointBodyContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<ViewpointBodyItemContext *> viewpointBodyItem();
    ViewpointBodyItemContext* viewpointBodyItem(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ViewpointBodyContext* viewpointBody();

  class  ViewpointBodyItemContext : public antlr4::ParserRuleContext {
  public:
    ViewpointBodyItemContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    DefinitionBodyItemContext *definitionBodyItem();
    RequirementMemberContext *requirementMember();
    RequirementConstraintMemberContext *requirementConstraintMember();
    FrameConcernMemberContext *frameConcernMember();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ViewpointBodyItemContext* viewpointBodyItem();

  class  FrameConcernMemberContext : public antlr4::ParserRuleContext {
  public:
    FrameConcernMemberContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    VisibilityIndicatorContext *visibilityIndicator();
    UsageDeclarationContext *usageDeclaration();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  FrameConcernMemberContext* frameConcernMember();

  class  ViewDefinitionBodyContext : public antlr4::ParserRuleContext {
  public:
    ViewDefinitionBodyContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<ViewDefinitionBodyItemContext *> viewDefinitionBodyItem();
    ViewDefinitionBodyItemContext* viewDefinitionBodyItem(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ViewDefinitionBodyContext* viewDefinitionBody();

  class  ViewDefinitionBodyItemContext : public antlr4::ParserRuleContext {
  public:
    ViewDefinitionBodyItemContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    DefinitionBodyItemContext *definitionBodyItem();
    ElementFilterMemberContext *elementFilterMember();
    ViewRenderingMemberContext *viewRenderingMember();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ViewDefinitionBodyItemContext* viewDefinitionBodyItem();

  class  ElementFilterMemberContext : public antlr4::ParserRuleContext {
  public:
    ElementFilterMemberContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    FilterExpressionContext *filterExpression();
    VisibilityIndicatorContext *visibilityIndicator();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ElementFilterMemberContext* elementFilterMember();

  class  FilterExpressionContext : public antlr4::ParserRuleContext {
  public:
    FilterExpressionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    MetadataAccessExpressionContext *metadataAccessExpression();
    std::vector<FilterExpressionContext *> filterExpression();
    FilterExpressionContext* filterExpression(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  FilterExpressionContext* filterExpression();
  FilterExpressionContext* filterExpression(int precedence);
  class  MetadataAccessExpressionContext : public antlr4::ParserRuleContext {
  public:
    MetadataAccessExpressionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    QualifiedNameContext *qualifiedName();
    MetadataAccessExpressionContext *metadataAccessExpression();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  MetadataAccessExpressionContext* metadataAccessExpression();
  MetadataAccessExpressionContext* metadataAccessExpression(int precedence);
  class  ViewRenderingMemberContext : public antlr4::ParserRuleContext {
  public:
    ViewRenderingMemberContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    ViewRenderingUsageContext *viewRenderingUsage();
    VisibilityIndicatorContext *visibilityIndicator();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ViewRenderingMemberContext* viewRenderingMember();

  class  ViewRenderingUsageContext : public antlr4::ParserRuleContext {
  public:
    ViewRenderingUsageContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    OwnedReferenceSubsettingContext *ownedReferenceSubsetting();
    FeatureSpecializationPartContext *featureSpecializationPart();
    UsageBodyContext *usageBody();
    UsageDeclarationContext *usageDeclaration();
    ValuePartContext *valuePart();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ViewRenderingUsageContext* viewRenderingUsage();

  class  ViewBodyContext : public antlr4::ParserRuleContext {
  public:
    ViewBodyContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<ViewBodyItemContext *> viewBodyItem();
    ViewBodyItemContext* viewBodyItem(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ViewBodyContext* viewBody();

  class  ViewBodyItemContext : public antlr4::ParserRuleContext {
  public:
    ViewBodyItemContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    SatisfyStatementContext *satisfyStatement();
    ExposeStatementContext *exposeStatement();
    ElementFilterMemberContext *elementFilterMember();
    ViewRenderingMemberContext *viewRenderingMember();
    DefinitionBodyItemContext *definitionBodyItem();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ViewBodyItemContext* viewBodyItem();

  class  ExposeStatementContext : public antlr4::ParserRuleContext {
  public:
    ExposeStatementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    MembershipExposeContext *membershipExpose();
    NamespaceExposeContext *namespaceExpose();
    RelationshipBodyContext *relationshipBody();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ExposeStatementContext* exposeStatement();

  class  MembershipExposeContext : public antlr4::ParserRuleContext {
  public:
    MembershipExposeContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    QualifiedNameContext *qualifiedName();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  MembershipExposeContext* membershipExpose();

  class  NamespaceExposeContext : public antlr4::ParserRuleContext {
  public:
    NamespaceExposeContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    QualifiedNameContext *qualifiedName();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  NamespaceExposeContext* namespaceExpose();

  class  ConcernBodyContext : public antlr4::ParserRuleContext {
  public:
    ConcernBodyContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<ConcernBodyItemContext *> concernBodyItem();
    ConcernBodyItemContext* concernBodyItem(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ConcernBodyContext* concernBody();

  class  ConcernBodyItemContext : public antlr4::ParserRuleContext {
  public:
    ConcernBodyItemContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    DefinitionBodyItemContext *definitionBodyItem();
    RequirementMemberContext *requirementMember();
    DocStatementContext *docStatement();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ConcernBodyItemContext* concernBodyItem();

  class  RequirementMemberContext : public antlr4::ParserRuleContext {
  public:
    RequirementMemberContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    UsageCompletionContext *usageCompletion();
    UsageDeclarationContext *usageDeclaration();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  RequirementMemberContext* requirementMember();

  class  RequirementConstraintMemberContext : public antlr4::ParserRuleContext {
  public:
    RequirementConstraintMemberContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    ExpressionContext *expression();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  RequirementConstraintMemberContext* requirementConstraintMember();

  class  PortBodyContext : public antlr4::ParserRuleContext {
  public:
    PortBodyContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<DefinitionContext *> definition();
    DefinitionContext* definition(size_t i);
    std::vector<UsageContext *> usage();
    UsageContext* usage(size_t i);
    std::vector<AttributeContext *> attribute();
    AttributeContext* attribute(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  PortBodyContext* portBody();

  class  ConnectorBodyContext : public antlr4::ParserRuleContext {
  public:
    ConnectorBodyContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<DefinitionContext *> definition();
    DefinitionContext* definition(size_t i);
    std::vector<UsageContext *> usage();
    UsageContext* usage(size_t i);
    std::vector<AttributeContext *> attribute();
    AttributeContext* attribute(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ConnectorBodyContext* connectorBody();

  class  FlowBodyContext : public antlr4::ParserRuleContext {
  public:
    FlowBodyContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<DefinitionContext *> definition();
    DefinitionContext* definition(size_t i);
    std::vector<UsageContext *> usage();
    UsageContext* usage(size_t i);
    std::vector<AttributeContext *> attribute();
    AttributeContext* attribute(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  FlowBodyContext* flowBody();

  class  InterfaceBodyContext : public antlr4::ParserRuleContext {
  public:
    InterfaceBodyContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<DefinitionContext *> definition();
    DefinitionContext* definition(size_t i);
    std::vector<UsageContext *> usage();
    UsageContext* usage(size_t i);
    std::vector<AttributeContext *> attribute();
    AttributeContext* attribute(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  InterfaceBodyContext* interfaceBody();

  class  DatatypeBodyContext : public antlr4::ParserRuleContext {
  public:
    DatatypeBodyContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<AttributeContext *> attribute();
    AttributeContext* attribute(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  DatatypeBodyContext* datatypeBody();

  class  ClassBodyContext : public antlr4::ParserRuleContext {
  public:
    ClassBodyContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<DefinitionContext *> definition();
    DefinitionContext* definition(size_t i);
    std::vector<UsageContext *> usage();
    UsageContext* usage(size_t i);
    std::vector<AttributeContext *> attribute();
    AttributeContext* attribute(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ClassBodyContext* classBody();

  class  EnumBodyContext : public antlr4::ParserRuleContext {
  public:
    EnumBodyContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<EnumValueContext *> enumValue();
    EnumValueContext* enumValue(size_t i);
    std::vector<AnnotatingMemberContext *> annotatingMember();
    AnnotatingMemberContext* annotatingMember(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  EnumBodyContext* enumBody();

  class  AnnotatingMemberContext : public antlr4::ParserRuleContext {
  public:
    AnnotatingMemberContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    DocStatementContext *docStatement();
    CommentStatementContext *commentStatement();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  AnnotatingMemberContext* annotatingMember();

  class  DocStatementContext : public antlr4::ParserRuleContext {
  public:
    DocStatementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    BlockCommentContext *blockComment();
    IdentificationContext *identification();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  DocStatementContext* docStatement();

  class  CommentStatementContext : public antlr4::ParserRuleContext {
  public:
    CommentStatementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    BlockCommentContext *blockComment();
    IdentificationContext *identification();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  CommentStatementContext* commentStatement();

  class  UsageContext : public antlr4::ParserRuleContext {
  public:
    UsageContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    PartUsageContext *partUsage();
    ActionUsageContext *actionUsage();
    AnalysisUsageContext *analysisUsage();
    RenderingUsageContext *renderingUsage();
    StateUsageContext *stateUsage();
    RequirementUsageContext *requirementUsage();
    ConstraintUsageContext *constraintUsage();
    PortUsageContext *portUsage();
    ConnectorUsageContext *connectorUsage();
    FlowUsageContext *flowUsage();
    ConnectionUsageContext *connectionUsage();
    AllocationUsageContext *allocationUsage();
    ItemUsageContext *itemUsage();
    ViewUsageContext *viewUsage();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  UsageContext* usage();

  class  StatementContext : public antlr4::ParserRuleContext {
  public:
    StatementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    BindStatementContext *bindStatement();
    ConnectStatementContext *connectStatement();
    AllocateStatementContext *allocateStatement();
    EndStatementContext *endStatement();
    FlowStatementContext *flowStatement();
    PerformStatementContext *performStatement();
    AssertStatementContext *assertStatement();
    RequireStatementContext *requireStatement();
    AssumeStatementContext *assumeStatement();
    RedefinesStatementContext *redefinesStatement();
    SatisfyStatementContext *satisfyStatement();
    ConstraintStatementContext *constraintStatement();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  StatementContext* statement();

  class  BindStatementContext : public antlr4::ParserRuleContext {
  public:
    BindStatementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<ConnectorEndMemberContext *> connectorEndMember();
    ConnectorEndMemberContext* connectorEndMember(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  BindStatementContext* bindStatement();

  class  ConnectStatementContext : public antlr4::ParserRuleContext {
  public:
    ConnectStatementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    ConnectorPartContext *connectorPart();
    std::vector<DefinitionBodyItemContext *> definitionBodyItem();
    DefinitionBodyItemContext* definitionBodyItem(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ConnectStatementContext* connectStatement();

  class  AllocateStatementContext : public antlr4::ParserRuleContext {
  public:
    AllocateStatementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    AllocationPartContext *allocationPart();
    std::vector<DefinitionBodyItemContext *> definitionBodyItem();
    DefinitionBodyItemContext* definitionBodyItem(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  AllocateStatementContext* allocateStatement();

  class  AllocationPartContext : public antlr4::ParserRuleContext {
  public:
    AllocationPartContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    BinaryAllocationPartContext *binaryAllocationPart();
    NaryAllocationPartContext *naryAllocationPart();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  AllocationPartContext* allocationPart();

  class  BinaryAllocationPartContext : public antlr4::ParserRuleContext {
  public:
    BinaryAllocationPartContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<ConnectorEndMemberContext *> connectorEndMember();
    ConnectorEndMemberContext* connectorEndMember(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  BinaryAllocationPartContext* binaryAllocationPart();

  class  NaryAllocationPartContext : public antlr4::ParserRuleContext {
  public:
    NaryAllocationPartContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<ConnectorEndMemberContext *> connectorEndMember();
    ConnectorEndMemberContext* connectorEndMember(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  NaryAllocationPartContext* naryAllocationPart();

  class  EndStatementContext : public antlr4::ParserRuleContext {
  public:
    EndStatementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    NameContext *name();
    OwnedReferenceSubsettingContext *ownedReferenceSubsetting();
    ReferencesKeywordContext *referencesKeyword();
    IdentifierContext *identifier();
    antlr4::tree::TerminalNode *COLON();
    QualifiedNameContext *qualifiedName();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  EndStatementContext* endStatement();

  class  FlowDefinitionContext : public antlr4::ParserRuleContext {
  public:
    FlowDefinitionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    OccurrenceDefinitionPrefixContext *occurrenceDefinitionPrefix();
    DefinitionDeclarationContext *definitionDeclaration();
    DefinitionBodyContext *definitionBody();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  FlowDefinitionContext* flowDefinition();

  class  FlowStatementContext : public antlr4::ParserRuleContext {
  public:
    FlowStatementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    FlowDeclarationContext *flowDeclaration();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  FlowStatementContext* flowStatement();

  class  FlowDeclarationContext : public antlr4::ParserRuleContext {
  public:
    FlowDeclarationContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    UsageDeclarationContext *usageDeclaration();
    ValuePartContext *valuePart();
    PayloadFeatureMemberContext *payloadFeatureMember();
    std::vector<FlowEndMemberContext *> flowEndMember();
    FlowEndMemberContext* flowEndMember(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  FlowDeclarationContext* flowDeclaration();

  class  FlowEndMemberContext : public antlr4::ParserRuleContext {
  public:
    FlowEndMemberContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    ConnectorEndMemberContext *connectorEndMember();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  FlowEndMemberContext* flowEndMember();

  class  PayloadFeatureMemberContext : public antlr4::ParserRuleContext {
  public:
    PayloadFeatureMemberContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    UsageDeclarationContext *usageDeclaration();
    ValuePartContext *valuePart();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  PayloadFeatureMemberContext* payloadFeatureMember();

  class  PerformStatementContext : public antlr4::ParserRuleContext {
  public:
    PerformStatementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    UsageBodyContext *usageBody();
    OwnedReferenceSubsettingContext *ownedReferenceSubsetting();
    ValuePartContext *valuePart();
    FeatureSpecializationPartContext *featureSpecializationPart();
    UsageDeclarationContext *usageDeclaration();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  PerformStatementContext* performStatement();

  class  AssertStatementContext : public antlr4::ParserRuleContext {
  public:
    AssertStatementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    ExpressionContext *expression();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  AssertStatementContext* assertStatement();

  class  RequireStatementContext : public antlr4::ParserRuleContext {
  public:
    RequireStatementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    ExpressionContext *expression();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  RequireStatementContext* requireStatement();

  class  SatisfyStatementContext : public antlr4::ParserRuleContext {
  public:
    SatisfyStatementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<OwnedReferenceSubsettingContext *> ownedReferenceSubsetting();
    OwnedReferenceSubsettingContext* ownedReferenceSubsetting(size_t i);
    FeatureSpecializationPartContext *featureSpecializationPart();
    std::vector<RequirementBodyItemContext *> requirementBodyItem();
    RequirementBodyItemContext* requirementBodyItem(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  SatisfyStatementContext* satisfyStatement();

  class  RequirementBodyItemContext : public antlr4::ParserRuleContext {
  public:
    RequirementBodyItemContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    DefinitionBodyItemContext *definitionBodyItem();
    RequirementMemberContext *requirementMember();
    RequirementConstraintMemberContext *requirementConstraintMember();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  RequirementBodyItemContext* requirementBodyItem();

  class  AssumeStatementContext : public antlr4::ParserRuleContext {
  public:
    AssumeStatementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    ExpressionContext *expression();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  AssumeStatementContext* assumeStatement();

  class  RedefinesStatementContext : public antlr4::ParserRuleContext {
  public:
    RedefinesStatementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    QualifiedNameContext *qualifiedName();
    ExpressionContext *expression();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  RedefinesStatementContext* redefinesStatement();

  class  ActionInputOutputContext : public antlr4::ParserRuleContext {
  public:
    ActionInputOutputContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    UsageDeclarationContext *usageDeclaration();
    ValuePartContext *valuePart();
    UsageBodyContext *usageBody();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ActionInputOutputContext* actionInputOutput();

  class  ConnectorPartContext : public antlr4::ParserRuleContext {
  public:
    ConnectorPartContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    BinaryConnectorPartContext *binaryConnectorPart();
    NaryConnectorPartContext *naryConnectorPart();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ConnectorPartContext* connectorPart();

  class  BinaryConnectorPartContext : public antlr4::ParserRuleContext {
  public:
    BinaryConnectorPartContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<ConnectorEndMemberContext *> connectorEndMember();
    ConnectorEndMemberContext* connectorEndMember(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  BinaryConnectorPartContext* binaryConnectorPart();

  class  NaryConnectorPartContext : public antlr4::ParserRuleContext {
  public:
    NaryConnectorPartContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<ConnectorEndMemberContext *> connectorEndMember();
    ConnectorEndMemberContext* connectorEndMember(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  NaryConnectorPartContext* naryConnectorPart();

  class  ConnectorEndMemberContext : public antlr4::ParserRuleContext {
  public:
    ConnectorEndMemberContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    NameContext *name();
    OwnedReferenceSubsettingContext *ownedReferenceSubsetting();
    OwnedMultiplicityContext *ownedMultiplicity();
    ReferencesKeywordContext *referencesKeyword();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ConnectorEndMemberContext* connectorEndMember();

  class  OwnedReferenceSubsettingContext : public antlr4::ParserRuleContext {
  public:
    OwnedReferenceSubsettingContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    QualifiedNameContext *qualifiedName();
    MemberAccessExpressionContext *memberAccessExpression();
    IdentifierContext *identifier();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  OwnedReferenceSubsettingContext* ownedReferenceSubsetting();

  class  ReferencesKeywordContext : public antlr4::ParserRuleContext {
  public:
    ReferencesKeywordContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ReferencesKeywordContext* referencesKeyword();

  class  OwnedMultiplicityContext : public antlr4::ParserRuleContext {
  public:
    OwnedMultiplicityContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<antlr4::tree::TerminalNode *> Integer();
    antlr4::tree::TerminalNode* Integer(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  OwnedMultiplicityContext* ownedMultiplicity();

  class  TransitionContext : public antlr4::ParserRuleContext {
  public:
    TransitionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    TransitionBodyContext *transitionBody();
    IdentifierContext *identifier();
    QualifiedNameContext *qualifiedName();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  TransitionContext* transition();

  class  TransitionBodyContext : public antlr4::ParserRuleContext {
  public:
    TransitionBodyContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    SimpleTransitionBodyContext *simpleTransitionBody();
    ComplexTransitionBodyContext *complexTransitionBody();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  TransitionBodyContext* transitionBody();

  class  SimpleTransitionBodyContext : public antlr4::ParserRuleContext {
  public:
    SimpleTransitionBodyContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<StateReferenceContext *> stateReference();
    StateReferenceContext* stateReference(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  SimpleTransitionBodyContext* simpleTransitionBody();

  class  ComplexTransitionBodyContext : public antlr4::ParserRuleContext {
  public:
    ComplexTransitionBodyContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<StateReferenceContext *> stateReference();
    StateReferenceContext* stateReference(size_t i);
    TransitionTriggerPartContext *transitionTriggerPart();
    TransitionGuardPartContext *transitionGuardPart();
    TransitionEffectPartContext *transitionEffectPart();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ComplexTransitionBodyContext* complexTransitionBody();

  class  TransitionTriggerPartContext : public antlr4::ParserRuleContext {
  public:
    TransitionTriggerPartContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    AcceptClauseContext *acceptClause();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  TransitionTriggerPartContext* transitionTriggerPart();

  class  AcceptClauseContext : public antlr4::ParserRuleContext {
  public:
    AcceptClauseContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    AcceptItemContext *acceptItem();
    AcceptExpressionContext *acceptExpression();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  AcceptClauseContext* acceptClause();

  class  AcceptItemContext : public antlr4::ParserRuleContext {
  public:
    AcceptItemContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    UsageDeclarationContext *usageDeclaration();
    OwnedReferenceSubsettingContext *ownedReferenceSubsetting();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  AcceptItemContext* acceptItem();

  class  AcceptExpressionContext : public antlr4::ParserRuleContext {
  public:
    AcceptExpressionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    ExpressionContext *expression();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  AcceptExpressionContext* acceptExpression();

  class  TransitionGuardPartContext : public antlr4::ParserRuleContext {
  public:
    TransitionGuardPartContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    ExpressionContext *expression();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  TransitionGuardPartContext* transitionGuardPart();

  class  TransitionEffectPartContext : public antlr4::ParserRuleContext {
  public:
    TransitionEffectPartContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    TransitionActionContext *transitionAction();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  TransitionEffectPartContext* transitionEffectPart();

  class  TransitionActionContext : public antlr4::ParserRuleContext {
  public:
    TransitionActionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    SendStatementContext *sendStatement();
    ActionReferenceContext *actionReference();
    ActionInvocationContext *actionInvocation();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  TransitionActionContext* transitionAction();

  class  SendStatementContext : public antlr4::ParserRuleContext {
  public:
    SendStatementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    OwnedReferenceSubsettingContext *ownedReferenceSubsetting();
    SendActionPartContext *sendActionPart();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  SendStatementContext* sendStatement();

  class  SendActionPartContext : public antlr4::ParserRuleContext {
  public:
    SendActionPartContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    ActionInvocationContext *actionInvocation();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  SendActionPartContext* sendActionPart();

  class  ActionReferenceContext : public antlr4::ParserRuleContext {
  public:
    ActionReferenceContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    OwnedReferenceSubsettingContext *ownedReferenceSubsetting();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ActionReferenceContext* actionReference();

  class  ActionInvocationContext : public antlr4::ParserRuleContext {
  public:
    ActionInvocationContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    IdentifierContext *identifier();
    std::vector<ActionArgumentContext *> actionArgument();
    ActionArgumentContext* actionArgument(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ActionInvocationContext* actionInvocation();

  class  ActionArgumentContext : public antlr4::ParserRuleContext {
  public:
    ActionArgumentContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    ExpressionContext *expression();
    IdentifierContext *identifier();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ActionArgumentContext* actionArgument();

  class  StateReferenceContext : public antlr4::ParserRuleContext {
  public:
    StateReferenceContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    IdentifierContext *identifier();
    QualifiedNameContext *qualifiedName();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  StateReferenceContext* stateReference();

  class  TriggerExpressionContext : public antlr4::ParserRuleContext {
  public:
    TriggerExpressionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<QualifiedNameContext *> qualifiedName();
    QualifiedNameContext* qualifiedName(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  TriggerExpressionContext* triggerExpression();

  class  GuardExpressionContext : public antlr4::ParserRuleContext {
  public:
    GuardExpressionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    IdentifierContext *identifier();
    QualifiedNameContext *qualifiedName();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  GuardExpressionContext* guardExpression();

  class  EffectExpressionContext : public antlr4::ParserRuleContext {
  public:
    EffectExpressionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<QualifiedNameContext *> qualifiedName();
    QualifiedNameContext* qualifiedName(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  EffectExpressionContext* effectExpression();

  class  EnumValueContext : public antlr4::ParserRuleContext {
  public:
    EnumValueContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    IdentifierContext *identifier();
    antlr4::tree::TerminalNode *Integer();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  EnumValueContext* enumValue();

  class  AttributeContext : public antlr4::ParserRuleContext {
  public:
    AttributeContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    AttributeContentContext *attributeContent();
    std::vector<FeaturePrefixContext *> featurePrefix();
    FeaturePrefixContext* featurePrefix(size_t i);
    AttributeDefinitionContext *attributeDefinition();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  AttributeContext* attribute();

  class  FeaturePrefixContext : public antlr4::ParserRuleContext {
  public:
    FeaturePrefixContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  FeaturePrefixContext* featurePrefix();

  class  AttributeContentContext : public antlr4::ParserRuleContext {
  public:
    AttributeContentContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    QualifiedNameContext *qualifiedName();
    ExpressionContext *expression();
    IdentifierContext *identifier();
    AttributeExpressionsContext *attributeExpressions();
    ValuePartContext *valuePart();
    RedefinitionExpressionContext *redefinitionExpression();
    TypingExpressionContext *typingExpression();
    SubsettingExpressionContext *subsettingExpression();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  AttributeContentContext* attributeContent();

  class  AttributeExpressionsContext : public antlr4::ParserRuleContext {
  public:
    AttributeExpressionsContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    SubsettingExpressionContext *subsettingExpression();
    RedefinitionExpressionContext *redefinitionExpression();
    TypingExpressionContext *typingExpression();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  AttributeExpressionsContext* attributeExpressions();

  class  SubsettingExpressionContext : public antlr4::ParserRuleContext {
  public:
    SubsettingExpressionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *SUBSETS_OP();
    QualifiedNameContext *qualifiedName();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  SubsettingExpressionContext* subsettingExpression();

  class  RedefinitionExpressionContext : public antlr4::ParserRuleContext {
  public:
    RedefinitionExpressionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *REDEFINES_OP();
    QualifiedNameContext *qualifiedName();
    OwnedMultiplicityContext *ownedMultiplicity();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  RedefinitionExpressionContext* redefinitionExpression();

  class  TypingExpressionContext : public antlr4::ParserRuleContext {
  public:
    TypingExpressionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *COLON();
    QualifiedNameContext *qualifiedName();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  TypingExpressionContext* typingExpression();

  class  SubclassificationExpressionContext : public antlr4::ParserRuleContext {
  public:
    SubclassificationExpressionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *SUBSETS_OP();
    std::vector<QualifiedNameContext *> qualifiedName();
    QualifiedNameContext* qualifiedName(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  SubclassificationExpressionContext* subclassificationExpression();

  class  AttributeBodyContext : public antlr4::ParserRuleContext {
  public:
    AttributeBodyContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<DefinitionContext *> definition();
    DefinitionContext* definition(size_t i);
    std::vector<UsageContext *> usage();
    UsageContext* usage(size_t i);
    std::vector<AttributeContext *> attribute();
    AttributeContext* attribute(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  AttributeBodyContext* attributeBody();

  class  ImportStatementContext : public antlr4::ParserRuleContext {
  public:
    ImportStatementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    RelationshipBodyContext *relationshipBody();
    MembershipImportContext *membershipImport();
    NamespaceImportContext *namespaceImport();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ImportStatementContext* importStatement();

  class  MembershipImportContext : public antlr4::ParserRuleContext {
  public:
    MembershipImportContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    ImportPrefixContext *importPrefix();
    ImportedMembershipContext *importedMembership();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  MembershipImportContext* membershipImport();

  class  ImportedMembershipContext : public antlr4::ParserRuleContext {
  public:
    ImportedMembershipContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    QualifiedNameContext *qualifiedName();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ImportedMembershipContext* importedMembership();

  class  NamespaceImportContext : public antlr4::ParserRuleContext {
  public:
    NamespaceImportContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    ImportPrefixContext *importPrefix();
    ImportedNamespaceContext *importedNamespace();
    FilterPackageContext *filterPackage();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  NamespaceImportContext* namespaceImport();

  class  ImportedNamespaceContext : public antlr4::ParserRuleContext {
  public:
    ImportedNamespaceContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    QualifiedNameContext *qualifiedName();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ImportedNamespaceContext* importedNamespace();

  class  FilterPackageContext : public antlr4::ParserRuleContext {
  public:
    FilterPackageContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    QualifiedNameContext *qualifiedName();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  FilterPackageContext* filterPackage();

  class  ImportPrefixContext : public antlr4::ParserRuleContext {
  public:
    ImportPrefixContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    VisibilityIndicatorContext *visibilityIndicator();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ImportPrefixContext* importPrefix();

  class  RelationshipBodyContext : public antlr4::ParserRuleContext {
  public:
    RelationshipBodyContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<RelationshipBodyItemContext *> relationshipBodyItem();
    RelationshipBodyItemContext* relationshipBodyItem(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  RelationshipBodyContext* relationshipBody();

  class  RelationshipBodyItemContext : public antlr4::ParserRuleContext {
  public:
    RelationshipBodyItemContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    AnnotatingMemberContext *annotatingMember();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  RelationshipBodyItemContext* relationshipBodyItem();

  class  RelationshipContext : public antlr4::ParserRuleContext {
  public:
    RelationshipContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    SubclassificationContext *subclassification();
    SubsettingContext *subsetting();
    RedefinitionContext *redefinition();
    ConjugationContext *conjugation();
    DependencyContext *dependency();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  RelationshipContext* relationship();

  class  SubclassificationContext : public antlr4::ParserRuleContext {
  public:
    SubclassificationContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    TypeExpressionContext *typeExpression();
    std::vector<QualifiedNameContext *> qualifiedName();
    QualifiedNameContext* qualifiedName(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  SubclassificationContext* subclassification();

  class  SubsettingContext : public antlr4::ParserRuleContext {
  public:
    SubsettingContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    FeatureExpressionContext *featureExpression();
    QualifiedNameContext *qualifiedName();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  SubsettingContext* subsetting();

  class  RedefinitionContext : public antlr4::ParserRuleContext {
  public:
    RedefinitionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    FeatureExpressionContext *featureExpression();
    QualifiedNameContext *qualifiedName();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  RedefinitionContext* redefinition();

  class  ConjugationContext : public antlr4::ParserRuleContext {
  public:
    ConjugationContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    TypeExpressionContext *typeExpression();
    QualifiedNameContext *qualifiedName();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ConjugationContext* conjugation();

  class  DependencyContext : public antlr4::ParserRuleContext {
  public:
    DependencyContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    ElementExpressionContext *elementExpression();
    std::vector<QualifiedNameContext *> qualifiedName();
    QualifiedNameContext* qualifiedName(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  DependencyContext* dependency();

  class  TypeExpressionContext : public antlr4::ParserRuleContext {
  public:
    TypeExpressionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    QualifiedNameContext *qualifiedName();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  TypeExpressionContext* typeExpression();

  class  FeatureExpressionContext : public antlr4::ParserRuleContext {
  public:
    FeatureExpressionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    QualifiedNameContext *qualifiedName();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  FeatureExpressionContext* featureExpression();

  class  ElementExpressionContext : public antlr4::ParserRuleContext {
  public:
    ElementExpressionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    QualifiedNameContext *qualifiedName();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ElementExpressionContext* elementExpression();

  class  QualifiedNameContext : public antlr4::ParserRuleContext {
  public:
    QualifiedNameContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<IdentifierContext *> identifier();
    IdentifierContext* identifier(size_t i);
    MemberAccessExpressionContext *memberAccessExpression();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  QualifiedNameContext* qualifiedName();

  class  MemberAccessExpressionContext : public antlr4::ParserRuleContext {
  public:
    MemberAccessExpressionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<IdentifierContext *> identifier();
    IdentifierContext* identifier(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  MemberAccessExpressionContext* memberAccessExpression();

  class  IdentifierContext : public antlr4::ParserRuleContext {
  public:
    IdentifierContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *IDENTIFIER();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  IdentifierContext* identifier();

  class  BlockCommentContext : public antlr4::ParserRuleContext {
  public:
    BlockCommentContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *BLOCK_COMMENT_TOKEN();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  BlockCommentContext* blockComment();


  bool sempred(antlr4::RuleContext *_localctx, size_t ruleIndex, size_t predicateIndex) override;

  bool filterExpressionSempred(FilterExpressionContext *_localctx, size_t predicateIndex);
  bool metadataAccessExpressionSempred(MetadataAccessExpressionContext *_localctx, size_t predicateIndex);

  // By default the static state used to implement the parser is lazily initialized during the first
  // call to the constructor. You can call this function if you wish to initialize the static state
  // ahead of time.
  static void initialize();

private:
};

}  // namespace SysML
