import { describe, expect, it } from 'vitest';
import { ModelQuery } from '../model/query';
import type { ModelSnapshot } from '../model/types';
import { buildRequirementPackageTree, buildRequirements, filterRequirements } from './requirements';

const snapshot: ModelSnapshot = {
  schemaVersion: '0.1',
  document: {
    uri: 'memory://test.sysml',
    contentHash: 'a'.repeat(64),
    parser: { name: 'test', baseline: 'test', capabilityVersion: '0.1' },
  },
  capabilities: { 'trace.verify': 'unsupported' },
  diagnostics: [],
  relationships: [
    { id: 'typing-limit', kind: 'FeatureTyping', sourceIds: ['limit'], targetIds: [], ownerId: 'limit', source: null, properties: { typeName: 'ISQ::time' } },
    { id: 'typing-status', kind: 'FeatureTyping', sourceIds: ['status'], targetIds: [], ownerId: 'status', source: null, properties: { typeName: 'String' } },
  ],
  elements: [
    { id: 'pkg', kind: 'Package', name: 'Robot', qualifiedName: 'Robot', ownerId: null, source: null, properties: {} },
    { id: 'req', kind: 'RequirementDefinition', name: 'SafeStop', qualifiedName: 'Robot::SafeStop', ownerId: 'pkg', source: null, properties: {} },
    { id: 'doc', kind: 'DocStatement', name: null, qualifiedName: null, ownerId: 'req', source: null, properties: { content: '机器人必须安全停车' } },
    { id: 'constraint', kind: 'RequireStatement', name: null, qualifiedName: null, ownerId: 'req', source: null, properties: { expression: 'stopTime <= 500' } },
    { id: 'limit', kind: 'Feature', name: 'stopTime', qualifiedName: null, ownerId: 'req', source: null, properties: { valueExpression: '500 [ms]' } },
    { id: 'status', kind: 'Feature', name: 'status', qualifiedName: null, ownerId: 'req', source: null, properties: { valueExpression: 'approved' } },
  ],
};

describe('requirements provider', () => {
  it('builds business rows from generic model elements', () => {
    const records = buildRequirements(snapshot, new ModelQuery(snapshot));
    expect(records).toHaveLength(1);
    expect(records[0]).toMatchObject({
      name: 'SafeStop',
      statement: '机器人必须安全停车',
      constraints: ['stopTime <= 500'],
      constraintStatements: ['require constraint (stopTime <= 500)'],
      attributes: [
        { name: 'stopTime', value: '500 [ms]', typeName: 'ISQ::time' },
      ],
      status: 'approved',
      coverage: 'unsupported',
      packagePath: [{ id: 'pkg', name: 'Robot' }],
    });
  });

  it('preserves nested package ownership for hierarchical tables', () => {
    const nestedSnapshot: ModelSnapshot = {
      ...snapshot,
      elements: [
        snapshot.elements[0],
        { id: 'subpkg', kind: 'Package', name: 'Safety', qualifiedName: 'Robot::Safety', ownerId: 'pkg', source: null, properties: {} },
        { ...snapshot.elements[1], ownerId: 'subpkg', qualifiedName: 'Robot::Safety::SafeStop' },
        ...snapshot.elements.slice(2),
      ],
    };
    const records = buildRequirements(nestedSnapshot, new ModelQuery(nestedSnapshot));
    expect(records[0].packagePath.map((item) => item.name)).toEqual(['Robot', 'Safety']);
    expect(buildRequirementPackageTree(records)).toMatchObject([
      {
        id: 'pkg',
        name: 'Robot',
        count: 1,
        records: [],
        children: [{ id: 'subpkg', name: 'Safety', count: 1, records: [{ name: 'SafeStop' }] }],
      },
    ]);
  });

  it('filters rows by text and status', () => {
    const records = buildRequirements(snapshot, new ModelQuery(snapshot));
    expect(filterRequirements(records, { query: '安全', type: '', status: 'approved', discipline: '', variant: '', coverage: '' })).toHaveLength(1);
    expect(filterRequirements(records, { query: '', type: '', status: 'draft', discipline: '', variant: '', coverage: '' })).toHaveLength(0);
  });

  it('inherits attributes and constraints from a requirement definition', () => {
    const usageSnapshot: ModelSnapshot = {
      ...snapshot,
      elements: [
        ...snapshot.elements,
        { id: 'usage', kind: 'RequirementUsage', name: 'safeStop', qualifiedName: 'Robot::safeStop', ownerId: 'pkg', source: null, properties: { typeName: 'SafeStop', declaredShortName: '1' } },
      ],
    };
    const records = buildRequirements(usageSnapshot, new ModelQuery(usageSnapshot));
    const usage = records.find((item) => item.element.id === 'usage');
    expect(usage).toMatchObject({
      displayId: '1',
      constraints: ['stopTime <= 500'],
      constraintStatements: ['require constraint (stopTime <= 500)'],
    });
    expect(usage?.attributes).toContainEqual({ name: 'stopTime', value: '500 [ms]', typeName: 'ISQ::time' });
  });
});
