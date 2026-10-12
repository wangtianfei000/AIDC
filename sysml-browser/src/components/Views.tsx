import { useEffect, useMemo } from 'react';
import {
  AlertCircle,
  CheckCircle2,
  ChevronRight,
  CircleDashed,
  FileWarning,
  LocateFixed,
  Minus,
} from 'lucide-react';
import { ModelQuery } from '../model/query';
import type {
  ModelElement,
  ModelSnapshot,
  RequirementRecord,
} from '../model/types';
import { isTraceRelationship } from '../plugins/requirements';

interface SelectionProps {
  selectedId?: string;
  onSelect: (id: string) => void;
}

const CoverageBadge = ({ coverage }: { coverage: RequirementRecord['coverage'] }) => {
  if (coverage === 'verified')
    return <span className="badge badge-success"><CheckCircle2 size={13} /> 已验证</span>;
  if (coverage === 'uncovered')
    return <span className="badge badge-warning"><AlertCircle size={13} /> 未覆盖</span>;
  return <span className="badge badge-muted"><CircleDashed size={13} /> 暂不支持</span>;
};

export function RequirementTable({
  records,
  selectedId,
  onSelect,
}: { records: RequirementRecord[] } & SelectionProps) {
  return (
    <div className="view-scroll">
      <table className="requirements-table">
        <thead>
          <tr>
            <th className="id-column">ID</th>
            <th className="name-column">名称</th>
            <th className="description-column">描述</th>
            <th className="attributes-column">属性</th>
            <th className="constraints-column">约束</th>
          </tr>
        </thead>
        <tbody>
          {records.map((record) => (
            <tr
              key={record.element.id}
              className={record.element.id === selectedId ? 'selected' : ''}
              onClick={() => onSelect(record.element.id)}
            >
              <td><span className="req-id">{record.displayId}</span></td>
              <td className="requirement-name"><strong>{record.name}</strong></td>
              <td className="requirement-description">{record.statement}</td>
              <td><div className="requirement-lines">{record.attributes.length ? record.attributes.map((attribute) => <code key={attribute.name}>{attribute.name}{attribute.value ? ` = ${attribute.value}` : attribute.typeName ? ` : ${attribute.typeName}` : ''}</code>) : <span>—</span>}</div></td>
              <td><div className="requirement-lines constraint-lines">{record.constraintStatements.length ? record.constraintStatements.map((constraint, index) => <code key={`${constraint}-${index}`}>{constraint}</code>) : <span>—</span>}</div></td>
            </tr>
          ))}
        </tbody>
      </table>
      {!records.length && <Empty title="没有匹配的需求" detail="调整筛选条件，或检查模型中是否包含 RequirementDefinition / RequirementUsage。" />}
    </div>
  );
}

export function RequirementTree({ records, query, selectedId, onSelect }: {
  records: RequirementRecord[];
  query: ModelQuery;
} & SelectionProps) {
  const groups = useMemo(() => {
    const result = new Map<string, RequirementRecord[]>();
    for (const record of records) {
      const values = result.get(record.packageName) ?? [];
      values.push(record);
      result.set(record.packageName, values);
    }
    return result;
  }, [records]);

  return (
    <div className="tree-view view-scroll">
      {[...groups.entries()].map(([name, values]) => (
        <section className="tree-group" key={name}>
          <div className="tree-group-title"><ChevronRight size={16} /> {name}<span>{values.length}</span></div>
          {values.map((record) => {
            const parentRequirement = query.ancestors(record.element.id).find((item) =>
              ['RequirementDefinition', 'RequirementUsage'].includes(item.kind),
            );
            return (
              <button
                className={`tree-requirement ${record.element.id === selectedId ? 'selected' : ''}`}
                key={record.element.id}
                onClick={() => onSelect(record.element.id)}
                style={{ marginLeft: parentRequirement ? 28 : 0 }}
              >
                <span className="tree-rail" />
                <span className="tree-node-icon">R</span>
                <span className="tree-node-copy">
                  <strong>{record.displayId} · {record.name}</strong>
                  <small>{record.requirementType} · {record.status}</small>
                </span>
                <CoverageBadge coverage={record.coverage} />
              </button>
            );
          })}
        </section>
      ))}
      {!records.length && <Empty title="需求树为空" detail="当前模型或筛选结果中没有需求。" />}
    </div>
  );
}

