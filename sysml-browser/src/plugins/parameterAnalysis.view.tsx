import { useEffect, useMemo, useRef, useState } from 'react';
import { AlertTriangle, Calculator, ChevronDown, GitFork, Maximize2, Minus, Plus, Search, Table2, X } from 'lucide-react';
import { CapabilityNotice, Empty } from '../components/Views';
import {
  analysisStatusLabel, elementTableViews, formatAnalysisNumber, isAnalysisFailureStatus, projectCalculationDetail, projectParameterTable,
  projectSemanticDependencyGraph, type CalculationDetailParameter, type ParameterTableRow,
} from './parameterAnalysis';
import type { BrowserViewContext, BrowserViewDefinition } from './viewRegistry';

// P2 基线保留详情投影与组件，当前产品界面仅展示参数表和按需打开的计算依赖图。
const calculationDetailVisible = false;

function ParameterTableView(context: BrowserViewContext) {
  const views = useMemo(() => elementTableViews(context.snapshot), [context.snapshot]);
  const selectedView = views.some((item) => item.id === context.selectedId) ? context.selectedId : undefined;
  const projection = useMemo(
    () => projectParameterTable(context.snapshot, context.analysis, selectedView),
    [context.snapshot, context.analysis, selectedView],
  );
  const resultIds = useMemo(() => new Set(projection?.rows.map((row) => row.parameterId) ?? []), [projection]);
  const initialRowId = context.selectedId && resultIds.has(context.selectedId) ? context.selectedId : projection?.rows[0]?.parameterId;
  const [activeRowId, setActiveRowId] = useState(initialRowId);
  useEffect(() => {
    if (context.selectedId && resultIds.has(context.selectedId)) setActiveRowId(context.selectedId);
    else if (!activeRowId || !resultIds.has(activeRowId)) setActiveRowId(projection?.rows[0]?.parameterId);
  }, [activeRowId, context.selectedId, projection, resultIds]);
  const [query, setQuery] = useState('');
  const [status, setStatus] = useState('');
  const [sort, setSort] = useState<{ column: string; direction: 'asc' | 'desc' }>({ column: '', direction: 'asc' });
  if (context.analysisError) return <Empty title="参数分析不可用" detail={context.analysisError} />;
  if (!context.analysis) return <Empty title="正在等待计算结果" detail="Runtime 需要支持 /analyze 和 AnalysisSnapshot 0.1。" />;
  if (!projection) return <Empty title="没有可显示的参数表" detail="模型需要声明 GridView、asElementTable、columnView 和 expose。" />;
  const visible = projection.rows
    .filter((row) => (!query.trim() || `${row.name} ${row.formula}`.toLowerCase().includes(query.toLowerCase())) && (!status || row.status === status));
  if (sort.column) visible.sort((left, right) => compareCell(left, right, sort.column) * (sort.direction === 'asc' ? 1 : -1));
  const selectedRow = projection.rows.find((row) => row.parameterId === activeRowId) ?? projection.rows[0];
  const statuses = [...new Set(projection.rows.map((row) => row.status))];
  const abnormalRows = projection.rows.filter((row) => isAnalysisFailureStatus(row.status));
  const healthyRows = projection.rows.filter((row) => !isAnalysisFailureStatus(row.status));
  return <div className="analysis-view">
    <div className="analysis-toolbar">
      <div className="search-box"><Search size={16} /><input value={query} onChange={(event) => setQuery(event.target.value)} placeholder="搜索参数或公式…" />{query && <button onClick={() => setQuery('')}><X size={13} /></button>}</div>
      <label className="select-wrap"><select value={projection.view.id} onChange={(event) => context.openView('analysis.table', event.target.value)}>{views.map((item) => <option key={item.id} value={item.id}>{item.name}</option>)}</select><ChevronDown size={14} /></label>
      <label className="select-wrap"><select value={status} onChange={(event) => setStatus(event.target.value)}><option value="">全部状态</option>{statuses.map((item) => <option key={item} value={item}>{statusLabel(item)}</option>)}</select><ChevronDown size={14} /></label>
      <button className="analysis-dependency-button" onClick={() => context.openView('analysis.dependencies', selectedRow?.parameterId)} disabled={!selectedRow}><GitFork size={15} />计算依赖</button>
    </div>
    {!!abnormalRows.length && <div className="analysis-exception-summary" role="status"><AlertTriangle size={16} /><div><strong>{abnormalRows.length} 项计算未产生有效数值</strong><span>{healthyRows.length} 项结果仍正常显示；异常不会用 0、NaN 或类型名代替。</span></div><div>{statuses.filter(isAnalysisFailureStatus).map((item) => <span key={item} className={item}>{analysisStatusLabel(item)} {projection.rows.filter((row) => row.status === item).length}</span>)}</div></div>}
    {projection.diagnostics.map((item) => <CapabilityNotice key={item} text={item} />)}
    <div className="analysis-table-wrap">
      <table className="analysis-table"><thead><tr>{projection.columns.map((column) => <th key={column.id}><button onClick={() => setSort((current) => ({ column: column.name, direction: current.column === column.name && current.direction === 'asc' ? 'desc' : 'asc' }))}>{column.label}<span>{sort.column === column.name ? (sort.direction === 'asc' ? '↑' : '↓') : '↕'}</span></button></th>)}</tr></thead><tbody>{visible.map((row) => <tr key={row.parameterId} className={activeRowId === row.parameterId ? 'selected' : ''} onClick={() => { setActiveRowId(row.parameterId); context.selectInView(row.parameterId); }}>{projection.columns.map((column) => <td key={column.id}>{renderCell(column.name, row)}</td>)}</tr>)}</tbody></table>
      {!visible.length && <Empty title="没有匹配的分析结果" detail="请调整搜索词或状态筛选。" />}
    </div>
    {calculationDetailVisible && selectedRow && <AnalysisDetail row={selectedRow} context={context} />}
  </div>;
}

