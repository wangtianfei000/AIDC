import { useCallback, useEffect, useMemo, useRef, useState } from 'react';
import {
  AlertTriangle, ArrowLeft, ArrowRight, Box, Braces, ChevronDown, ChevronRight, CircleDot, ExternalLink,
  FileCode2, FileSearch, GitBranch, Info, MoreHorizontal, Network, RefreshCw,
  Search, Server, Settings2, ShieldCheck, SlidersHorizontal, Table2, Upload, X,
} from 'lucide-react';
import { ModelQuery } from './model/query';
import { visibleInspectorProperties } from './model/propertyVisibility';
import type { AnalysisResult, AnalysisSnapshot, Diagnostic, ModelElement, ModelSnapshot, ModelSource, RequirementRecord } from './model/types';
import {
  canMoveNavigation, createNavigationState, currentNavigationEntry, moveNavigationState,
  navigateNavigationState, resetNavigationState, selectInCurrentNavigation, type NavigationEntry,
  updateCurrentNavigationViewState,
} from './navigation/history';
import { analysisStatusLabel } from './plugins/parameterAnalysis';
import { buildRequirements, isTraceRelationship } from './plugins/requirements';
import { browserViews, defaultViewId } from './plugins/viewRegistry';
import { ParserClient, ParserRuntimeError, type RuntimeHealth } from './services/parserClient';