export function TraceabilityGraph({ snapshot, query, records, selectedId, onSelect }: {
  snapshot: ModelSnapshot;
  query: ModelQuery;
  records: RequirementRecord[];
} & SelectionProps) {
  const centerId = selectedId && query.byId.has(selectedId) ? selectedId : records[0]?.element.id;
  const edges = snapshot.relationships.filter(
    (relationship) => isTraceRelationship(relationship) &&
      (!centerId || relationship.sourceIds.includes(centerId) || relationship.targetIds.includes(centerId)),
  );
  const nodeIds = new Set<string>();
  if (centerId) nodeIds.add(centerId);
  for (const edge of edges) [...edge.sourceIds, ...edge.targetIds].forEach((id) => nodeIds.add(id));
  const nodes = [...nodeIds].map((id) => query.byId.get(id)).filter(Boolean) as ModelElement[];
  const positions = new Map<string, { x: number; y: number }>();
  nodes.forEach((node, index) => {
    if (node.id === centerId) positions.set(node.id, { x: 400, y: 220 });
    else {
      const others = Math.max(nodes.length - 1, 1);
      const angle = ((index - (nodes[0]?.id === centerId ? 1 : 0)) / others) * Math.PI * 2 - Math.PI / 2;
      positions.set(node.id, { x: 400 + Math.cos(angle) * 265, y: 220 + Math.sin(angle) * 150 });
    }
  });
  const unsupported = ['trace.satisfy', 'trace.verify', 'trace.derive', 'trace.refine'].filter(
    (key) => snapshot.capabilities[key] === 'unsupported',
  );

  return (
    <div className="graph-view">
      {!!unsupported.length && (
        <CapabilityNotice text={`Parser 尚不支持：${unsupported.map((item) => item.split('.')[1]).join('、')}。图中只显示模型已解析的关系。`} />
      )}
      {nodes.length > 1 && edges.length ? (
        <svg viewBox="0 0 800 440" className="trace-svg" aria-label="需求追溯图">
          <defs>
            <marker id="arrow" markerWidth="8" markerHeight="8" refX="7" refY="4" orient="auto">
              <path d="M0,0 L8,4 L0,8 Z" fill="#8190a8" />
            </marker>
          </defs>
          {edges.flatMap((edge) => edge.sourceIds.flatMap((sourceId) => edge.targetIds.map((targetId) => {
            const source = positions.get(sourceId);
            const target = positions.get(targetId);
            if (!source || !target) return null;
            return <g key={`${edge.id}-${sourceId}-${targetId}`}>
              <line x1={source.x} y1={source.y} x2={target.x} y2={target.y} className="trace-edge" markerEnd="url(#arrow)" />
              <text x={(source.x + target.x) / 2} y={(source.y + target.y) / 2 - 8} className="trace-label">{edge.kind}</text>
            </g>;
          })))}
          {nodes.map((node) => {
            const position = positions.get(node.id)!;
            const requirement = records.find((item) => item.element.id === node.id);
            return <g key={node.id} transform={`translate(${position.x - 78}, ${position.y - 30})`} className="trace-node" onClick={() => onSelect(node.id)}>
              <rect width="156" height="60" rx="12" className={node.id === centerId ? 'center-node' : ''} />
              <text x="14" y="24" className="node-kind">{requirement?.displayId ?? node.kind}</text>
              <text x="14" y="44" className="node-name">{node.name ?? '未命名元素'}</text>
            </g>;
          })}
        </svg>
      ) : (
        <Empty title="当前需求没有可显示的追溯关系" detail="这可能表示模型中没有关系，也可能是 Parser 尚未支持；上方能力提示给出当前边界。" />
      )}
    </div>
  );
}

export function VerificationMatrix({ snapshot, query, records, selectedId, onSelect }: {
  snapshot: ModelSnapshot;
  query: ModelQuery;
  records: RequirementRecord[];
} & SelectionProps) {
  const cases = snapshot.elements.filter((item) => /verification|testcase/i.test(item.kind));
  const supported = snapshot.capabilities['trace.verify'] !== 'unsupported';
  return (
    <div className="matrix-view view-scroll">
      {!supported && <CapabilityNotice text="当前 Parser 不支持 verify 关系。矩阵保留需求行并明确显示“暂不支持”，不会把缺失关系解释为未覆盖。" />}
      <table className="matrix-table">
        <thead><tr><th>需求</th>{cases.length ? cases.map((item) => <th key={item.id}>{item.name ?? item.kind}</th>) : <th>验证覆盖</th>}</tr></thead>
        <tbody>
          {records.map((record) => (
            <tr key={record.element.id} className={selectedId === record.element.id ? 'selected' : ''} onClick={() => onSelect(record.element.id)}>
              <td><strong>{record.displayId}</strong><span>{record.name}</span></td>
              {cases.length ? cases.map((verification) => {
                const linked = (query.outgoing.get(record.element.id) ?? []).some(
                  (edge) => edge.kind.toLowerCase().includes('verify') && edge.targetIds.includes(verification.id),
                );
                return <td key={verification.id}>{linked ? <CheckCircle2 className="matrix-pass" size={18} /> : <Minus className="matrix-empty" size={18} />}</td>;
              }) : <td><CoverageBadge coverage={record.coverage} /></td>}
            </tr>
          ))}
        </tbody>
      </table>
    </div>
  );
}

export function SourceView({ text, selected }: { text: string; selected?: ModelElement }) {
  const lines = text.split('\n');
  const selectedLine = selected?.source?.start.line;
  useEffect(() => {
    if (!selectedLine) return;
    requestAnimationFrame(() => {
      document.getElementById(`source-line-${selectedLine}`)?.scrollIntoView({ block: 'center' });
    });
  }, [selectedLine]);
  return (
    <div className="source-view view-scroll">
      <div className="source-head"><LocateFixed size={16} /> {selectedLine ? `已定位到第 ${selectedLine} 行` : '选择带源码位置的元素可自动定位'}</div>
      <pre>
        {lines.map((line, index) => <div key={index} id={`source-line-${index + 1}`} className={selectedLine === index + 1 ? 'active-line' : ''}>
          <span className="line-number">{index + 1}</span><code>{line || ' '}</code>
        </div>)}
      </pre>
    </div>
  );
}

export function Empty({ title, detail }: { title: string; detail: string }) {
  return <div className="empty-state"><FileWarning size={34} /><strong>{title}</strong><p>{detail}</p></div>;
}

export function CapabilityNotice({ text }: { text: string }) {
  return <div className="capability-notice"><AlertCircle size={16} /><span>{text}</span></div>;
}
