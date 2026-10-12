import { describe, expect, it } from 'vitest';
import type { AnalysisSnapshot, ModelSnapshot } from '../model/types';
import { analysisStatusLabel, dependencyClosure, formatAnalysisNumber, isAnalysisFailureStatus, projectCalculationDetail, projectParameterTable, projectSemanticDependencyGraph } from './parameterAnalysis';

const snapshot = {
  schemaVersion: '0.1', document: { uri: 'memory://c4.sysml', contentHash: 'x', parser: { name: 'p', baseline: 'b', capabilityVersion: '0.7.0' } },
  elements: [], relationships: [], diagnostics: [], capabilities: {},
  calculationModel: { version: '0.1', definitions: [
    { id: 'definition', elementId: 'definition', name: 'SumMass', inputParameterIds: ['formal'], outputParameterIds: ['output'], localParameterIds: [], source: null },
  ], usages: [
    { id: 'usage', analysisId: 'a', definitionId: 'definition', definitionName: 'SumMass', resultParameterId: 'result', arguments: [{ kind: 'reference', path: 'robot.input' }], actualParameterIds: ['input'], bindings: [
      { index: 0, formalParameterId: 'formal', formalParameterName: 'itemMass', actualParameterId: 'input', actualReference: 'robot.input', argument: { kind: 'reference', path: 'robot.input' } },
    ], source: null },
  ], analyses: [], constraints: [], parameters: [
    { id: 'input', elementId: 'input', name: 'itemMass', ownerId: 'robot', role: 'local', typeName: 'ISQ::mass', unit: 'g', expression: '816.8[g]', expressionAst: {}, source: null },
    { id: 'formal', elementId: 'formal', name: 'itemMass', ownerId: 'definition', role: 'input', typeName: 'ISQ::mass', unit: null, expression: '', expressionAst: {}, source: null },
    { id: 'output', elementId: 'output', name: 'total', ownerId: 'definition', role: 'output', typeName: 'ISQ::mass', unit: null, expression: 'itemMass', expressionAst: {}, source: null },
    { id: 'result', elementId: 'result', name: 'mass', ownerId: 'a', role: 'result', typeName: 'ISQ::mass', unit: null, expression: 'sum', expressionAst: {}, source: null },
    { id: 'limit', elementId: 'limit', name: 'maximumMass', ownerId: 'robot', role: 'local', typeName: 'ISQ::mass', unit: 'g', expression: '800[g]', expressionAst: {}, source: null },
  ] },
  viewModel: { version: '0.1', definitions: [], renderings: [
    { id: 'rendering', elementId: 'rendering', name: 'r', renderingKind: 'element-table', renderingType: 'asElementTable', source: null,
      columns: [
        { id: 'c1', name: 'parameterColumn', key: 'parameterColumn', label: '分析参数', columnView: 'columnView', source: null },
        { id: 'c2', name: 'statusColumn', key: 'statusColumn', label: '判定结果', columnView: 'columnView', source: null },
      ] },
  ], views: [{ id: 'view', elementId: 'view', name: 'table', definitionId: 'd', definitionReference: 'D', renderingId: 'rendering', renderingReference: 'r', exposedElementIds: ['result'], exposedReferences: ['P::a::mass'], source: null }] },
} satisfies ModelSnapshot;