function compareCell(left: ParameterTableRow, right: ParameterTableRow, column: string) {
  const value = (row: ParameterTableRow): string | number => {
    if (column === 'parameterColumn') return row.name;
    if (column === 'roleColumn') return row.role;
    if (column === 'currentValueColumn') return typeof row.value === 'number' ? row.value : Number.NEGATIVE_INFINITY;
    if (column === 'unitColumn') return row.unit ?? '';
    if (column === 'targetColumn') return typeof row.target === 'number' ? row.target : Number.NEGATIVE_INFINITY;
    if (column === 'marginColumn') return typeof row.margin === 'number' ? row.margin : Number.NEGATIVE_INFINITY;
    if (column === 'statusColumn') return row.status;
    if (column === 'sourceColumn') return row.sourceLine ?? Number.NEGATIVE_INFINITY;
    return '';
  };
  const a = value(left); const b = value(right);
  return typeof a === 'number' && typeof b === 'number' ? a - b : String(a).localeCompare(String(b), 'zh-CN');
}

function renderCell(column: string, row: ParameterTableRow) {
  if (column === 'parameterColumn') return <strong>{row.name}</strong>;
  if (column === 'roleColumn') return row.role;
  if (column === 'currentValueColumn') return <span className="analysis-number">{formatAnalysisNumber(row.value)}</span>;
  if (column === 'unitColumn') return row.unit ?? '—';
  if (column === 'targetColumn') return formatAnalysisNumber(row.target);
  if (column === 'marginColumn') return <span className={typeof row.margin === 'number' && row.margin < 0 ? 'negative' : 'positive'}>{formatAnalysisNumber(row.margin)}</span>;
  if (column === 'statusColumn') return <StatusBadge status={row.status} />;
  if (column === 'sourceColumn') return row.sourceLine ? `第 ${row.sourceLine} 行` : '—';
  return '—';
}