interface ContextMenuState { x: number; y: number; element: ModelElement }
export default function App() {
  const [runtimeUrl, setRuntimeUrl] = useState(() => new URLSearchParams(window.location.search).get('runtime') ?? localStorage.getItem('aidc-runtime-url') ?? 'http://127.0.0.1:47321');
  const [health, setHealth] = useState<RuntimeHealth>();
  const [source, setSource] = useState<ModelSource>();
  const [snapshot, setSnapshot] = useState<ModelSnapshot>();
  const [analysis, setAnalysis] = useState<AnalysisSnapshot>();
  const [analysisError, setAnalysisError] = useState<string>();
  const [staleSnapshot, setStaleSnapshot] = useState(false);
  const [busy, setBusy] = useState(false);
  const [runtimeError, setRuntimeError] = useState<string>();
  const [view, setView] = useState(defaultViewId);
  const [selectedId, setSelectedId] = useState<string>();
  const [diagnosticsOpen, setDiagnosticsOpen] = useState(false);
  const [settingsOpen, setSettingsOpen] = useState(false);
  const navigation = useRef(createNavigationState());
  const [, setNavigationRevision] = useState(0);
  const fileInput = useRef<HTMLInputElement>(null);
  const sessionInputLoaded = useRef(false);
  const client = useMemo(() => new ParserClient(runtimeUrl.replace(/\/$/, '')), [runtimeUrl]);
  const query = useMemo(() => (snapshot ? new ModelQuery(snapshot) : undefined), [snapshot]);
  const requirements = useMemo(() => (snapshot && query ? buildRequirements(snapshot, query) : []), [snapshot, query]);
  const selected = query?.byId.get(selectedId ?? '');
  const selectedRequirement = requirements.find((item) => item.element.id === selectedId);
  const activeView = browserViews.find((item) => item.id === view) ?? browserViews[0];

  const checkHealth = useCallback(async () => {
    try { const result = await client.health(); setHealth(result); setRuntimeError(undefined); return true; }
    catch (error) { setHealth(undefined); setRuntimeError(error instanceof Error ? error.message : 'Parser Runtime 不可用'); return false; }
  }, [client]);
  useEffect(() => { void checkHealth(); }, [checkHealth]);

  const resetNavigation = useCallback((entry: NavigationEntry) => {
    navigation.current = resetNavigationState(entry);
    setView(entry.viewId); setSelectedId(entry.selectedId);
    setNavigationRevision((value) => value + 1);
  }, []);
  const navigateTo = useCallback((viewId: string, elementId?: string) => {
    const next = navigateNavigationState(navigation.current, { viewId, selectedId: elementId, viewContextId: elementId });
    if (next === navigation.current) return;
    navigation.current = next;
    setView(viewId); setSelectedId(elementId);
    setNavigationRevision((value) => value + 1);
  }, []);
  const selectElement = useCallback((elementId: string) => {
    setSelectedId(elementId);
  }, []);
  const selectInView = useCallback((elementId: string) => {
    navigation.current = selectInCurrentNavigation(navigation.current, elementId);
    setSelectedId(elementId);
  }, []);
  const moveHistory = (offset: -1 | 1) => {
    const next = moveNavigationState(navigation.current, offset);
    const entry = currentNavigationEntry(next);
    if (next === navigation.current || !entry) return;
    navigation.current = next;
    setView(entry.viewId); setSelectedId(entry.selectedId);
    setNavigationRevision((value) => value + 1);
  };
  const saveViewState = useCallback((viewState: Record<string, unknown>) => {
    navigation.current = updateCurrentNavigationViewState(navigation.current, viewState);
  }, []);

  const parseSource = useCallback(async (nextSource: ModelSource, preferredView?: string) => {
    setBusy(true); setRuntimeError(undefined); setAnalysis(undefined); setAnalysisError(undefined); setSource(nextSource);
    try {
      const result = await client.parse(nextSource.uri, nextSource.text);
      let nextAnalysis: AnalysisSnapshot | undefined;
      let nextAnalysisError: string | undefined;
      try { nextAnalysis = await client.analyze(nextSource.uri, nextSource.text); }
      catch (error) { nextAnalysisError = error instanceof Error ? error.message : '参数分析失败'; }
      const nextQuery = new ModelQuery(result);
      const firstRequirement = buildRequirements(result, nextQuery)[0];
      const firstAnalysisView = result.viewModel?.views.find((item) =>
        result.viewModel?.renderings.some((rendering) => rendering.id === item.renderingId && rendering.renderingKind === 'element-table'),
      );
      setSnapshot(result); setAnalysis(nextAnalysis); setAnalysisError(nextAnalysisError); setStaleSnapshot(false);
      resetNavigation({
        selectedId: firstRequirement?.element.id ?? firstAnalysisView?.exposedElementIds[0] ?? firstAnalysisView?.id ?? result.elements[0]?.id,
        viewId: preferredView ?? (firstRequirement ? 'requirements.table' : firstAnalysisView ? 'analysis.table' : 'architecture.tree'),
      });
      setDiagnosticsOpen(result.elements.length === 0 && result.diagnostics.some((item) => item.severity === 'error'));
    } catch (error) { setRuntimeError(error instanceof ParserRuntimeError ? error.message : '解析失败'); setStaleSnapshot(Boolean(snapshot)); }
    finally { setBusy(false); }
  }, [client, resetNavigation, snapshot]);
  useEffect(() => {
    if (sessionInputLoaded.current) return;
    sessionInputLoaded.current = true;
    void fetch('./session/input').then(async (response) => {
      if (!response.ok) return;
      const input = await response.json() as { name: string; uri: string; text: string };
      await parseSource({ name: input.name, uri: input.uri, text: input.text });
    }).catch(() => undefined);
  }, [parseSource]);

  const openFile = async (file: File) => {
    if (!file.name.toLowerCase().endsWith('.sysml') && !file.name.toLowerCase().endsWith('.kerml')) { setRuntimeError('请选择 .sysml 或 .kerml 文本文件'); return; }
    await parseSource({ name: file.name, uri: `browser-file:///${encodeURIComponent(file.name)}`, text: await file.text(), file });
  };
  const loadSample = async () => { const response = await fetch('./samples/VehicleMode.sysml'); await parseSource({ name: 'VehicleMode.sysml', uri: 'sample:///VehicleMode.sysml', text: await response.text() }); };
  const loadArchitectureSample = async () => { const response = await fetch('./samples/robot-architecture.sysml'); await parseSource({ name: 'robot-architecture.sysml', uri: 'sample:///robot-architecture.sysml', text: await response.text() }, 'analysis.table'); };
  const loadAnalysisExceptionSample = async () => { const response = await fetch('./samples/analysis-exception-acceptance.sysml'); await parseSource({ name: 'analysis-exception-acceptance.sysml', uri: 'sample:///analysis-exception-acceptance.sysml', text: await response.text() }, 'analysis.table'); };
  const refresh = async () => { if (source) await parseSource({ ...source, text: source.file ? await source.file.text() : source.text }, view); };
  const openElementView = (elementId: string, viewId: string) => navigateTo(viewId, elementId);
  const saveRuntimeUrl = () => { localStorage.setItem('aidc-runtime-url', runtimeUrl.replace(/\/$/, '')); setSettingsOpen(false); void checkHealth(); };
  const errors = (snapshot?.diagnostics.filter((item) => item.severity === 'error').length ?? 0) + (analysis?.diagnostics.filter((item) => item.severity === 'error').length ?? 0);
  const combinedDiagnostics = useMemo<Diagnostic[]>(() => {
    const parameters = new Map((snapshot?.calculationModel?.parameters ?? []).map((item) => [item.id, item]));
    const results = new Map((analysis?.analyses ?? []).flatMap((item) => item.results).map((item) => [item.parameterId, item]));
    return [
      ...(snapshot?.diagnostics ?? []),
      ...(analysis?.diagnostics.map((item) => {
        const parameter = parameters.get(item.parameterId);
        return {
          code: item.code, severity: item.severity, message: item.message,
          source: results.get(item.parameterId)?.source ?? parameter?.source ?? null,
          relatedElementIds: item.parameterId ? [parameter?.elementId ?? item.parameterId] : undefined,
        } satisfies Diagnostic;
      }) ?? []),
    ];
  }, [snapshot, analysis]);
  const warnings = combinedDiagnostics.filter((item) => item.severity === 'warning').length;

  return <div className="app-shell navigator-layout" onDragOver={(event) => event.preventDefault()} onDrop={(event) => { event.preventDefault(); const file = event.dataTransfer.files[0]; if (file) void openFile(file); }}>
    <input ref={fileInput} type="file" accept=".sysml,.kerml,text/plain" hidden onChange={(event) => { const file = event.target.files?.[0]; if (file) void openFile(file); event.target.value = ''; }} />
    <header className="topbar">
      <div className="topbar-brand"><span>A</span><strong>AIDC</strong></div>
      <div className="document-identity"><FileCode2 size={20} /><div><strong>{source?.name ?? '尚未打开模型'}</strong><span>{snapshot ? `${requirements.length} 条需求 · ${snapshot.elements.length} 个元素` : 'SysML v2 只读浏览'}</span></div>{staleSnapshot && <span className="stale-chip">旧快照</span>}</div>
      <div className="topbar-actions"><div className={`runtime-chip ${health ? 'online' : 'offline'}`} title={health?.parserBaseline ?? runtimeError}><CircleDot size={13} /> Runtime {health ? `v${health.runtimeVersion}` : '离线'}</div><button className="ghost-button" onClick={() => void refresh()} disabled={!source || busy}><RefreshCw size={16} className={busy ? 'spin' : ''} />刷新</button><button className="primary-button" onClick={() => fileInput.current?.click()}><Upload size={16} />打开 SysML</button><button className="icon-button" onClick={() => setSettingsOpen(true)} aria-label="Runtime 设置"><Settings2 size={18} /></button></div>
    </header>

    <main className="workspace">
      {!snapshot ? <Welcome busy={busy} error={runtimeError} onOpen={() => fileInput.current?.click()} onSample={() => void loadSample()} onArchitecture={() => void loadArchitectureSample()} onAnalysisExceptions={() => void loadAnalysisExceptionSample()} /> : <>
        <section className="workspace-head"><div className="workspace-title"><div className="view-history"><button type="button" aria-label="后退" title="后退" disabled={!canMoveNavigation(navigation.current, -1)} onClick={() => moveHistory(-1)}><ArrowLeft size={16} /></button><button type="button" aria-label="前进" title="前进" disabled={!canMoveNavigation(navigation.current, 1)} onClick={() => moveHistory(1)}><ArrowRight size={16} /></button></div><div><div className="eyebrow">{activeView?.hint}</div><h1>{activeView?.label}</h1>{selected && <span className="workspace-context">{selected.qualifiedName ?? selected.name}</span>}</div></div><div className="model-stats"><span><strong>{requirements.length}</strong> 需求</span><span><strong>{snapshot.relationships.length}</strong> 关系</span><button className={errors ? 'has-error' : ''} onClick={() => setDiagnosticsOpen(true)}><strong>{errors}</strong> 错误 · {warnings} 警告</button></div></section>
        <section className="view-surface">{busy && <div className="loading-overlay"><RefreshCw className="spin" /><span>正在解析并计算模型…</span></div>}{activeView && query && <activeView.component snapshot={snapshot} analysis={analysis} analysisError={analysisError} query={query} sourceText={source?.text ?? ''} selected={selected} selectedId={selectedId} viewContextId={currentNavigationEntry(navigation.current)?.viewContextId} viewState={currentNavigationEntry(navigation.current)?.viewState} onSelect={selectElement} selectInView={selectInView} openView={(viewId, elementId) => navigateTo(viewId, elementId ?? selectedId)} saveViewState={saveViewState} />}</section>
      </>}
    </main>

    <aside className="model-dock">
      {query && snapshot ? <ModelNavigator query={query} snapshot={snapshot} selectedId={selectedId} activeViewId={view} onSelect={selectElement} onOpenView={openElementView} onShowProperties={selectElement} /> : <div className="navigator-empty"><Network size={28} /><strong>尚未打开模型</strong><span>打开 SysML 文件后在这里浏览模型结构。</span></div>}
      <div className="navigator-footer"><ShieldCheck size={15} /><span>只读浏览，不修改源文件</span></div>
    </aside>
    <aside className="property-inspector"><div className="property-inspector-head"><SlidersHorizontal size={16} /><div><strong>属性查看</strong><span>当前模型元素</span></div></div><Inspector element={selected} requirement={selectedRequirement} analysisResult={analysis?.analyses.flatMap((item) => item.results).find((item) => item.parameterId === selectedId)} analysis={analysis} query={query} onSource={() => navigateTo('core.source', selectedId)} /></aside>

    {snapshot && <DiagnosticsBar diagnostics={combinedDiagnostics} open={diagnosticsOpen} onToggle={() => setDiagnosticsOpen((value) => !value)} onSelect={(diagnostic) => { const id = diagnostic.relatedElementIds?.[0]; if (diagnostic.source) navigateTo('core.source', id ?? selectedId); else if (id) selectElement(id); }} />}
    {settingsOpen && <div className="modal-backdrop" onMouseDown={() => setSettingsOpen(false)}><div className="settings-modal" onMouseDown={(event) => event.stopPropagation()}><div className="modal-head"><div><Server size={20} /><span><strong>Parser Runtime</strong><small>localhost 连接设置</small></span></div><button className="icon-button" onClick={() => setSettingsOpen(false)}><X size={18} /></button></div><label>Runtime URL<input value={runtimeUrl} onChange={(event) => setRuntimeUrl(event.target.value)} placeholder="http://127.0.0.1:47321" /></label><p>浏览器只向本机 Runtime 发送当前打开的 SysML 文本。文件不会上传到远程服务。</p><div className="modal-actions"><button className="ghost-button" onClick={() => setSettingsOpen(false)}>取消</button><button className="primary-button" onClick={saveRuntimeUrl}>保存并检测</button></div></div></div>}
  </div>;
}

