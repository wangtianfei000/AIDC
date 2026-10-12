import type {
  AnalysisCaseResult, AnalysisConstraint, AnalysisResult, AnalysisSnapshot, CalculationBinding,
  CalculationDefinition, CalculationParameter, CalculationUsage,
  ModelSnapshot, SysmlViewUsage, ViewRendering,
} from '../model/types';

export interface ParameterTableColumn {
  id: string;
  name: string;
  label: string;
}

export interface ParameterTableRow {
  parameterId: string;
  name: string;
  role: string;
  value?: number | boolean;
  unit?: string | null;
  target?: number;
  margin?: number;
  status: string;
  sourceLine?: number;
  formula: string;
  dimension: string;
  inputParameterIds: string[];
  result: AnalysisResult;
  constraint?: AnalysisConstraint;
}

export interface ParameterTableProjection {
  view: SysmlViewUsage;
  rendering: ViewRendering;
  analysis: AnalysisCaseResult;
  columns: ParameterTableColumn[];
  rows: ParameterTableRow[];
  diagnostics: string[];
}

export type SemanticDependencyNodeKind = 'input' | 'definition' | 'usage' | 'result' | 'constraint' | 'target';

export interface SemanticDependencyNode {
  id: string;
  semanticId: string;
  selectionId?: string;
  kind: SemanticDependencyNodeKind;
  name: string;
  subtitle: string;
  formula?: string;
  column: number;
  status?: string;
}

export interface SemanticDependencyEdge {
  id: string;
  source: string;
  target: string;
  label: string;
  kind: 'binding' | 'definition' | 'return' | 'check' | 'target';
}

export interface SemanticDependencyProjection {
  analysis: AnalysisCaseResult;
  root: AnalysisResult;
  nodes: SemanticDependencyNode[];
  edges: SemanticDependencyEdge[];
  definitionFormulas: Array<{ name: string; expression: string }>;
  invocationFormula: string;
  diagnostics: string[];
}

export interface CalculationDetailBinding {
  index: number;
  formalParameterId: string | null;
  formalParameterName: string;
  formalTypeName: string;
  actualParameterId: string | null;
  actualParameterName: string;
  actualReference: string;
  actualExpression: string;
  actualValue?: number | boolean;
  actualUnit?: string | null;
  actualStatus?: string;
}

export interface CalculationDetailParameter {
  id: string;
  elementId: string;
  name: string;
  role: 'input' | 'local' | 'output' | string;
  roleLabel: string;
  typeName: string;
  expression: string;
  value?: number | boolean;
  unit?: string | null;
  status?: string;
}

export interface CalculationDetailDiagnostic {
  code: string;
  severity: 'error' | 'warning' | 'info';
  message: string;
  parameterId?: string;
  selectionId?: string;
  sourceLine?: number;
}

export interface CalculationDetailProjection {
  analysis: AnalysisCaseResult;
  result: AnalysisResult;
  usage?: CalculationUsage;
  definition?: CalculationDefinition;
  constraint?: AnalysisConstraint;
  bindings: CalculationDetailBinding[];
  definitionParameters: CalculationDetailParameter[];
  diagnostics: CalculationDetailDiagnostic[];
}

export function elementTableViews(snapshot: ModelSnapshot): SysmlViewUsage[] {
  const renderings = new Map((snapshot.viewModel?.renderings ?? []).map((item) => [item.id, item]));
  return (snapshot.viewModel?.views ?? []).filter((view) => renderings.get(view.renderingId)?.renderingKind === 'element-table');
}

