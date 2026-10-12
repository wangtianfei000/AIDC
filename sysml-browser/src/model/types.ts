export type CapabilityStatus = 'supported' | 'partial' | 'unsupported' | 'unknown';

export interface Position {
  line: number;
  column: number;
  offset: number;
}

export interface SourceSpan {
  uri: string;
  start: Position;
  end: Position;
}

export interface ModelElement {
  id: string;
  kind: string;
  name: string | null;
  qualifiedName: string | null;
  ownerId: string | null;
  source: SourceSpan | null;
  properties: Record<string, unknown>;
}

export interface ModelRelationship {
  id: string;
  kind: string;
  sourceIds: string[];
  targetIds: string[];
  ownerId: string | null;
  source: SourceSpan | null;
  properties: Record<string, unknown>;
}

export interface Diagnostic {
  code: string;
  severity: 'error' | 'warning' | 'info';
  message: string;
  source: SourceSpan | null;
  relatedElementIds?: string[];
}

export interface ModelSnapshot {
  schemaVersion: '0.1';
  document: {
    uri: string;
    contentHash: string;
    parser: { name: string; baseline: string; capabilityVersion: string };
  };
  elements: ModelElement[];
  relationships: ModelRelationship[];
  diagnostics: Diagnostic[];
  capabilities: Record<string, CapabilityStatus>;
  calculationModel?: CalculationModel;
  viewModel?: ViewModel;
}

export interface CalculationParameter {
  id: string;
  elementId: string;
  name: string;
  ownerId: string;
  role: string;
  typeName: string;
  unit: string | null;
  expression: string;
  expressionAst: Record<string, unknown>;
  source: SourceSpan | null;
}

export interface CalculationDefinition {
  id: string;
  elementId: string;
  name: string;
  inputParameterIds: string[];
  outputParameterIds: string[];
  localParameterIds: string[];
  source: SourceSpan | null;
}

export interface CalculationBinding {
  index: number;
  formalParameterId: string | null;
  formalParameterName: string | null;
  actualParameterId: string | null;
  actualReference: string | null;
  argument: Record<string, unknown>;
}

export interface CalculationUsage {
  id: string;
  analysisId: string | null;
  definitionId: string | null;
  definitionName: string;
  resultParameterId: string;
  arguments: Array<Record<string, unknown>>;
  bindings?: CalculationBinding[];
  actualParameterIds?: string[];
  source: SourceSpan | null;
}

export interface CalculationConstraint {
  id: string;
  elementId: string;
  name: string;
  analysisId: string;
  expression: string;
  expressionAst: Record<string, unknown>;
  source: SourceSpan | null;
}

export interface CalculationModel {
  version: string;
  definitions: CalculationDefinition[];
  usages: CalculationUsage[];
  analyses: Array<Record<string, unknown>>;
  parameters: CalculationParameter[];
  constraints: CalculationConstraint[];
}

export interface ViewRendering {
  id: string;
  elementId: string;
  name: string;
  renderingKind: string;
  renderingType: string;
  columns: Array<{ id: string; name: string; key?: string; label?: string; columnView: string; source: SourceSpan | null }>;
  source: SourceSpan | null;
}

export interface SysmlViewUsage {
  id: string;
  elementId: string;
  name: string;
  definitionId: string;
  definitionReference: string;
  renderingId: string;
  renderingReference: string;
  exposedElementIds: string[];
  exposedReferences: string[];
  source: SourceSpan | null;
}

export interface ViewModel {
  version: string;
  definitions: Array<Record<string, unknown>>;
  renderings: ViewRendering[];
  views: SysmlViewUsage[];
}

export type AnalysisStatus = 'computed' | 'missing-input' | 'unit-error' | 'unsupported' | 'cycle' | 'runtime-error';

export interface AnalysisResult {
  parameterId: string;
  name: string;
  typeName: string;
  status: AnalysisStatus;
  formula: string;
  formulaAst: Record<string, unknown>;
  dimension: string;
  value?: number | boolean;
  unit?: string | null;
  siValue?: number;
  message?: string;
  inputParameterIds: string[];
  calculationUsageId?: string;
  calculationDefinitionId?: string;
  source: SourceSpan | null;
}

export interface AnalysisConstraint {
  constraintId: string;
  name: string;
  expression: string;
  expressionAst: Record<string, unknown>;
  status: 'pass' | 'fail' | Exclude<AnalysisStatus, 'computed'>;
  actual?: number;
  target?: number;
  margin?: number;
  unit?: string | null;
  comparison?: string;
  resultParameterId?: string;
  targetParameterId?: string;
  inputParameterIds?: string[];
  message?: string;
  source: SourceSpan | null;
}

export interface AnalysisCaseResult {
  analysisId: string;
  name: string;
  subjectId: string | null;
  subjectReference: string;
  results: AnalysisResult[];
  constraints: AnalysisConstraint[];
  evaluationOrder: string[];
  dependencies: Array<{ sourceParameterId: string; targetParameterId: string }>;
}

export interface AnalysisSnapshot {
  schemaVersion: '0.1';
  document: ModelSnapshot['document'];
  engine: { name: string; version: string };
  analyses: AnalysisCaseResult[];
  diagnostics: Array<{ code: string; severity: 'error' | 'warning' | 'info'; message: string; parameterId: string }>;
  parseDiagnostics?: Diagnostic[];
  capabilities: Record<string, CapabilityStatus>;
}

export interface ModelSource {
  name: string;
  uri: string;
  text: string;
  file?: File;
}

export interface RequirementRecord {
  element: ModelElement;
  displayId: string;
  name: string;
  statement: string;
  constraints: string[];
  constraintStatements: string[];
  attributes: Array<{ name: string; value: string; typeName: string }>;
  requirementType: string;
  status: string;
  discipline: string;
  variant: string;
  coverage: 'verified' | 'uncovered' | 'unsupported';
  packageName: string;
}

export interface RequirementFilters {
  query: string;
  type: string;
  status: string;
  discipline: string;
  variant: string;
  coverage: string;
}