function ModelNavigator({ query, snapshot, selectedId, activeViewId, onSelect, onOpenView, onShowProperties }: { query: ModelQuery; snapshot: ModelSnapshot; selectedId?: string; activeViewId: string; onSelect: (id: string) => void; onOpenView: (id: string, viewId: string) => void; onShowProperties: (id: string) => void }) {
  const [expanded, setExpanded] = useState(() => new Set<string>());
  const [search, setSearch] = useState('');
  const [menu, setMenu] = useState<ContextMenuState>();
  useEffect(() => { setExpanded(new Set(query.children(null).map((item) => item.id))); }, [query]);
  useEffect(() => { const close = () => setMenu(undefined); window.addEventListener('click', close); return () => window.removeEventListener('click', close); }, []);
  const isNavigable = (element: ModelElement) => !['LibraryElement', 'LibraryPackage'].includes(element.kind);
  const navigableChildren = (ownerId: string | null) => query.children(ownerId).filter(isNavigable);
  const matches = (element: ModelElement) => !search.trim() || `${element.name ?? ''} ${element.kind} ${element.qualifiedName ?? ''}`.toLowerCase().includes(search.trim().toLowerCase());
  const hasMatchingDescendant = (element: ModelElement) => query.descendants(element.id).some((item) => isNavigable(item) && matches(item));
  const toggle = (id: string) => setExpanded((current) => { const next = new Set(current); next.has(id) ? next.delete(id) : next.add(id); return next; });
  const render = (ownerId: string | null, depth: number): React.ReactNode => navigableChildren(ownerId).filter((element) => matches(element) || hasMatchingDescendant(element)).map((element) => {
    const children = navigableChildren(element.id); const isExpanded = expanded.has(element.id) || Boolean(search); const actions = viewsForElement(element, snapshot, query); const primary = actions[0];
    return <div key={element.id}><div className={`navigator-row ${selectedId === element.id ? 'selected' : ''}`} style={{ paddingLeft: 8 + depth * 16 }} onContextMenu={(event) => { event.preventDefault(); onSelect(element.id); setMenu({ x: event.clientX, y: event.clientY, element }); }}>
      <button className="tree-expander" onClick={() => children.length && toggle(element.id)} aria-label={isExpanded ? '折叠节点' : '展开节点'}>{children.length ? <ChevronRight size={14} className={isExpanded ? 'expanded' : ''} /> : <span />}</button>
      <button className="navigator-node" onClick={() => onSelect(element.id)} title={element.qualifiedName ?? element.name ?? element.kind}><KindIcon kind={element.kind} /><span><strong>{element.name ?? '未命名元素'}</strong><small>{element.kind}</small></span></button>
      {primary && <button className={`node-open ${selectedId === element.id && activeViewId === primary.id ? 'active' : ''}`} onClick={() => onOpenView(element.id, primary.id)} title={`打开${primary.label}`} aria-label={`打开 ${element.name ?? element.kind} 的${primary.label}`}><ExternalLink size={14} /></button>}
      <button className="node-menu" onClick={(event) => { event.stopPropagation(); setMenu({ x: event.clientX, y: event.clientY, element }); }} aria-label={`更多 ${element.name ?? element.kind} 视图`}><MoreHorizontal size={15} /></button>
    </div>{isExpanded && render(element.id, depth + 1)}</div>;
  });
  const menuViews = menu ? viewsForElement(menu.element, snapshot, query) : [];
  const visibleCount = snapshot.elements.filter(isNavigable).length;
  return <div className="navigator-body"><div className="navigator-head"><div><strong>模型结构</strong><span>节点右键可选择视图</span></div><em>{visibleCount}</em></div><div className="navigator-search"><Search size={15} /><input value={search} onChange={(event) => setSearch(event.target.value)} placeholder="搜索模型节点…" />{search && <button onClick={() => setSearch('')}><X size={13} /></button>}</div><div className="navigator-tree">{render(null, 0)}</div>{menu && <div className="node-context-menu" style={{ left: Math.min(menu.x, window.innerWidth - 220), top: Math.min(menu.y, window.innerHeight - 320) }} onClick={(event) => event.stopPropagation()}><div className="context-title"><strong>{menu.element.name ?? '未命名元素'}</strong><span>{menu.element.kind}</span></div><button onClick={() => { onShowProperties(menu.element.id); setMenu(undefined); }}><SlidersHorizontal size={15} /><span><strong>属性查看</strong><small>属性、关系与源码位置</small></span></button>{menuViews.map((item) => <button key={item.id} onClick={() => { onOpenView(menu.element.id, item.id); setMenu(undefined); }}><item.icon size={15} /><span><strong>{item.label}</strong><small>{item.hint}</small></span>{activeViewId === item.id && <CircleDot size={11} />}</button>)}</div>}</div>;
}

