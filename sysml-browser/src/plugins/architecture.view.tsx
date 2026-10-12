import { useEffect, useMemo, useState } from 'react';
import { Box, Boxes, Cable, ChevronRight, CircleDot, Network, RefreshCw } from 'lucide-react';
import { CapabilityNotice, Empty } from '../components/Views';
import { layoutDiagram } from '../diagram/layout';
import { applyGraphicMetadata } from '../diagram/metadata';
import { MaxGraphArchitecture } from '../diagram/MaxGraphArchitecture';
import { projectInterconnection } from '../diagram/projection';
import type { DiagramDirection, DiagramModel } from '../diagram/model';
import type { ModelElement } from '../model/types';
import { architectureChildren, buildArchitectureModel } from './architecture';
import type { BrowserViewContext, BrowserViewDefinition } from './viewRegistry';

function ArchitectureTreeView(context: BrowserViewContext) {
  const model = useMemo(() => buildArchitectureModel(context.snapshot, context.query), [context.snapshot, context.query]);
  const render = (ownerId: string | null, depth: number): React.ReactNode =>
    architectureChildren(context.query, ownerId).map((element) => <div key={element.id}>
      <button className={`architecture-tree-row ${context.selectedId === element.id ? 'selected' : ''}`} style={{ paddingLeft: 18 + depth * 24 }} onClick={() => context.onSelect(element.id)}>
        <ChevronRight size={14} className={architectureChildren(context.query, element.id).length ? '' : 'leaf-chevron'} />
        <ArchitectureIcon kind={element.kind} />
        <span><strong>{element.name ?? '未命名元素'}</strong><small>{element.kind}{element.properties.typeName ? ` · ${String(element.properties.typeName)}` : ''}</small></span>
      </button>
      {render(element.id, depth + 1)}
    </div>);
  return <div className="architecture-tree view-scroll">
    <ArchitectureSummary model={model} />
    {model.elements.length ? render(null, 0) : <Empty title="没有架构元素" detail="当前模型未包含 Part、Port、Interface 或 Connector。" />}
  </div>;
}

function ArchitectureDiagramView(context: BrowserViewContext) {
  const model = useMemo(() => buildArchitectureModel(context.snapshot, context.query), [context.snapshot, context.query]);
  const defaultContainer = model.containers.reduce<ModelElement | undefined>((best, item) => {
    const score = context.query.children(item.id).filter((child) => ['PartUsage', 'PortUsage'].includes(child.kind)).length;
    const bestScore = best ? context.query.children(best.id).filter((child) => ['PartUsage', 'PortUsage'].includes(child.kind)).length : -1;
    return score > bestScore ? item : best;
  }, undefined);
  const contextContainer = model.containers.find((item) => item.id === context.viewContextId);
  const container = contextContainer ?? defaultContainer;
  const projection = useMemo(() => {
    if (!container) return undefined;
    const semantic = projectInterconnection(context.snapshot, {
      viewId: `architecture.interconnection.${container.id}`,
      contextElementId: container.id,
      selection: { mode: 'owned', depth: 1 },
    });
    const metadata = applyGraphicMetadata(context.snapshot, semantic.model);
    return { model: metadata.model, diagnostics: [...semantic.diagnostics, ...metadata.diagnostics] };
  }, [container, context.snapshot]);
  const [direction, setDirection] = useState<DiagramDirection>('RIGHT');
  const [layoutRevision, setLayoutRevision] = useState(0);
  const [laidOut, setLaidOut] = useState<DiagramModel>();
  const [layoutError, setLayoutError] = useState<string>();
  useEffect(() => {
    let canceled = false;
    setLaidOut(undefined);
    setLayoutError(undefined);
    if (!projection) return () => { canceled = true; };
    layoutDiagram(projection.model, direction)
      .then((diagram) => { if (!canceled) setLaidOut(diagram); })
      .catch((error: unknown) => { if (!canceled) setLayoutError(error instanceof Error ? error.message : String(error)); });
    return () => { canceled = true; };
  }, [projection, direction, layoutRevision]);
  const hasParts = projection?.model.nodes.some((node) => node.role === 'part');
  return <div className="architecture-diagram">
    <div className="architecture-diagram-toolbar">
      <div><strong>结构上下文</strong><span>选择一个部件定义查看其组成和连接</span></div>
      <div className="architecture-layout-controls">
        {projection && <span className="architecture-layout-source">{projection.model.layoutSource === 'graphic-metadata' ? '参考图形元数据' : projection.model.layoutSource === 'mixed' ? '元数据 + ELK' : 'ELK 自动布局'}</span>}
        <select aria-label="结构上下文" value={container?.id ?? ''} onChange={(event) => context.openView('architecture.diagram', event.target.value)}>
          {model.containers.map((item) => <option key={item.id} value={item.id}>{item.qualifiedName ?? item.name ?? item.kind}</option>)}
        </select>
        <select aria-label="布局方向" value={direction} onChange={(event) => setDirection(event.target.value as DiagramDirection)}>
          <option value="RIGHT">从左到右</option>
          <option value="DOWN">从上到下</option>
        </select>
        <button type="button" onClick={() => setLayoutRevision((value) => value + 1)} title="重新运行 ELK 布局"><RefreshCw size={14} />重新布局</button>
      </div>
    </div>
    {!!projection?.diagnostics.length && <CapabilityNotice text={`图形投影诊断：${projection.diagnostics.join('；')}`} />}
    {layoutError && <CapabilityNotice text={`ELK 布局失败：${layoutError}`} />}
    {laidOut && hasParts ? <MaxGraphArchitecture diagram={laidOut} selectedId={context.selectedId} onSelect={context.onSelect} canOpenNode={(id) => model.containers.some((item) => item.id === id)} onOpenNode={(id) => context.openView('architecture.diagram', id)} />
      : projection && hasParts && !layoutError ? <div className="architecture-layout-loading"><RefreshCw size={18} />正在计算布局…</div>
        : <Empty title="没有可绘制的组成" detail="请选择含 PartUsage 的部件定义或用法。" />}
  </div>;
}

function ArchitectureSummary({ model }: { model: ReturnType<typeof buildArchitectureModel> }) {
  return <div className="architecture-summary">
    <div><Boxes /><span><strong>{model.parts.length}</strong>部件</span></div>
    <div><CircleDot /><span><strong>{model.ports.length}</strong>端口</span></div>
    <div><Network /><span><strong>{model.interfaces.length}</strong>接口</span></div>
    <div><Cable /><span><strong>{model.connections.length}</strong>连接</span></div>
  </div>;
}

function ArchitectureIcon({ kind }: { kind: string }) {
  return kind.startsWith('Port') ? <CircleDot size={16} /> : kind.startsWith('Interface') || kind.startsWith('Connector') ? <Network size={16} /> : <Box size={16} />;
}

const architectureCount = (snapshot: BrowserViewContext['snapshot']) => snapshot.elements.filter((item) => /^Part|^Port|^Interface|^Connector/.test(item.kind)).length;

export const browserViews: BrowserViewDefinition[] = [
  { id: 'architecture.tree', label: '架构树', hint: '部件、端口与接口', group: '架构浏览', icon: Boxes, component: ArchitectureTreeView, badge: architectureCount },
  { id: 'architecture.diagram', label: '架构图', hint: '组成与连接', group: '架构浏览', icon: Network, component: ArchitectureDiagramView },
];