const analysis = {
  schemaVersion: '0.1', document: snapshot.document, engine: { name: 'e', version: '0.1' }, diagnostics: [], capabilities: {}, analyses: [{
    analysisId: 'a', name: 'analysis', subjectId: null, subjectReference: 'robot', evaluationOrder: ['result'],
    dependencies: [{ sourceParameterId: 'result', targetParameterId: 'input' }],
    results: [
      { parameterId: 'input', name: 'itemMass', typeName: 'ISQ::mass', status: 'computed', formula: '816.8[g]', formulaAst: {}, dimension: 'M^1', value: 816.8, unit: 'g', inputParameterIds: ['input'], source: null },
      { parameterId: 'limit', name: 'maximumMass', typeName: 'ISQ::mass', status: 'computed', formula: '800[g]', formulaAst: {}, dimension: 'M^1', value: 800, unit: 'g', inputParameterIds: ['limit'], source: null },
      { parameterId: 'result', name: 'mass', typeName: 'ISQ::mass', status: 'computed', formula: 'sum', formulaAst: {}, dimension: 'M^1', value: 816.8, unit: 'g', inputParameterIds: ['input'], calculationUsageId: 'usage', calculationDefinitionId: 'definition', source: null },
    ],
    constraints: [{ constraintId: 'k', name: 'massLimit', expression: 'mass<=maximumMass', expressionAst: {}, status: 'fail', actual: 816.8, target: 800, margin: -16.8, unit: 'g', resultParameterId: 'result', targetParameterId: 'limit', inputParameterIds: ['result', 'limit'], source: null }],
  }],
} satisfies AnalysisSnapshot;