function AnalysisDetail({ row, context }: { row: ParameterTableRow; context: BrowserViewContext }) {
  const detail = projectCalculationDetail(context.snapshot, context.analysis, row.parameterId);
  if (!detail) return <div className="analysis-detail"><CapabilityNotice text="当前结果缺少 CalculationModel 或 AnalysisSnapshot 语义，无法生成计算详情。" /></div>;
  const internals = detail.definitionParameters.filter((item) => item.role !== 'input');
  const constraint = detail.constraint;
  return <div className="analysis-detail">
    <div className="analysis-detail-head"><div><span>计算详情</span><strong>{row.name}</strong><small>{detail.analysis.name}</small></div><StatusBadge status={row.status} /></div>
    <div className="analysis-detail-grid"><div><span>当前值</span><strong>{formatAnalysisNumber(row.value)} {row.value == null ? '' : row.unit ?? ''}</strong></div><div><span>目标</span><strong>{formatAnalysisNumber(row.target)} {row.target == null ? '' : row.unit ?? ''}</strong></div><div><span>余量</span><strong className={typeof row.margin === 'number' && row.margin < 0 ? 'negative' : 'positive'}>{formatAnalysisNumber(row.margin)} {row.margin == null ? '' : row.unit ?? ''}</strong></div><div><span>量纲</span><strong>{row.dimension || '—'}</strong></div></div>
    {isAnalysisFailureStatus(row.status) && <section className={`analysis-interruption ${row.status}`}><AlertTriangle size={15} /><div><span>计算在此中断</span><strong>{analysisStatusLabel(row.status)}</strong><p>{row.result.message ?? 'Runtime 未返回进一步说明。'}</p></div></section>}
    <div className="analysis-identity-grid">
      <button disabled={!detail.definition} onClick={() => detail.definition && context.onSelect(detail.definition.elementId)}><span>计算定义</span><strong>{detail.definition?.name ?? '未解析'}</strong><small>{detail.definition ? `${detail.definition.inputParameterIds.length} 个输入` : '缺少定义'}</small></button>
      <button disabled={!detail.usage} onClick={() => detail.usage && context.openView('core.source', row.parameterId)}><span>计算调用</span><strong>{detail.usage?.definitionName ?? '未解析'}</strong><small>{detail.usage ? `${detail.bindings.length} 项绑定 · 第 ${detail.usage.source?.start.line ?? '—'} 行` : '缺少调用'}</small></button>
      <button onClick={() => context.onSelect(row.parameterId)}><span>分析输出</span><strong>{row.name}</strong><small>{formatAnalysisNumber(row.value)} {row.unit ?? ''}</small></button>
      <button disabled={!constraint} onClick={() => constraint && context.onSelect(constraint.constraintId)}><span>约束校核</span><strong>{constraint?.name ?? '未声明'}</strong><small>{constraint ? `${constraint.comparison ?? ''} ${formatAnalysisNumber(constraint.target)} ${constraint.unit ?? ''} · ${analysisStatusLabel(constraint.status)}` : '没有关联约束'}</small></button>
    </div>
    <section className="analysis-formula"><span>调用表达式</span><code>{row.formula || '—'}</code></section>
    <section className="analysis-bindings"><span>输入绑定 · {detail.bindings.length}</span>{detail.bindings.length ? <div className="analysis-binding-table">{detail.bindings.map((binding) => <div key={`${detail.usage?.id}-${binding.index}`} className="analysis-binding-row"><button disabled={!binding.formalParameterId} onClick={() => binding.formalParameterId && context.onSelect(binding.formalParameterId)}><small>形式参数</small><strong>{binding.formalParameterName}</strong><em>{binding.formalTypeName}</em></button><b>←</b><button disabled={!binding.actualParameterId} onClick={() => binding.actualParameterId && context.onSelect(binding.actualParameterId)} title={binding.actualReference}><small>实际参数</small><strong>{binding.actualParameterName}</strong><em>{binding.actualValue == null ? binding.actualExpression || '—' : `${formatAnalysisNumber(binding.actualValue)} ${binding.actualUnit ?? ''}`}{binding.actualStatus ? ` · ${analysisStatusLabel(binding.actualStatus)}` : ''}</em></button></div>)}</div> : <p className="muted-copy">Runtime 未提供形式参数与实际参数绑定。</p>}</section>
    <section className="analysis-parameters"><span>局部量与返回值 · {internals.length}</span><div className="analysis-parameter-list">{internals.map((parameter) => <ParameterChip key={parameter.id} parameter={parameter} onSelect={context.onSelect} />)}{!internals.length && <p className="muted-copy">计算定义没有局部量或返回值投影。</p>}</div></section>
    {constraint && <section className={`analysis-verdict ${constraint.status}`}><span>校核结论</span><strong>{constraintVerdict(constraint, row)}</strong><code>{constraint.expression}</code></section>}
    <section className="analysis-diagnostics"><span>相关诊断 · {detail.diagnostics.length}</span>{detail.diagnostics.length ? <div>{detail.diagnostics.map((item, index) => <button key={`${item.code}-${index}`} className={item.severity} onClick={() => item.sourceLine ? context.openView('core.source', item.selectionId ?? item.parameterId) : item.selectionId && context.onSelect(item.selectionId)}><code>{item.code}</code><strong>{item.message}</strong><small>{item.sourceLine ? `第 ${item.sourceLine} 行 · 点击定位源码` : '无源码位置'}</small></button>)}</div> : <p className="analysis-clean">没有相关计算诊断</p>}</section>
    <div className="analysis-detail-actions"><button onClick={() => context.openView('analysis.dependencies', row.parameterId)}><GitFork size={14} />查看依赖图</button><button onClick={() => context.openView('core.source', row.parameterId)}>定位调用源码</button></div>
  </div>;
}

