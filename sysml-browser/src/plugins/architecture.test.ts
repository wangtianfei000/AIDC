import { describe, expect, it } from 'vitest';
import { ModelQuery } from '../model/query';
import type { ModelSnapshot } from '../model/types';
import { buildArchitectureModel, hasDrawableArchitectureContent } from './architecture';

const snapshot: ModelSnapshot = {
  schemaVersion: '0.1',
  document: { uri: 'memory://architecture.sysml', contentHash: 'b'.repeat(64), parser: { name: 'test', baseline: 'test', capabilityVersion: '0.1' } },
  capabilities: { 'architecture.connection': 'partial' },
  diagnostics: [],
  elements: [
    { id: 'pkg', kind: 'Package', name: 'Robot', qualifiedName: 'Robot', ownerId: null, source: null, properties: {} },
    { id: 'def', kind: 'PartDefinition', name: 'MobileRobot', qualifiedName: 'Robot::MobileRobot', ownerId: 'pkg', source: null, properties: {} },
    { id: 'motor', kind: 'PartUsage', name: 'leftMotor', qualifiedName: 'Robot::MobileRobot::leftMotor', ownerId: 'def', source: null, properties: { typeName: 'DriveMotor' } },
    { id: 'port', kind: 'PortUsage', name: 'commandIn', qualifiedName: 'Robot::MobileRobot::commandIn', ownerId: 'def', source: null, properties: {} },
    { id: 'portOnly', kind: 'PartDefinition', name: 'Controller', qualifiedName: 'Robot::Controller', ownerId: 'pkg', source: null, properties: {} },
    { id: 'controllerPort', kind: 'PortUsage', name: 'bus', qualifiedName: 'Robot::Controller::bus', ownerId: 'portOnly', source: null, properties: {} },
  ],
  relationships: [
    { id: 'connection', kind: 'Connection', sourceIds: ['port'], targetIds: ['motor'], ownerId: 'def', source: null, properties: { ends: ['commandIn', 'leftMotor.command'] } },
  ],
};

describe('architecture provider', () => {
  it('builds parts, ports, connections and diagram containers from the generic snapshot', () => {
    const model = buildArchitectureModel(snapshot, new ModelQuery(snapshot));
    expect(model.parts).toHaveLength(3);
    expect(model.ports).toHaveLength(2);
    expect(model.connections).toHaveLength(1);
    expect(model.containers.map((item) => item.id)).toContain('def');
    expect(model.containers.map((item) => item.id)).not.toContain('portOnly');
    expect(hasDrawableArchitectureContent(new ModelQuery(snapshot), 'def')).toBe(true);
    expect(hasDrawableArchitectureContent(new ModelQuery(snapshot), 'portOnly')).toBe(false);
  });
});