export function projectParameterTable(
  snapshot: ModelSnapshot,
  analysisSnapshot: AnalysisSnapshot | undefined,
  preferredViewId?: string,
): ParameterTableProjection | undefined {
  if (!analysisSnapshot) return undefined;
  const views = elementTableViews(snapshot);
  const view = views.find((item) => item.id === preferredViewId || item.elementId === preferredViewId) ?? views[0];
  if (!view) return undefined;
  const rendering = snapshot.viewModel?.renderings.find((item) => item.id === view.renderingId);
  if (!rendering) return undefined;

  const resultById = new Map<string, { result: AnalysisResult; analysis: AnalysisCaseResult }>();
  for (const analysis of analysisSnapshot.analyses) {
    for (const result of analysis.results) resultById.set(result.parameterId, { result, analysis });
  }
  const analysis = view.exposedElementIds.map((id) => resultById.get(id)?.analysis).find(Boolean) ?? analysisSnapshot.analyses[0];
  if (!analysis) return undefined;
  const parameters = new Map((snapshot.calculationModel?.parameters ?? []).map((item) => [item.id, item]));
  const constraints = new Map(analysis.constraints.filter((item) => item.resultParameterId).map((item) => [item.resultParameterId!, item]));
  const diagnostics: string[] = [];
  const rows: ParameterTableRow[] = [];
  for (const id of view.exposedElementIds) {
    const entry = resultById.get(id);
    if (!entry) { diagnostics.push(`View expose ${id} 没有对应计算结果`); continue; }
    const parameter = parameters.get(id);
    const constraint = constraints.get(id);
    rows.push({
      parameterId: id,
      name: entry.result.name,
      role: roleLabel(parameter),
      value: entry.result.value,
      unit: entry.result.unit,
      target: constraint?.target,
      margin: constraint?.margin,
      status: constraint?.status ?? entry.result.status,
      sourceLine: entry.result.source?.start.line,
      formula: entry.result.formula,
      dimension: entry.result.dimension,
      inputParameterIds: entry.result.inputParameterIds,
      result: entry.result,
      constraint,
    });
  }
  if (!rendering.columns.length) diagnostics.push('asElementTable 未声明 columnView');
  return {
    view,
    rendering,
    analysis,
    rows,
    columns: rendering.columns.map((item) => ({
      id: item.id,
      name: item.key ?? item.name,
      label: item.label ?? item.name,
    })),
    diagnostics,
  };
}

function roleLabel(parameter?: CalculationParameter) {
  const labels: Record<string, string> = { result: '分析结果', input: '输入', output: '输出', local: '中间量' };
  return labels[parameter?.role ?? ''] ?? parameter?.role ?? '参数';
}

export function analysisStatusLabel(status: string): string {
  return ({
    pass: '通过', fail: '未通过', computed: '已计算', 'missing-input': '缺少输入',
    'unit-error': '单位错误', unsupported: '不支持', cycle: '循环依赖', 'runtime-error': '计算错误',
  } as Record<string, string>)[status] ?? status;
}

export function isAnalysisFailureStatus(status: string): boolean {
  return ['missing-input', 'unit-error', 'unsupported', 'cycle', 'runtime-error'].includes(status);
}

export function formatAnalysisNumber(value: number | boolean | undefined): string {
  if (value == null) return '—';
  if (typeof value === 'boolean') return value ? 'true' : 'false';
  if (!Number.isFinite(value)) return '—';
  return new Intl.NumberFormat('zh-CN', { maximumFractionDigits: 6 }).format(Math.abs(value) < 1e-12 ? 0 : value);
}

export function parameterName(snapshot: ModelSnapshot, id: string): string {
  return snapshot.calculationModel?.parameters.find((item) => item.id === id)?.name
    ?? snapshot.elements.find((item) => item.id === id)?.name
    ?? id;
}

export function dependencyClosure(analysis: AnalysisCaseResult, rootId: string): string[] {
  const targets = new Map<string, string[]>();
  for (const edge of analysis.dependencies) targets.set(edge.sourceParameterId, [...(targets.get(edge.sourceParameterId) ?? []), edge.targetParameterId]);
  const result: string[] = [];
  const visited = new Set<string>();
  const visit = (id: string) => {
    if (visited.has(id)) return;
    visited.add(id); result.push(id);
    for (const target of targets.get(id) ?? []) visit(target);
  };
  visit(rootId);
  return result;
}

