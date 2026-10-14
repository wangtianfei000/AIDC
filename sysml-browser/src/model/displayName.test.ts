import { describe, expect, it } from 'vitest';
import type { ModelElement, ModelSnapshot } from './types';
import { elementDisplayName, localizedElementName } from './displayName';

function snapshot(owner: ModelElement, content: string): ModelSnapshot {
  return {
    schemaVersion: '0.1',
    document: { uri: 'memory://labels.sysml', contentHash: 'x', parser: { name: 'test', baseline: 'test', capabilityVersion: 'test' } },
    elements: [owner, { id: 'doc', kind: 'DocStatement', name: null, qualifiedName: null, ownerId: owner.id, source: null, properties: { content } }],
    relationships: [], diagnostics: [], capabilities: {},
  };
}

describe('element display names', () => {
  const owner: ModelElement = { id: 'part', kind: 'PartUsage', name: 'sensingSystem', qualifiedName: 'Robot::sensingSystem', ownerId: null, source: null, properties: {} };

  it('uses a short Chinese doc as the display name', () => {
    const model = snapshot(owner, '感知系统');
    expect(localizedElementName(model, owner)).toBe('感知系统');
    expect(elementDisplayName(model, owner)).toBe('感知系统');
  });

  it('does not turn a long description into a title', () => {
    expect(elementDisplayName(snapshot(owner, '机器人收到停止命令后，应当在规定时间内完成安全停止。'), owner)).toBe('sensingSystem');
  });
});