function viewsForElement(element: ModelElement, snapshot: ModelSnapshot, query: ModelQuery) {
  const ids: string[] = []; const subtree = [element, ...query.descendants(element.id)]; const hasReq = subtree.some((item) => /^Requirement/.test(item.kind)); const hasArch = subtree.some((item) => /^(Part|Port|Interface|Connector)/.test(item.kind)); const hasAnalysis = subtree.some((item) => ['AnalysisCase', 'ViewUsage'].includes(item.kind)) && Boolean(snapshot.viewModel?.views.length);
  if (/^Requirement/.test(element.kind)) ids.push('requirements.table', 'requirements.tree', 'requirements.trace', 'requirements.matrix');
  else if (/^Part/.test(element.kind)) ids.push('architecture.diagram', 'architecture.tree');
  else if (/^(Port|Interface|Connector)/.test(element.kind)) ids.push('architecture.tree', 'architecture.diagram');
  else if (element.kind === 'AnalysisCase') ids.push('analysis.table', 'analysis.dependencies');
  else if (element.kind === 'ViewUsage' && snapshot.viewModel?.views.some((item) => item.elementId === element.id)) ids.push('analysis.table', 'analysis.dependencies');
  else if (snapshot.calculationModel?.parameters.some((item) => item.elementId === element.id && item.role === 'result')) ids.push('analysis.table', 'analysis.dependencies');
  else if (element.kind === 'Package') { if (hasReq) ids.push('requirements.table', 'requirements.tree', 'requirements.trace', 'requirements.matrix'); if (hasArch) ids.push('architecture.diagram', 'architecture.tree'); if (hasAnalysis) ids.push('analysis.table', 'analysis.dependencies'); }
  if (element.source) ids.push('core.source');
  return [...new Set(ids)].map((id) => browserViews.find((item) => item.id === id)).filter((item): item is NonNullable<typeof item> => Boolean(item && (!item.badge || item.badge(snapshot) !== 0 || item.group === '通用浏览')));
}

