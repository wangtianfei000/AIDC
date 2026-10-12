import { describe, expect, it } from 'vitest';
import * as THREE from 'three';
import type { Model3DManifest } from '@aidc/model3d-contracts';
import { detectInterferences, pairId, type CollisionProxy } from './collision';

const manifest = {
  collision: { enabledComponentIds: ['a', 'b'], tolerance: 0.002, allowedContacts: [] },
} as unknown as Model3DManifest;
const proxy = (componentId: string, min: number, max: number, bodyId = componentId): CollisionProxy => ({
  componentId, bodyId, box: new THREE.Box3(new THREE.Vector3(min, 0, 0), new THREE.Vector3(max, 1, 1)),
});

describe('V3 collision preview', () => {
  it('reports penetration and captures the pose', () => {
    const result = detectInterferences([proxy('a', 0, 1), proxy('b', .5, 1.5)], manifest, { knee: .2 });
    expect(result[0]).toMatchObject({ status: 'WARNING', precision: 'MESH_PREVIEW', commonVolume: .5, pose: { knee: .2 } });
  });
  it('reports a near-gap but ignores a clear gap and same rigid body', () => {
    expect(detectInterferences([proxy('a', 0, 1), proxy('b', 1.001, 2)], manifest)).toHaveLength(1);
    expect(detectInterferences([proxy('a', 0, 1), proxy('b', 1.01, 2)], manifest)).toHaveLength(0);
    expect(detectInterferences([proxy('a', 0, 1, 'body'), proxy('b', .5, 2, 'body')], manifest)).toHaveLength(0);
  });
  it('classifies configured assembly contact separately', () => {
    const configured = { ...manifest, collision: { ...manifest.collision!, allowedContacts: [{ componentA: 'b', componentB: 'a', reason: 'joint contact' }] } };
    expect(detectInterferences([proxy('a', 0, 1), proxy('b', .5, 2)], configured)[0]).toMatchObject({ status: 'ALLOWED_CONTACT', message: 'joint contact' });
    expect(pairId('b', 'a')).toBe('a::b');
  });
});
