import { useMemo, useState } from 'react';
import { ChevronDown, FolderTree, GitBranch, Grid3X3, Search, Table2, X } from 'lucide-react';
import { RequirementTable, RequirementTree, TraceabilityGraph, VerificationMatrix } from '../components/Views';
import type { RequirementFilters } from '../model/types';
import { buildRequirements, filterRequirements, uniqueValues } from './requirements';
import type { BrowserViewContext, BrowserViewDefinition } from './viewRegistry';

const emptyFilters: RequirementFilters = {
  query: '', type: '', status: '', discipline: '', variant: '', coverage: '',
};

function RequirementView({ mode, ...context }: BrowserViewContext & { mode: 'table' | 'tree' | 'trace' | 'matrix' }) {
  const [filters, setFilters] = useState<RequirementFilters>(emptyFilters);
  const records = useMemo(() => buildRequirements(context.snapshot, context.query), [context.snapshot, context.query]);
  const visible = useMemo(() => filterRequirements(records, filters), [records, filters]);
  return <div className="plugin-view">
    <RequirementFilterBar records={records} filters={filters} onChange={setFilters} onReset={() => setFilters(emptyFilters)} />
    <div className="plugin-content">
      {mode === 'table' && <RequirementTable records={visible} selectedId={context.selectedId} onSelect={context.onSelect} />}
      {mode === 'tree' && <RequirementTree records={visible} query={context.query} selectedId={context.selectedId} onSelect={context.onSelect} />}
      {mode === 'trace' && <TraceabilityGraph snapshot={context.snapshot} query={context.query} records={visible} selectedId={context.selectedId} onSelect={context.onSelect} />}
      {mode === 'matrix' && <VerificationMatrix snapshot={context.snapshot} query={context.query} records={visible} selectedId={context.selectedId} onSelect={context.onSelect} />}
    </div>
  </div>;
}

function RequirementFilterBar({ records, filters, onChange, onReset }: {
  records: ReturnType<typeof buildRequirements>;
  filters: RequirementFilters;
  onChange: (value: RequirementFilters) => void;
  onReset: () => void;
}) {
  const selects: { key: keyof RequirementFilters; label: string; values: string[] }[] = [
    { key: 'type', label: '全部类型', values: uniqueValues(records, 'requirementType') },
    { key: 'status', label: '全部状态', values: uniqueValues(records, 'status') },
    { key: 'discipline', label: '全部专业', values: uniqueValues(records, 'discipline') },
    { key: 'variant', label: '全部变体', values: uniqueValues(records, 'variant') },
    { key: 'coverage', label: '全部验证', values: uniqueValues(records, 'coverage') },
  ];
  const active = Object.values(filters).filter(Boolean).length;
  return <div className="filter-bar plugin-toolbar">
    <div className="search-box"><Search size={16} /><input value={filters.query} onChange={(event) => onChange({ ...filters, query: event.target.value })} placeholder="搜索 ID、名称或正文…" /></div>
    {selects.map((item) => <label className="select-wrap" key={item.key}><select value={filters[item.key]} onChange={(event) => onChange({ ...filters, [item.key]: event.target.value })}><option value="">{item.label}</option>{item.values.map((value) => <option key={value} value={value}>{value}</option>)}</select><ChevronDown size={14} /></label>)}
    {active > 0 && <button className="clear-filter" onClick={onReset}><X size={14} />清除 {active}</button>}
  </div>;
}

const countRequirements = (snapshot: BrowserViewContext['snapshot']) =>
  snapshot.elements.filter((item) => item.kind === 'RequirementDefinition' || item.kind === 'RequirementUsage').length;

export const browserViews: BrowserViewDefinition[] = [
  { id: 'requirements.table', label: '需求表格', hint: '字段与覆盖状态', group: '需求浏览', icon: Table2, component: (props) => <RequirementView {...props} mode="table" />, badge: countRequirements },
  { id: 'requirements.tree', label: '需求树', hint: '层级与分组', group: '需求浏览', icon: FolderTree, component: (props) => <RequirementView {...props} mode="tree" /> },
  { id: 'requirements.trace', label: '追溯关系', hint: '一跳关系图', group: '需求浏览', icon: GitBranch, component: (props) => <RequirementView {...props} mode="trace" /> },
  { id: 'requirements.matrix', label: '验证矩阵', hint: '需求与验证', group: '需求浏览', icon: Grid3X3, component: (props) => <RequirementView {...props} mode="matrix" /> },
];
