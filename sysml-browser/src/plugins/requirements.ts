import { ModelQuery } from '../model/query';
import type {
  ModelElement,
  ModelRelationship,
  ModelSnapshot,
  RequirementFilters,
  RequirementRecord,
} from '../model/types';

export const REQUIREMENT_KINDS = new Set(['RequirementDefinition', 'RequirementUsage']);
export const TRACE_KINDS = ['derive', 'refine', 'satisfy', 'verify', 'allocate', 'dependency'];

export interface RequirementPackageGroup {
  id: string;
  name: string;
  path: string[];
  records: RequirementRecord[];
  children: RequirementPackageGroup[];
  count: number;
}

const cleanValue = (value: unknown): string => {
  if (value == null) return '';
  const text = String(value).trim();
  return text.replace(/^['"]|['"]$/g, '').replace(/^\{+|\}+$/g, '').trim();
};

const propertyValue = (element: ModelElement): string =>
  cleanValue(
    element.properties.valueExpression ??
      element.properties.defaultValue ??
      element.properties.content ??
      element.properties.expression,
  );

const fieldAliases: Record<string, string[]> = {
  reqId: ['reqid', 'requirementid', 'identifier', 'id'],
  type: ['type', 'category', 'requirementtype', 'class'],
  status: ['status', 'state', 'maturity'],
  discipline: ['discipline', 'domain', 'owner', 'allocation', 'specialty'],
  variant: ['variant', 'applicability', 'configuration'],
};

function fieldFromChildren(query: ModelQuery, requirementId: string, field: keyof typeof fieldAliases) {
  const aliases = fieldAliases[field];
  const match = query
    .descendants(requirementId)
    .find((item) => item.name && aliases.includes(item.name.toLowerCase().replace(/[_-]/g, '')));
  return match ? propertyValue(match) : '';
}

function statementFor(query: ModelQuery, requirement: ModelElement): string {
  const docs = query
    .children(requirement.id)
    .filter((item) => item.kind === 'DocStatement')
    .map((item) => cleanValue(item.properties.content))
    .filter(Boolean);
  return docs[0] ?? '—';
}

function requirementScopes(query: ModelQuery, requirement: ModelElement): ModelElement[] {
  const typeName = cleanValue(requirement.properties.typeName);
  if (!typeName) return [requirement];
  const definition = query.snapshot.elements.find(
    (item) => item.kind === 'RequirementDefinition' && item.name === typeName,
  );
  return definition ? [definition, requirement] : [requirement];
}

function constraintsFor(query: ModelQuery, requirement: ModelElement): string[] {
  return requirementScopes(query, requirement)
    .flatMap((scope) => query.children(scope.id))
    .filter((item) => ['RequireStatement', 'AssertStatement', 'AssumeStatement', 'ConstraintStatement'].includes(item.kind))
    .map((item) => cleanValue(item.properties.expression))
    .filter(Boolean);
}

function constraintStatementsFor(query: ModelQuery, requirement: ModelElement): string[] {
  const keyword: Record<string, string> = {
    RequireStatement: 'require constraint',
    AssertStatement: 'assert constraint',
    AssumeStatement: 'assume constraint',
    ConstraintStatement: 'constraint',
  };
  return requirementScopes(query, requirement)
    .flatMap((scope) => query.children(scope.id))
    .filter((item) => keyword[item.kind])
    .map((item) => ({ prefix: keyword[item.kind], expression: cleanValue(item.properties.expression) }))
    .filter((item) => item.expression)
    .map((item) => `${item.prefix} (${item.expression})`);
}

function featureTypeName(query: ModelQuery, feature: ModelElement): string {
  const typing = (query.outgoing.get(feature.id) ?? []).find((relationship) => relationship.kind === 'FeatureTyping');
  return cleanValue(typing?.properties.typeName);
}

function isStringFeature(query: ModelQuery, feature: ModelElement): boolean {
  const typeName = featureTypeName(query, feature).split('::').pop()?.toLowerCase();
  const valueExpression = String(feature.properties.valueExpression ?? '').trim();
  return typeName === 'string' || /^(["']).*\1$/s.test(valueExpression);
}

function attributesFor(query: ModelQuery, requirement: ModelElement): Array<{ name: string; value: string; typeName: string }> {
  const attributes = requirementScopes(query, requirement)
    .flatMap((scope) => query.children(scope.id))
    .filter((item) => item.kind === 'Feature' && item.name)
    .filter((item) => !isStringFeature(query, item))
    .map((item) => ({ name: item.name!, value: propertyValue(item), typeName: featureTypeName(query, item) }));
  return [...new Map(attributes.map((item) => [item.name, item])).values()];
}

export function isTraceRelationship(relationship: ModelRelationship): boolean {
  const kind = relationship.kind.toLowerCase();
  return TRACE_KINDS.some((name) => kind.includes(name));
}

export function buildRequirements(snapshot: ModelSnapshot, query: ModelQuery): RequirementRecord[] {
  const verifySupported = snapshot.capabilities['trace.verify'] !== 'unsupported';
  return snapshot.elements
    .filter((element) => REQUIREMENT_KINDS.has(element.kind))
    .map((element) => {
      const ancestors = query.ancestors(element.id);
      const packagePath = ancestors
        .filter((item) => item.kind === 'Package')
        .reverse()
        .map((item) => ({ id: item.id, name: item.name ?? '未命名 Package' }));
      const packageOwner = packagePath.at(-1);
      const verifyEdges = (query.outgoing.get(element.id) ?? []).filter((relationship) =>
        relationship.kind.toLowerCase().includes('verify'),
      );
      const typeName = cleanValue(element.properties.typeName);
      return {
        element,
        displayId: fieldFromChildren(query, element.id, 'reqId') || cleanValue(element.properties.declaredShortName) || element.id.slice(2, 10).toUpperCase(),
        name: element.name ?? '未命名需求',
        statement: statementFor(query, element),
        constraints: constraintsFor(query, element),
        constraintStatements: constraintStatementsFor(query, element),
        attributes: attributesFor(query, element),
        requirementType:
          fieldFromChildren(query, element.id, 'type') ||
          typeName ||
          (element.kind === 'RequirementDefinition' ? '需求定义' : '需求实例'),
        status: fieldFromChildren(query, element.id, 'status') || '未标注',
        discipline: fieldFromChildren(query, element.id, 'discipline') || '未分配',
        variant: fieldFromChildren(query, element.id, 'variant') || '全部',
        coverage: !verifySupported ? 'unsupported' : verifyEdges.length ? 'verified' : 'uncovered',
        packageName: packageOwner?.name ?? '未分组',
        packagePath,
      } satisfies RequirementRecord;
    });
}

export function buildRequirementPackageTree(records: RequirementRecord[]): RequirementPackageGroup[] {
  type MutableGroup = RequirementPackageGroup & { childIndex: Map<string, MutableGroup> };
  const roots: MutableGroup[] = [];
  const rootIndex = new Map<string, MutableGroup>();

  for (const record of records) {
    const path = record.packagePath.length
      ? record.packagePath
      : [{ id: '__unassigned__', name: '未分组' }];
    let siblings = roots;
    let index = rootIndex;
    let names: string[] = [];
    let group: MutableGroup | undefined;
    for (const segment of path) {
      names = [...names, segment.name];
      group = index.get(segment.id);
      if (!group) {
        group = { id: segment.id, name: segment.name, path: names, records: [], children: [], count: 0, childIndex: new Map() };
        index.set(segment.id, group);
        siblings.push(group);
      }
      siblings = group.children as MutableGroup[];
      index = group.childIndex;
    }
    group!.records.push(record);
  }

  const finalize = (group: MutableGroup): RequirementPackageGroup => {
    const children = group.children.map((child) => finalize(child as MutableGroup));
    return {
      id: group.id,
      name: group.name,
      path: group.path,
      records: group.records,
      children,
      count: group.records.length + children.reduce((sum, child) => sum + child.count, 0),
    };
  };
  return roots.map(finalize);
}

export function filterRequirements(records: RequirementRecord[], filters: RequirementFilters) {
  const query = filters.query.trim().toLocaleLowerCase();
  return records.filter((record) => {
    if (
      query &&
      ![record.displayId, record.name, record.statement, record.requirementType, ...record.attributes.map((item) => `${item.name} ${item.value}`), ...record.constraintStatements]
        .join(' ')
        .toLocaleLowerCase()
        .includes(query)
    )
      return false;
    if (filters.type && record.requirementType !== filters.type) return false;
    if (filters.status && record.status !== filters.status) return false;
    if (filters.discipline && record.discipline !== filters.discipline) return false;
    if (filters.variant && record.variant !== filters.variant) return false;
    if (filters.coverage && record.coverage !== filters.coverage) return false;
    return true;
  });
}

export function uniqueValues(records: RequirementRecord[], key: keyof RequirementRecord): string[] {
  return [...new Set(records.map((record) => String(record[key])).filter(Boolean))].sort((a, b) =>
    a.localeCompare(b, 'zh-CN'),
  );
}