function ParameterChip({ parameter, onSelect }: { parameter: CalculationDetailParameter; onSelect: (id: string) => void }) {
  return <button onClick={() => onSelect(parameter.elementId)}><span>{parameter.roleLabel}</span><strong>{parameter.name}</strong><small>{parameter.value == null ? parameter.expression || parameter.typeName : `${formatAnalysisNumber(parameter.value)} ${parameter.unit ?? ''}`}{parameter.status ? ` · ${analysisStatusLabel(parameter.status)}` : ''}</small></button>;
}

function constraintVerdict(constraint: NonNullable<ParameterTableRow['constraint']>, row: ParameterTableRow): string {
  const actual = formatAnalysisNumber(constraint.actual ?? row.value);
  const target = formatAnalysisNumber(constraint.target);
  const unit = constraint.unit ?? row.unit ?? '';
  if (constraint.status === 'pass' || constraint.status === 'fail') return `当前值 ${actual} ${unit} ${constraint.status === 'pass' ? '满足' : '不满足'} ${constraint.comparison ?? ''} ${target} ${unit}，余量 ${formatAnalysisNumber(constraint.margin)} ${unit}`;
  return constraint.message ?? `约束无法完成校核：${analysisStatusLabel(constraint.status)}`;
}

function DependencyView(context: BrowserViewContext) {
  const tableProjection = useMemo(() => projectParameterTable(context.snapshot, context.analysis), [context.snapshot, context.analysis]);
  const resultIds = useMemo(() => new Set(tableProjection?.rows.map((item) => item.parameterId) ?? []), [tableProjection]);
  const initialRootId = context.selectedId && resultIds.has(context.selectedId) ? context.selectedId : tableProjection?.rows[0]?.parameterId;
  const [rootId, setRootId] = useState(initialRootId);
  const [zoom, setZoom] = useState(1);
  const canvasRef = useRef<HTMLDivElement>(null);
  useEffect(() => {
    if (context.selectedId && resultIds.has(context.selectedId)) setRootId(context.selectedId);
    else if (!rootId || !resultIds.has(rootId)) setRootId(tableProjection?.rows[0]?.parameterId);
  }, [context.selectedId, resultIds, rootId, tableProjection]);
  if (context.analysisError) return <Empty title="计算依赖不可用" detail={context.analysisError} />;
  if (!tableProjection) return <Empty title="没有计算依赖" detail="请先打开包含 Analysis 和参数 Table View 的模型。" />;
  if (!rootId) return <Empty title="没有分析输出" detail="当前 Analysis 没有可作为依赖根节点的结果。" />;
  const graph = projectSemanticDependencyGraph(context.snapshot, context.analysis, rootId);
  if (!graph) return <Empty title="无法建立计算语义图" detail="当前结果缺少 CalculationModel 或 AnalysisSnapshot 语义。" />;
  const nodeWidth = 228; const nodeHeight = 66; const columnGap = 128; const rowGap = 28;
  const maxDefinitionWidth = 360; const formulaCharacterWidth = 4.1;
  const definitionStatements = graph.definitionFormulas.length
    ? graph.definitionFormulas.map((formula) => ({ key: formula.name, text: `${formula.name} = ${formula.expression.replace(/;\s*$/, '')};` }))
    : [{ key: 'empty', text: '未声明局部量或返回值公式；' }];
  const definitionWidth = Math.min(maxDefinitionWidth, Math.max(nodeWidth, 32 + Math.max(...definitionStatements.map((line) => line.text.length)) * formulaCharacterWidth));
  const formulaDisplayLength = Math.max(18, Math.floor((definitionWidth - 28) / formulaCharacterWidth));
  const definitionLines = definitionStatements.flatMap((statement) => wrapFormulaStatement(statement.text, formulaDisplayLength).map((text, index) => ({ key: `${statement.key}-${index}`, text, continuation: index > 0 })));
  const nodeWidthFor = (node: typeof graph.nodes[number]) => node.kind === 'definition' ? definitionWidth : nodeWidth;
  const nodeHeightFor = (node: typeof graph.nodes[number]) => node.kind === 'definition'
    ? Math.max(82, 58 + definitionLines.length * 13)
    : nodeHeight;
  const columns = new Map<number, typeof graph.nodes>();
  for (const node of graph.nodes) columns.set(node.column, [...(columns.get(node.column) ?? []), node]);
  const maxRows = Math.max(1, ...[...columns.values()].map((items) => items.length));
  const contentHeight = maxRows * nodeHeight + (maxRows - 1) * rowGap;
  const definitionLaneHeight = Math.max(112, ...graph.nodes.filter((node) => node.kind === 'definition').map((node) => nodeHeightFor(node) + 34));
  const positions = new Map<string, { x: number; y: number }>();
  for (const [column, nodes] of columns) {
    const definitions = nodes.filter((node) => node.kind === 'definition');
    const flowNodes = nodes.filter((node) => node.kind !== 'definition');
    const columnHeight = flowNodes.length * nodeHeight + Math.max(0, flowNodes.length - 1) * rowGap;
    const startY = 46 + definitionLaneHeight + (contentHeight - columnHeight) / 2;
    const columnX = 32 + column * (nodeWidth + columnGap);
    flowNodes.forEach((node, index) => positions.set(node.id, { x: columnX, y: startY + index * (nodeHeight + rowGap) }));
    let definitionY = 20;
    definitions.forEach((node) => { positions.set(node.id, { x: columnX - (nodeWidthFor(node) - nodeWidth) / 2, y: definitionY }); definitionY += nodeHeightFor(node) + 12; });
  }
  const width = Math.max(760, 64 + 5 * nodeWidth + 4 * columnGap, ...graph.nodes.map((node) => (positions.get(node.id)?.x ?? 0) + nodeWidthFor(node) + 32));
  const height = Math.max(390, contentHeight + definitionLaneHeight + 92);
  const incomingEdges = groupEdges(graph.edges, 'target', positions, 'source');
  const outgoingEdges = groupEdges(graph.edges, 'source', positions, 'target');
  const updateZoom = (next: number) => setZoom(Math.min(2.5, Math.max(.4, Math.round(next * 10) / 10)));
  const fitToCanvas = () => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    updateZoom(Math.min((canvas.clientWidth - 24) / width, (canvas.clientHeight - 24) / height));
    canvas.scrollTo({ left: 0, top: 0 });
  };
  return <div className="dependency-view">
    <div className="dependency-toolbar"><div><strong>计算语义依赖</strong><span>输入参数 → 计算定义与调用 → 输出 → 约束；连线标签为形式参数</span></div><select value={rootId} onChange={(event) => { setRootId(event.target.value); context.openView('analysis.dependencies', event.target.value); }}>{tableProjection.rows.map((row) => <option key={row.parameterId} value={row.parameterId}>{row.name}</option>)}</select><div className="dependency-zoom" role="group" aria-label="图形缩放"><button onClick={() => updateZoom(zoom - .1)} disabled={zoom <= .4} title="缩小"><Minus size={14} /></button><button className="dependency-zoom-value" onClick={() => updateZoom(1)} title="恢复到 100%">{Math.round(zoom * 100)}%</button><button onClick={() => updateZoom(zoom + .1)} disabled={zoom >= 2.5} title="放大"><Plus size={14} /></button><button onClick={fitToCanvas} title="适应窗口"><Maximize2 size={14} /></button></div><button onClick={() => context.openView('analysis.table', tableProjection.view.id)}><Table2 size={14} />返回参数表</button></div>
    {graph.diagnostics.map((item) => <CapabilityNotice key={item} text={item} />)}
    <div className="dependency-legend"><span className="input">输入参数</span><span className="definition">计算定义</span><span className="usage">计算调用</span><span className="result">输出结果</span><span className="constraint">约束校核</span></div>
    <div ref={canvasRef} className="dependency-canvas" onWheel={(event) => { if (!event.ctrlKey) return; event.preventDefault(); updateZoom(zoom + (event.deltaY < 0 ? .1 : -.1)); }}><div className="dependency-surface" style={{ width: width * zoom, height: height * zoom }}><svg width={width * zoom} height={height * zoom} viewBox={`0 0 ${width} ${height}`} role="img" aria-label={`计算语义依赖图，当前缩放 ${Math.round(zoom * 100)}%`}>
      <defs><marker id="semantic-arrow" markerWidth="8" markerHeight="8" refX="7" refY="4" orient="auto"><path d="M 0 0 L 8 4 L 0 8 z" /></marker></defs>
      {graph.edges.map((edge) => {
        const from = positions.get(edge.source); const to = positions.get(edge.target);
        if (!from || !to) return null;
        if (edge.kind === 'definition') {
          const sourceNode = graph.nodes.find((node) => node.id === edge.source);
          const targetNode = graph.nodes.find((node) => node.id === edge.target);
          const startX = from.x + (sourceNode ? nodeWidthFor(sourceNode) : nodeWidth) / 2; const startY = from.y + (sourceNode ? nodeHeightFor(sourceNode) : nodeHeight);
          const endX = to.x + (targetNode ? nodeWidthFor(targetNode) : nodeWidth) / 2; const endY = to.y;
          const middleY = (startY + endY) / 2;
          return <g key={edge.id} className="semantic-edge definition"><path d={`M ${startX} ${startY} C ${startX} ${middleY}, ${endX} ${middleY}, ${endX} ${endY}`} markerEnd="url(#semantic-arrow)" /><text x={(startX + endX) / 2} y={middleY - 6} textAnchor="middle">{edge.label}</text></g>;
        }
        const outgoing = outgoingEdges.get(edge.source) ?? [edge];
        const incoming = incomingEdges.get(edge.target) ?? [edge];
        const sourceNode = graph.nodes.find((node) => node.id === edge.source);
        const targetNode = graph.nodes.find((node) => node.id === edge.target);
        const startX = from.x + (sourceNode ? nodeWidthFor(sourceNode) : nodeWidth);
        const startY = edgePortY(from.y, outgoing.indexOf(edge), outgoing.length, sourceNode ? nodeHeightFor(sourceNode) : nodeHeight);
        const endX = to.x;
        const endY = edgePortY(to.y, incoming.indexOf(edge), incoming.length, targetNode ? nodeHeightFor(targetNode) : nodeHeight);
        const middle = (startX + endX) / 2;
        const fanIn = incoming.length > 1;
        const labelX = fanIn ? startX + 12 : middle;
        const labelY = fanIn ? startY - 6 : (startY + endY) / 2 - 6;
        return <g key={edge.id} className={`semantic-edge ${edge.kind}`}><path d={`M ${startX} ${startY} C ${middle} ${startY}, ${middle} ${endY}, ${endX} ${endY}`} markerEnd="url(#semantic-arrow)" /><text x={labelX} y={labelY} textAnchor={fanIn ? 'start' : 'middle'}>{edge.label}</text></g>;
      })}
      {graph.nodes.map((node) => {
        const point = positions.get(node.id)!;
        const currentWidth = nodeWidthFor(node);
        const currentHeight = nodeHeightFor(node);
        const subtitle = `${node.subtitle}${node.status ? ` · ${statusLabel(node.status)}` : ''}`;
        return <g key={node.id} transform={`translate(${point.x} ${point.y})`} className={`dependency-node semantic-${node.kind} ${node.status ? `status-${node.status}` : ''} ${node.selectionId && context.selectedId === node.selectionId ? 'selected' : ''}`} onClick={() => node.selectionId && context.onSelect(node.selectionId)}>
          <title>{node.name}{'\n'}{node.formula || subtitle}</title>
          <rect width={currentWidth} height={currentHeight} rx="10" />
          {node.status && isAnalysisFailureStatus(node.status) && <circle cx={currentWidth - 12} cy="12" r="5" className="dependency-interruption" />}
          <text x="14" y="18" className="dependency-kind">{nodeKindLabel(node.kind)}</text>
          <text x="14" y="38" className="dependency-name">{shortText(node.name, 30)}</text>
          {node.kind === 'definition' ? <>
            <line x1="14" x2={currentWidth - 14} y1="48" y2="48" className="dependency-node-divider" />
            {definitionLines.map((line, index) => <text key={line.key} x={line.continuation ? 21 : 14} y={65 + index * 13} className={`dependency-formula-line ${line.continuation ? 'continuation' : ''}`}>{line.text}</text>)}
          </> : <text x="14" y="55" className="dependency-value">{shortText(subtitle, 36)}</text>}
        </g>;
      })}
    </svg></div></div>
  </div>;
}