describe('parameter analysis projection', () => {
  it('uses SysML expose order, column order and constraint result', () => {
    const projection = projectParameterTable(snapshot, analysis)!;
    expect(projection.columns.map((item) => item.label)).toEqual(['分析参数', '判定结果']);
    expect(projection.rows[0]).toMatchObject({ parameterId: 'result', value: 816.8, target: 800, margin: -16.8, status: 'fail', role: '分析结果' });
  });

  it('builds a dependency closure without duplicates', () => {
    expect(dependencyClosure(analysis.analyses[0], 'result')).toEqual(['result', 'input']);
  });

  it('projects input, definition, usage, result, target and constraint semantics', () => {
    const graph = projectSemanticDependencyGraph(snapshot, analysis, 'result')!;
    expect(graph.nodes.map((item) => item.kind)).toEqual(['result', 'usage', 'definition', 'input', 'constraint', 'target']);
    expect(graph.edges.map((item) => [item.kind, item.label])).toEqual([
      ['return', '返回'], ['definition', '定义调用'], ['binding', 'itemMass'], ['check', '校核'], ['target', '目标'],
    ]);
    expect(graph.definitionFormulas).toEqual([{ name: 'total', expression: 'itemMass' }]);
    expect(graph.invocationFormula).toBe('sum');
    expect(graph.nodes.find((item) => item.kind === 'definition')?.formula).toBe('total = itemMass');
    expect(graph.nodes.find((item) => item.kind === 'usage')?.formula).toBe('sum');
    expect(graph.diagnostics).toEqual([]);
  });

  it('projects calculation identity, bindings, parameter roles and constraint detail', () => {
    const detail = projectCalculationDetail(snapshot, analysis, 'result')!;
    expect(detail.definition).toMatchObject({ id: 'definition', name: 'SumMass' });
    expect(detail.usage).toMatchObject({ id: 'usage', resultParameterId: 'result' });
    expect(detail.bindings).toEqual([expect.objectContaining({
      formalParameterName: 'itemMass', formalTypeName: 'ISQ::mass', actualParameterName: 'itemMass', actualExpression: '816.8[g]', actualValue: 816.8, actualUnit: 'g',
    })]);
    expect(detail.definitionParameters.map((item) => [item.name, item.roleLabel])).toEqual([['itemMass', '输入'], ['total', '返回值']]);
    expect(detail.constraint).toMatchObject({ name: 'massLimit', status: 'fail', targetParameterId: 'limit' });
    expect(detail.diagnostics).toEqual([]);
  });

  it('associates calculation diagnostics and translates statuses', () => {
    const withDiagnostic: AnalysisSnapshot = { ...analysis, diagnostics: [
      { code: 'ANALYSIS_UNIT_MISMATCH', severity: 'error', message: '单位不兼容', parameterId: 'input' },
      { code: 'UNRELATED', severity: 'warning', message: '无关诊断', parameterId: 'other' },
    ] };
    expect(projectCalculationDetail(snapshot, withDiagnostic, 'result')?.diagnostics).toEqual([
      expect.objectContaining({ code: 'ANALYSIS_UNIT_MISMATCH', message: '单位不兼容' }),
    ]);
    expect(analysisStatusLabel('computed')).toBe('已计算');
    expect(analysisStatusLabel('unit-error')).toBe('单位错误');
    expect(analysisStatusLabel('fail')).toBe('未通过');
  });

  it('keeps five abnormal states empty, localized, diagnosable and isolated from healthy results', () => {
    const failures = [
      ['missing', 'missing-input', '输入不存在'],
      ['unit', 'unit-error', '质量与时间量纲不兼容'],
      ['unsupported', 'unsupported', '计算定义无法解析'],
      ['cycle', 'cycle', '计算依赖存在循环'],
      ['runtime', 'runtime-error', '除数为零'],
    ] as const;
    const source = (line: number) => ({ uri: 'memory://exceptions.sysml', start: { line, column: 9, offset: line * 10 }, end: { line, column: 30, offset: line * 10 + 21 } });
    const exceptionSnapshot = structuredClone(snapshot) as ModelSnapshot;
    exceptionSnapshot.calculationModel!.usages = [];
    exceptionSnapshot.calculationModel!.definitions = [];
    exceptionSnapshot.calculationModel!.parameters = [
      { id: 'healthy', elementId: 'healthy', name: 'healthyResult', ownerId: 'a', role: 'result', typeName: 'Real', unit: null, expression: '42', expressionAst: {}, source: source(10) },
      ...failures.map(([id], index) => ({ id, elementId: id, name: `${id}Result`, ownerId: 'a', role: 'result', typeName: 'Real', unit: null, expression: id, expressionAst: {}, source: source(20 + index) })),
    ];
    exceptionSnapshot.viewModel!.views[0].exposedElementIds = ['healthy', ...failures.map(([id]) => id)];
    const exceptionAnalysis: AnalysisSnapshot = {
      ...analysis,
      diagnostics: failures.map(([id, status, message]) => ({ code: `AIDC_ANALYSIS_${status.toUpperCase().replace('-', '_')}`, severity: 'error', message, parameterId: id })),
      analyses: [{
        ...analysis.analyses[0], constraints: [], dependencies: [], evaluationOrder: ['healthy', ...failures.map(([id]) => id)],
        results: [
          { parameterId: 'healthy', name: 'healthyResult', typeName: 'Real', status: 'computed', formula: '42', formulaAst: {}, dimension: '1', value: 42, unit: null, inputParameterIds: [], source: source(10) },
          ...failures.map(([id, status, message], index) => ({ parameterId: id, name: `${id}Result`, typeName: 'Real', status, formula: id, formulaAst: {}, dimension: '1', message, inputParameterIds: [], source: source(20 + index) })),
        ],
      }],
    };

    const table = projectParameterTable(exceptionSnapshot, exceptionAnalysis)!;
    expect(table.rows[0]).toMatchObject({ name: 'healthyResult', value: 42, status: 'computed' });
    expect(table.rows.slice(1).map((row) => [row.status, row.value])).toEqual(failures.map(([, status]) => [status, undefined]));

    for (const [id, status, message] of failures) {
      expect(isAnalysisFailureStatus(status)).toBe(true);
      expect(analysisStatusLabel(status)).not.toBe(status);
      const detail = projectCalculationDetail(exceptionSnapshot, exceptionAnalysis, id)!;
      expect(detail.result.value).toBeUndefined();
      expect(detail.diagnostics).toEqual(expect.arrayContaining([
        expect.objectContaining({ message, selectionId: id, sourceLine: expect.any(Number) }),
      ]));
      const graph = projectSemanticDependencyGraph(exceptionSnapshot, exceptionAnalysis, id)!;
      expect(graph.nodes.find((node) => node.kind === 'result')).toMatchObject({ status, subtitle: '—' });
      expect(graph.diagnostics.join('；')).toContain(message);
    }
    expect(formatAnalysisNumber(Number.NaN)).toBe('—');
    expect(formatAnalysisNumber(Number.POSITIVE_INFINITY)).toBe('—');
    expect(formatAnalysisNumber(Number.NEGATIVE_INFINITY)).toBe('—');
  });
});
