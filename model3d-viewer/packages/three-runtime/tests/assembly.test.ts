import { describe, expect, it } from 'vitest';
import * as THREE from 'three';
import type { AssemblySequence, Model3DManifest, ProductStructureNode } from '@aidc/model3d-contracts';
import { AssemblyRig, ExplosionRig, interpolateKeyframes } from '../src/assembly';

const manifest = { modelId: 'robot', components: [{ componentId: 'part', label: 'part', nodeName: 'part', bodyId: 'body', geometryStatus: 'test', sourceObject: 'part' }] } as unknown as Model3DManifest;
const sequence = { steps: [{ stepId: 'remove', label: 'remove', componentIds: ['part'], dependsOn: [], pathStatus: 'MANUAL_DEMO', keyframes: [
  { time: 0, position: [0, 0, 0], quaternion: [0, 0, 0, 1] }, { time: 2, position: [2, 0, 0], quaternion: [0, 0, 0, 1] },
] }] } as unknown as AssemblySequence;

describe('V4 assembly rig', () => {
  it('interpolates authored keyframes', () => expect(interpolateKeyframes(sequence.steps[0], .25).position).toEqual([.5, 0, 0]));
  it('resets sequenced parts without accumulated error', () => {
    const scene = new THREE.Group(); const part = new THREE.Mesh(new THREE.BoxGeometry(1, 1, 1)); part.name = 'part'; part.position.set(1, 2, 3); scene.add(part);
    scene.updateMatrixWorld(true); const rig = new AssemblyRig(manifest, scene, new Map([['part', part]]));
    for (let index = 0; index < 20; index += 1) { rig.applySequence(sequence, 0, 1); rig.reset(); }
    expect(part.position.toArray()).toEqual([1, 2, 3]);
    rig.applySequence(sequence, 0, .5); expect(part.position.x).toBeCloseTo(2);
  });
  it('explodes direct CAD product children as rigid groups', () => {
    const scene = new THREE.Group();
    const left = new THREE.Group(); left.name = 'left'; left.position.x = -1; left.add(new THREE.Mesh(new THREE.BoxGeometry(1, 1, 1))); scene.add(left);
    const right = new THREE.Group(); right.name = 'right'; right.position.x = 1; right.add(new THREE.Mesh(new THREE.BoxGeometry(1, 1, 1))); scene.add(right);
    const nodes: ProductStructureNode[] = [
      { nodeId: 'root', label: 'root', nodeType: 'collection', nodeName: 'root', bodyId: 'root' },
      { nodeId: 'left', label: 'left', nodeType: 'collection', parentNodeId: 'root', nodeName: 'left', bodyId: 'left' },
      { nodeId: 'right', label: 'right', nodeType: 'collection', parentNodeId: 'root', nodeName: 'right', bodyId: 'right' },
    ];
    scene.updateMatrixWorld(true);
    const rig = new ExplosionRig(nodes, scene, new Map([['left', left], ['right', right]]));
    expect([...rig.nodeIds].sort()).toEqual(['left', 'right']);
    const plan = rig.layout('root', 'current');
    expect(plan.entries.map((entry) => entry.nodeId)).toEqual(['right', 'left']);
    expect(plan.origin).toEqual([0, 0, 0]);
    expect(plan.entries.find((entry) => entry.nodeId === 'left')?.direction).toEqual([-1, 0, 0]);
    expect(plan.entries.find((entry) => entry.nodeId === 'right')?.direction).toEqual([1, 0, 0]);
    rig.setExplode(1);
    expect(left.position.x).toBeLessThan(-1); expect(right.position.x).toBeGreaterThan(1);
    rig.reset(); expect(left.position.x).toBe(-1); expect(right.position.x).toBe(1);
  });
  it('selects a subassembly scope without moving its parent and avoids parent-child double transforms', () => {
    const scene = new THREE.Group();
    const module = new THREE.Group(); module.name = 'module'; scene.add(module);
    const first = new THREE.Mesh(new THREE.BoxGeometry(1, 1, 1)); first.name = 'first'; first.position.x = -1; module.add(first);
    const second = new THREE.Mesh(new THREE.BoxGeometry(1, 1, 1)); second.name = 'second'; second.position.x = 1; module.add(second);
    const nodes: ProductStructureNode[] = [
      { nodeId: 'root', label: 'root', nodeType: 'collection', nodeName: 'root', bodyId: 'root' },
      { nodeId: 'module', label: 'module', nodeType: 'collection', parentNodeId: 'root', nodeName: 'module', bodyId: 'module' },
      { nodeId: 'first', label: 'first', nodeType: 'part', parentNodeId: 'module', nodeName: 'first', bodyId: 'first' },
      { nodeId: 'second', label: 'second', nodeType: 'part', parentNodeId: 'module', nodeName: 'second', bodyId: 'second' },
    ];
    const rig = new ExplosionRig(nodes, scene, new Map([['module', module], ['first', first], ['second', second]]));
    const plan = rig.layout('module', 'leaves');
    expect(plan.scopeNodeId).toBe('module'); expect([...rig.nodeIds].sort()).toEqual(['first', 'second']);
    rig.setExplode(1);
    expect(module.position.toArray()).toEqual([0, 0, 0]);
    expect(first.position.x).toBeLessThan(-1); expect(second.position.x).toBeGreaterThan(1);
  });
  it('freezes directions in the active plan and scales adaptive spacing deterministically', () => {
    const scene = new THREE.Group(); const part = new THREE.Mesh(new THREE.BoxGeometry(2, 1, 1)); part.position.x = 2; scene.add(part);
    const nodes: ProductStructureNode[] = [
      { nodeId: 'root', label: 'root', nodeType: 'collection', nodeName: 'root', bodyId: 'root' },
      { nodeId: 'part', label: 'part', nodeType: 'part', parentNodeId: 'root', nodeName: 'part', bodyId: 'part' },
    ];
    const rig = new ExplosionRig(nodes, scene, new Map([['part', part]]));
    const first = rig.layout('root', 'current', undefined, 1); const baseDistance = first.entries[0].distance;
    rig.setSpacing(2); expect(rig.plan?.entries[0].distance).toBeCloseTo(baseDistance * 2);
    const direction = rig.getDirection('part'); rig.setExplode(.5); expect(rig.getDirection('part')).toEqual(direction);
  });
  it('returns one selected explosion target while preserving the other exploded targets', () => {
    const scene = new THREE.Group();
    const left = new THREE.Mesh(new THREE.BoxGeometry(1, 1, 1)); left.position.x = -1; scene.add(left);
    const right = new THREE.Mesh(new THREE.BoxGeometry(1, 1, 1)); right.position.x = 1; scene.add(right);
    const nodes: ProductStructureNode[] = [
      { nodeId: 'root', label: 'root', nodeType: 'collection', nodeName: 'root', bodyId: 'root' },
      { nodeId: 'left', label: 'left', nodeType: 'part', parentNodeId: 'root', nodeName: 'left', bodyId: 'left' },
      { nodeId: 'right', label: 'right', nodeType: 'part', parentNodeId: 'root', nodeName: 'right', bodyId: 'right' },
    ];
    const rig = new ExplosionRig(nodes, scene, new Map([['left', left], ['right', right]]));
    rig.layout('root', 'current'); rig.setExplode(1);
    const explodedRightX = right.position.x;
    expect(rig.restoreNode('left')).toBe(true);
    expect(rig.isNodeRestored('left')).toBe(true);
    expect(left.position.x).toBe(-1); expect(right.position.x).toBe(explodedRightX);
    rig.setExplode(.5);
    expect(left.position.x).toBe(-1); expect(right.position.x).not.toBe(1);
    rig.setExplode(0); rig.setExplode(1);
    expect(rig.isNodeRestored('left')).toBe(false); expect(left.position.x).not.toBe(-1);
  });
  it('preserves the original vertical component order with uniform radial scaling', () => {
    const scene = new THREE.Group();
    const bottom = new THREE.Mesh(new THREE.BoxGeometry(1, 1, 1)); bottom.position.y = -3; scene.add(bottom);
    const middle = new THREE.Mesh(new THREE.BoxGeometry(2, 2.5, 2)); middle.position.y = -1; scene.add(middle);
    const top = new THREE.Mesh(new THREE.BoxGeometry(1, 1, 1)); top.position.y = 3; scene.add(top);
    const nodes: ProductStructureNode[] = [
      { nodeId: 'root', label: 'root', nodeType: 'collection', nodeName: 'root', bodyId: 'root' },
      ...['bottom', 'middle', 'top'].map((nodeId) => ({ nodeId, label: nodeId, nodeType: 'part' as const, parentNodeId: 'root', nodeName: nodeId, bodyId: nodeId })),
    ];
    const rig = new ExplosionRig(nodes, scene, new Map([['bottom', bottom], ['middle', middle], ['top', top]]));
    rig.layout('root', 'current'); rig.setExplode(1);
    expect(bottom.position.y).toBeLessThan(middle.position.y);
    expect(middle.position.y).toBeLessThan(top.position.y);
  });
  it('relaxes overlapping projected bounds for the active camera', () => {
    const scene = new THREE.Group(); const nodes: ProductStructureNode[] = [
      { nodeId: 'root', label: 'root', nodeType: 'collection', nodeName: 'root', bodyId: 'root' },
    ];
    const mapped = new Map<string, THREE.Object3D>();
    for (let index = 0; index < 8; index += 1) {
      const id = `part-${index}`; const part = new THREE.Mesh(new THREE.BoxGeometry(2, 2, 2)); part.name = id; scene.add(part); mapped.set(id, part);
      nodes.push({ nodeId: id, label: id, nodeType: 'part', parentNodeId: 'root', nodeName: id, bodyId: id });
    }
    const camera = new THREE.PerspectiveCamera(38, 1, .1, 100); camera.position.set(0, 0, 10); camera.lookAt(0, 0, 0); camera.updateProjectionMatrix(); camera.updateMatrixWorld(true);
    const rig = new ExplosionRig(nodes, scene, mapped); const plan = rig.layout('root', 'current', camera, .5);
    expect(plan.relaxationIterations).toBeGreaterThan(0);
    expect(plan.remainingOverlaps).toBeLessThan(28);
    expect(plan.entries.some((entry) => entry.distance > 2)).toBe(true);
  });
});
