import { describe, expect, it } from 'vitest';
import { ModelQuery } from '../model/query';
import type { ModelSnapshot } from '../model/types';
import { componentIdForElement, elementByComponentId, resolveModel3D } from './registry';

const snapshot: ModelSnapshot = {
  schemaVersion: '0.1', document: { uri: 'test', contentHash: 'a'.repeat(64), parser: { name: 'test', baseline: 'test', capabilityVersion: 'test' } },
  capabilities: {}, diagnostics: [], relationships: [],
  elements: [
    { id: 'robot', kind: 'PartDefinition', name: 'JingangNative10', qualifiedName: 'P::JingangNative10', ownerId: null, source: null, properties: {} },
    { id: 'robot-id', kind: 'Feature', name: 'componentId', qualifiedName: null, ownerId: 'robot', source: null, properties: { valueExpression: '"SYS-JG10"' } },
    { id: 'leg', kind: 'PartUsage', name: 'leftLeg', qualifiedName: 'P::JingangNative10::leftLeg', ownerId: 'robot', source: null, properties: {} },
    { id: 'leg-id', kind: 'Feature', name: 'componentId', qualifiedName: null, ownerId: 'leg', source: null, properties: { valueExpression: '"ASM-LEG-L"' } },
    { id: 'configuration', kind: 'PartUsage', name: 'jingangConfiguration', qualifiedName: 'P::jingangConfiguration', ownerId: null, source: null, properties: { typeName: 'JingangNative10' } },
  ],
};

describe('Model3D project registry', () => {
  it('resolves a nested SysML part and builds the reverse selection map', () => {
    const query = new ModelQuery(snapshot);
    expect(componentIdForElement(query.byId.get('leg'), query)).toBe('ASM-LEG-L');
    expect(resolveModel3D(query.byId.get('leg'), query)?.registration.modelId).toBe('SYS-JG10');
    expect(elementByComponentId(query).get('ASM-LEG-L')?.id).toBe('leg');
  });

  it('resolves a typed configuration usage through its part definition', () => {
    const query = new ModelQuery(snapshot);
    const resolved = resolveModel3D(query.byId.get('configuration'), query);
    expect(resolved?.registration.modelId).toBe('SYS-JG10');
    expect(resolved?.contextElement.id).toBe('configuration');
    expect(resolved?.focusComponentId).toBe('SYS-JG10');
  });
});