export function projectCalculationDetail(
  snapshot: ModelSnapshot,
  analysisSnapshot: AnalysisSnapshot | undefined,
  resultId: string,
): CalculationDetailProjection | undefined {
  if (!analysisSnapshot || !snapshot.calculationModel) return undefined;
  const analysis = analysisSnapshot.analyses.find((item) => item.results.some((result) => result.parameterId === resultId));
  const result = analysis?.results.find((item) => item.parameterId === resultId);
  if (!analysis || !result) return undefined;

  const model = snapshot.calculationModel;
  const parameters = new Map(model.parameters.map((item) => [item.id, item]));
  const results = new Map(analysis.results.map((item) => [item.parameterId, item]));
  const usage = model.usages.find((item) => item.id === result.calculationUsageId || item.resultParameterId === resultId);
  const definition = model.definitions.find((item) => item.id === (usage?.definitionId ?? result.calculationDefinitionId));
  const constraint = analysis.constraints.find((item) => item.resultParameterId === resultId);

  const bindings = (usage?.bindings ?? []).map((binding: CalculationBinding): CalculationDetailBinding => {
    const formal = binding.formalParameterId ? parameters.get(binding.formalParameterId) : undefined;
    const actual = binding.actualParameterId ? parameters.get(binding.actualParameterId) : undefined;
    const actualResult = binding.actualParameterId ? results.get(binding.actualParameterId) : undefined;
    return {
      index: binding.index,
      formalParameterId: binding.formalParameterId,
      formalParameterName: binding.formalParameterName ?? formal?.name ?? `输入 ${binding.index + 1}`,
      formalTypeName: formal?.typeName ?? '—',
      actualParameterId: binding.actualParameterId,
      actualParameterName: actual?.name ?? actualResult?.name ?? binding.actualReference ?? '未解析',
      actualReference: binding.actualReference ?? '—',
      actualExpression: actual?.expression ?? '',
      actualValue: actualResult?.value,
      actualUnit: actualResult?.unit ?? actual?.unit,
      actualStatus: actualResult?.status,
    };
  });

  const parameterIds = definition
    ? [...definition.inputParameterIds, ...definition.localParameterIds, ...definition.outputParameterIds]
    : [];
  const definitionParameters = parameterIds.flatMap((id): CalculationDetailParameter[] => {
    const parameter = parameters.get(id);
    if (!parameter) return [];
    const parameterResult = results.get(id);
    return [{
      id: parameter.id,
      elementId: parameter.elementId,
      name: parameter.name,
      role: parameter.role,
      roleLabel: parameter.role === 'input' ? '输入' : parameter.role === 'local' ? '局部量' : parameter.role === 'output' ? '返回值' : roleLabel(parameter),
      typeName: parameter.typeName,
      expression: parameter.expression,
      value: parameterResult?.value,
      unit: parameterResult?.unit ?? parameter.unit,
      status: parameterResult?.status,
    }];
  });

  const relatedIds = new Set<string>([
    resultId,
    ...result.inputParameterIds,
    ...(usage?.actualParameterIds ?? []),
    ...parameterIds,
    ...(constraint?.inputParameterIds ?? []),
    constraint?.targetParameterId ?? '',
  ].filter(Boolean));
  const diagnostics: CalculationDetailDiagnostic[] = analysisSnapshot.diagnostics
    .filter((item) => relatedIds.has(item.parameterId))
    .map((item) => {
      const parameter = parameters.get(item.parameterId);
      const parameterResult = results.get(item.parameterId);
      return {
        ...item,
        selectionId: parameter?.elementId ?? item.parameterId,
        sourceLine: parameterResult?.source?.start.line ?? parameter?.source?.start.line,
      };
    });
  for (const item of analysisSnapshot.parseDiagnostics ?? []) {
    if (item.relatedElementIds?.some((id) => relatedIds.has(id))) {
      diagnostics.push({ code: item.code, severity: item.severity, message: item.message, sourceLine: item.source?.start.line });
    }
  }
  const resultParameter = parameters.get(resultId);
  const resultSelectionId = resultParameter?.elementId ?? resultId;
  const resultSourceLine = result.source?.start.line ?? resultParameter?.source?.start.line;
  if (result.message) diagnostics.push({ code: `RESULT_${result.status.toUpperCase().replace('-', '_')}`, severity: 'error', message: result.message, parameterId: resultId, selectionId: resultSelectionId, sourceLine: resultSourceLine });
  if (constraint?.message) diagnostics.push({ code: `CONSTRAINT_${constraint.status.toUpperCase().replace('-', '_')}`, severity: constraint.status === 'fail' ? 'warning' : 'error', message: constraint.message, parameterId: resultId, selectionId: resultSelectionId, sourceLine: constraint.source?.start.line ?? resultSourceLine });
  if (!usage && (result.calculationUsageId || result.calculationDefinitionId)) diagnostics.push({ code: 'CALC_USAGE_MISSING', severity: 'warning', message: `${result.name} 声明了计算调用但未解析到 CalculationUsage`, parameterId: resultId, selectionId: resultSelectionId, sourceLine: resultSourceLine });
  if (usage && !(usage.bindings ?? []).length) diagnostics.push({ code: 'CALC_BINDING_MISSING', severity: 'warning', message: `${usage.definitionName} 调用缺少形式参数与实际参数绑定`, parameterId: resultId, selectionId: resultSelectionId, sourceLine: usage.source?.start.line ?? resultSourceLine });
  const uniqueDiagnostics = diagnostics.filter((item, index, all) =>
    all.findIndex((candidate) => candidate.code === item.code && candidate.message === item.message && candidate.parameterId === item.parameterId) === index);

  return { analysis, result, usage, definition, constraint, bindings, definitionParameters, diagnostics: uniqueDiagnostics };
}