function nodeKindLabel(kind: string) { return ({ input: '输入参数', target: '目标参数', definition: 'CALC DEF', usage: 'CALC USAGE', result: '分析输出', constraint: '约束' } as Record<string, string>)[kind] ?? kind; }
function shortText(value: string, length: number) { return value.length > length ? `${value.slice(0, length - 1)}…` : value; }
function wrapFormulaStatement(value: string, maxLength: number): string[] {
  if (value.length <= maxLength) return [value];
  const lines: string[] = []; let rest = value;
  while (rest.length > maxLength) {
    const minimum = Math.floor(maxLength * .55);
    const operatorGroups = [['+', '-'], [','], ['*', '/'], [')']];
    let splitAt = -1;
    for (const operators of operatorGroups) {
      const candidates = operators.map((operator) => rest.lastIndexOf(operator, maxLength)).filter((index) => index >= minimum);
      if (candidates.length) { splitAt = Math.max(...candidates) + 1; break; }
    }
    if (splitAt < 0) splitAt = maxLength;
    lines.push(rest.slice(0, splitAt)); rest = rest.slice(splitAt);
  }
  if (rest) lines.push(rest);
  return lines;
}

function groupEdges<T extends { source: string; target: string }>(edges: T[], groupBy: 'source' | 'target', positions: Map<string, { x: number; y: number }>, orderBy: 'source' | 'target'): Map<string, T[]> {
  const groups = new Map<string, T[]>();
  for (const edge of edges) groups.set(edge[groupBy], [...(groups.get(edge[groupBy]) ?? []), edge]);
  for (const [id, items] of groups) groups.set(id, items.sort((left, right) => (positions.get(left[orderBy])?.y ?? 0) - (positions.get(right[orderBy])?.y ?? 0)));
  return groups;
}

function edgePortY(nodeY: number, index: number, count: number, nodeHeight: number): number {
  return count <= 1 ? nodeY + nodeHeight / 2 : nodeY + 11 + index * ((nodeHeight - 22) / (count - 1));
}

function StatusBadge({ status }: { status: string }) { return <span className={`analysis-status ${status}`}>{analysisStatusLabel(status)}</span>; }
const statusLabel = analysisStatusLabel;

const countTables = (snapshot: BrowserViewContext['snapshot']) => elementTableViews(snapshot).length;

export const browserViews: BrowserViewDefinition[] = [
  { id: 'analysis.table', label: '参数分析表', hint: 'SysML View 与计算结果', group: '参数分析', icon: Calculator, component: ParameterTableView, badge: countTables },
  { id: 'analysis.dependencies', label: '计算语义依赖', hint: '输入、计算、输出与约束', group: '参数分析', icon: GitFork, component: DependencyView },
];