function KindIcon({ kind }: { kind: string }) { if (/^Requirement/.test(kind)) return <Table2 size={14} />; if (/^View/.test(kind)) return <Table2 size={14} />; if (/^(Part|Port|Interface|Connector)/.test(kind)) return <Box size={14} />; if (/^(Analysis|Calculation|Rendering)/.test(kind)) return <Braces size={14} />; if (kind === 'Package') return <Network size={14} />; return <Info size={14} />; }

function Inspector({ element, requirement, analysisResult, analysis, query, onSource }: { element?: ModelElement; requirement?: RequirementRecord; analysisResult?: AnalysisResult; analysis?: AnalysisSnapshot; query?: ModelQuery; onSource: () => void }) {
  if (!element || !query) return <div className="inspector-empty"><SlidersHorizontal size={28} /><strong>属性查看</strong><span>从模型导航选择元素</span></div>;
  const related = query.related(element.id);
  const relationships = requirement
    ? [...related.incoming, ...related.outgoing].filter(isTraceRelationship)
    : [...related.incoming, ...related.outgoing].filter((item) => item.kind !== 'RequirementConstraint');
  const properties: Array<[string, unknown]> = requirement
    ? requirement.attributes.map((attribute) => [
        `${attribute.name}${attribute.typeName ? ` : ${attribute.typeName}` : ''}`,
        attribute.value || '未赋值',
      ])
    : visibleInspectorProperties(element, query.children(element.id));
  const relationshipText = (item: typeof relationships[number]) => {
    const outgoing = item.sourceIds.includes(element.id);
    const endpointIds = outgoing ? item.targetIds : item.sourceIds;
    const endpoints = endpointIds
      .map((id) => query.byId.get(id)?.name ?? query.byId.get(id)?.qualifiedName)
      .filter(Boolean)
      .join('、');
    return outgoing
      ? `${item.kind} → ${endpoints || '未解析目标'}`
      : `${endpoints || '未解析来源'} → ${item.kind}`;
  };
  const analysisConstraint = analysis?.analyses.flatMap((item) => item.constraints).find((item) => item.resultParameterId === analysisResult?.parameterId);
  return <div className="inspector-content"><div className="inspector-eyebrow">{element.kind}</div><h2>{element.name ?? '未命名元素'}</h2><code className="qualified-name">{element.qualifiedName ?? element.id}</code>{requirement && <div className="inspector-summary"><div><span>需求 ID</span><strong>{requirement.displayId}</strong></div><div><span>验证覆盖</span><strong>{requirement.coverage === 'unsupported' ? '暂不支持' : requirement.coverage}</strong></div></div>}{analysisResult && <section className="inspector-analysis"><h3>计算结果</h3><div className="inspector-summary"><div><span>当前值</span><strong>{analysisResult.value == null ? '—' : `${Number(analysisResult.value).toLocaleString('zh-CN', { maximumFractionDigits: 6 })} ${analysisResult.unit ?? ''}`}</strong></div><div><span>状态</span><strong>{analysisStatusLabel(analysisConstraint?.status ?? analysisResult.status)}</strong></div>{analysisConstraint && <><div><span>目标</span><strong>{analysisConstraint.target?.toLocaleString('zh-CN', { maximumFractionDigits: 6 })} {analysisConstraint.unit ?? ''}</strong></div><div><span>余量</span><strong>{analysisConstraint.margin?.toLocaleString('zh-CN', { maximumFractionDigits: 6 })} {analysisConstraint.unit ?? ''}</strong></div></>}</div></section>}{requirement?.statement && requirement.statement !== '—' && <section><h3>需求正文</h3><p className="statement-card">{requirement.statement}</p></section>}{requirement && requirement.constraints.length > 0 && <section><h3>需求约束 <span>{requirement.constraints.length}</span></h3><div className="constraint-list">{requirement.constraints.map((constraint, index) => <code key={`${constraint}-${index}`}>{constraint}</code>)}</div></section>}<section><h3>模型属性 <span>{properties.length}</span></h3>{properties.length ? <dl className="property-list">{properties.map(([key, value]) => <div key={key}><dt>{key}</dt><dd>{typeof value === 'object' ? JSON.stringify(value) : String(value)}</dd></div>)}</dl> : <p className="muted-copy">没有可显示的模型属性</p>}</section><section><h3>{requirement ? '追溯关系' : '关系'} <span>{relationships.length}</span></h3><div className="relation-pills">{relationships.map((item) => <span key={item.id}>{relationshipText(item)}</span>)}{!relationships.length && <p className="muted-copy">{requirement ? '没有已解析的需求追溯关系' : '没有已解析关系'}</p>}</div></section><section><h3>源码位置</h3>{element.source ? <button className="source-link" onClick={onSource}><FileCode2 size={16} /><span>第 {element.source.start.line} 行，第 {element.source.start.column} 列</span><ChevronRight size={15} /></button> : <p className="muted-copy">Parser 未提供可靠位置</p>}</section></div>;
}