export function projectSemanticDependencyGraph(
  snapshot: ModelSnapshot,
  analysisSnapshot: AnalysisSnapshot | undefined,
  rootId: string,
): SemanticDependencyProjection | undefined {
  if (!analysisSnapshot || !snapshot.calculationModel) return undefined;
  const analysis = analysisSnapshot.analyses.find((item) => item.results.some((result) => result.parameterId === rootId));
  const root = analysis?.results.find((item) => item.parameterId === rootId);
  if (!analysis || !root) return undefined;
  const model = snapshot.calculationModel;
  const parameters = new Map(model.parameters.map((item) => [item.id, item]));
  const results = new Map(analysis.results.map((item) => [item.parameterId, item]));
  const usage = model.usages.find((item) => item.id === root.calculationUsageId || item.resultParameterId === rootId);
  const definition = model.definitions.find((item) => item.id === (usage?.definitionId ?? root.calculationDefinitionId));
  const constraint = analysis.constraints.find((item) => item.resultParameterId === rootId);
  const definitionFormulas = definition
    ? [...definition.localParameterIds, ...definition.outputParameterIds].flatMap((id) => {
      const parameter = parameters.get(id);
      return parameter?.expression ? [{ name: parameter.name, expression: parameter.expression }] : [];
    })
    : [];
  const invocationFormula = root.formula ?? '';
  const nodes: SemanticDependencyNode[] = [];
  const edges: SemanticDependencyEdge[] = [];
  const diagnostics: string[] = [];
  const addNode = (node: SemanticDependencyNode) => { if (!nodes.some((item) => item.id === node.id)) nodes.push(node); };
  const parameterNode = (parameterId: string, kind: 'input' | 'target', column: number) => {
    const parameter = parameters.get(parameterId);
    const result = results.get(parameterId);
    const name = parameter?.name ?? result?.name ?? parameterName(snapshot, parameterId);
    const display = result?.value == null ? parameter?.expression ?? result?.message ?? '未计算' : `${formatAnalysisNumber(result.value)} ${result.unit ?? ''}`.trim();
    addNode({ id: `parameter:${parameterId}`, semanticId: parameterId, selectionId: parameter?.elementId ?? parameterId, kind, name, subtitle: display, column, status: result?.status });
  };

  const resultNodeId = `parameter:${rootId}`;
  addNode({ id: resultNodeId, semanticId: rootId, selectionId: rootId, kind: 'result', name: root.name,
    subtitle: `${formatAnalysisNumber(root.value)} ${root.unit ?? ''}`.trim(), column: 3, status: constraint?.status ?? root.status });

  if (usage) {
    const usageNodeId = `usage:${usage.id}`;
    addNode({ id: usageNodeId, semanticId: usage.id, selectionId: rootId, kind: 'usage', name: usage.definitionName,
      subtitle: invocationFormula || `计算调用 · ${(usage.bindings ?? []).length} 个绑定`, formula: invocationFormula, column: 2 });
    edges.push({ id: `${usageNodeId}->${resultNodeId}`, source: usageNodeId, target: resultNodeId, label: '返回', kind: 'return' });
    if (definition) {
      const definitionNodeId = `definition:${definition.id}`;
      const primaryFormula = definitionFormulas[definitionFormulas.length - 1];
      addNode({ id: definitionNodeId, semanticId: definition.id, selectionId: definition.elementId, kind: 'definition', name: definition.name,
        subtitle: primaryFormula ? `${primaryFormula.name} = ${primaryFormula.expression}` : `计算定义 · ${definition.inputParameterIds.length} 个输入`,
        formula: definitionFormulas.map((item) => `${item.name} = ${item.expression}`).join('; '), column: 1 });
      edges.push({ id: `${definitionNodeId}->${usageNodeId}`, source: definitionNodeId, target: usageNodeId, label: '定义调用', kind: 'definition' });
    } else diagnostics.push(`计算调用 ${usage.id} 没有解析到 CalculationDefinition`);
    const bindings = usage.bindings ?? [];
    if (!bindings.length) diagnostics.push(`计算调用 ${usage.id} 没有形式参数与实际参数绑定；需要 Runtime 0.8.0 或更高版本`);
    for (const binding of bindings) {
      if (!binding.actualParameterId) {
        diagnostics.push(`实际参数 ${binding.actualReference ?? binding.index} 没有解析到模型元素`);
        continue;
      }
      parameterNode(binding.actualParameterId, 'input', 0);
      edges.push({
        id: `binding:${usage.id}:${binding.index}`,
        source: `parameter:${binding.actualParameterId}`,
        target: usageNodeId,
        label: binding.formalParameterName ?? `输入 ${binding.index + 1}`,
        kind: 'binding',
      });
    }
  } else {
    diagnostics.push(`${root.name} 不是 CalculationUsage 的输出，显示参数级直接依赖`);
    for (const edge of analysis.dependencies.filter((item) => item.sourceParameterId === rootId)) {
      parameterNode(edge.targetParameterId, 'input', 0);
      edges.push({ id: `dependency:${rootId}:${edge.targetParameterId}`, source: `parameter:${edge.targetParameterId}`, target: resultNodeId, label: '依赖', kind: 'binding' });
    }
  }

  if (constraint) {
    const constraintNodeId = `constraint:${constraint.constraintId}`;
    addNode({ id: constraintNodeId, semanticId: constraint.constraintId, selectionId: constraint.constraintId, kind: 'constraint', name: constraint.name,
      subtitle: `${constraint.comparison ?? ''} ${formatAnalysisNumber(constraint.target)} ${constraint.unit ?? ''}`.trim(), column: 4, status: constraint.status });
    edges.push({ id: `${resultNodeId}->${constraintNodeId}`, source: resultNodeId, target: constraintNodeId, label: '校核', kind: 'check' });
    if (constraint.targetParameterId && constraint.targetParameterId !== rootId) {
      parameterNode(constraint.targetParameterId, 'target', 3);
      edges.push({ id: `target:${constraint.targetParameterId}:${constraint.constraintId}`, source: `parameter:${constraint.targetParameterId}`, target: constraintNodeId, label: '目标', kind: 'target' });
    }
  } else diagnostics.push(`${root.name} 没有关联约束`);

  const relatedGraphIds = new Set([rootId, ...root.inputParameterIds, ...(usage?.actualParameterIds ?? []), ...(constraint?.inputParameterIds ?? [])]);
  for (const item of analysisSnapshot.diagnostics.filter((candidate) => relatedGraphIds.has(candidate.parameterId))) {
    diagnostics.push(`${analysisStatusLabel(results.get(item.parameterId)?.status ?? root.status)}：${item.message}`);
  }
  if (root.message) diagnostics.push(`${analysisStatusLabel(root.status)}：${root.message}`);

  return { analysis, root, nodes, edges, definitionFormulas, invocationFormula, diagnostics: [...new Set(diagnostics)] };
}