function Welcome({ busy, error, onOpen, onSample, onArchitecture, onAnalysisExceptions }: { busy: boolean; error?: string; onOpen: () => void; onSample: () => void; onArchitecture: () => void; onAnalysisExceptions: () => void }) { return <div className="welcome"><div className="welcome-graphic"><div className="orbit orbit-a" /><div className="orbit orbit-b" /><FileSearch size={48} /></div><div className="welcome-copy"><span className="welcome-kicker">SYSML V2 · READ ONLY</span><h1>从文本模型，看清机器人需求与架构</h1><p>打开模型后，在左侧导航树选择节点，并进入该节点适用的需求、架构或源码视图。</p>{error && <div className="welcome-error"><AlertTriangle size={17} /><span>{error}<small>请先启动本机 Parser Runtime，或在右上角设置连接地址。</small></span></div>}<div className="welcome-actions"><button className="primary-button large" onClick={onOpen} disabled={busy}><Upload size={18} />选择 SysML 文件</button><button className="ghost-button large" onClick={onSample} disabled={busy}>VehicleMode 示例</button><button className="ghost-button large" onClick={onArchitecture} disabled={busy}>架构示例</button><button className="ghost-button large" onClick={onAnalysisExceptions} disabled={busy}>计算异常示例</button></div><span className="drop-hint">也可以将 .sysml 文件拖到此窗口</span></div><div className="welcome-features"><span><Table2 />需求字段化</span><span><GitBranch />关系可追溯</span><span><ShieldCheck />源码只读</span></div></div>; }

function DiagnosticsBar({ diagnostics, open, onToggle, onSelect }: { diagnostics: Diagnostic[]; open: boolean; onToggle: () => void; onSelect: (diagnostic: Diagnostic) => void }) { const errors = diagnostics.filter((item) => item.severity === 'error').length; const warnings = diagnostics.filter((item) => item.severity === 'warning').length; return <div className={`diagnostics ${open ? 'open' : ''}`}><button className="diagnostics-toggle" onClick={onToggle}><AlertTriangle size={15} /><span>解析诊断</span><em className={errors ? 'error-count' : ''}>{errors} 错误</em><em>{warnings} 警告</em><ChevronDown size={15} /></button>{open && <div className="diagnostics-list">{diagnostics.length ? diagnostics.map((item, index) => <button key={`${item.code}-${index}`} onClick={() => onSelect(item)}><span className={`severity-dot ${item.severity}`} /><code>{item.code}</code><p>{item.message}</p><small>{item.source ? `${item.source.start.line}:${item.source.start.column}` : '无源码位置'}</small></button>) : <div className="diagnostics-clean"><ShieldCheck size={18} />没有解析诊断</div>}</div>}</div>; }
