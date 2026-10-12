import { describe, expect, it } from 'vitest';
import * as THREE from 'three';
import type { ProductStructureNode } from '@aidc/model3d-contracts';
import { ExplosionRig, inferProductStructure } from '../src/assembly';

function camera(scene: THREE.Object3D) {
  scene.updateMatrixWorld(true); const sphere = new THREE.Box3().setFromObject(scene).getBoundingSphere(new THREE.Sphere());
  const value = new THREE.PerspectiveCamera(38, 1.4, .001, 10000);
  value.position.copy(sphere.center).add(new THREE.Vector3(1, .75, 1).normalize().multiplyScalar(Math.max(sphere.radius * 3.2, .01)));
  value.near = Math.max(sphere.radius / 100, .000001); value.far = Math.max(sphere.radius * 30, 1); value.lookAt(sphere.center); value.updateProjectionMatrix(); value.updateMatrixWorld(true);
  return value;
}

function part(name: string, size: [number, number, number], position: [number, number, number]) {
  const value = new THREE.Mesh(new THREE.BoxGeometry(...size)); value.name = name; value.position.set(...position); return value;
}

function assertUniversalPlan(scene: THREE.Object3D, nodes: ProductStructureNode[], mapped: Map<string, THREE.Object3D>, scope = 'root') {
  scene.updateMatrixWorld(true);
  const baselines = new Map([...mapped].map(([id, object]) => [id, object.position.clone()]));
  const rig = new ExplosionRig(nodes, scene, mapped);
  const first = rig.layout(scope, 'current', camera(scene), 1);
  const repeated = rig.layout(scope, 'current', camera(scene), 1);
  expect(repeated.entries).toEqual(first.entries);
  expect(first.entries.length).toBeGreaterThan(1);
  expect(first.entries.every((entry) => Number.isFinite(entry.distance) && entry.distance > 0)).toBe(true);
  rig.setExplode(1); rig.reset();
  for (const [id, position] of baselines) expect(mapped.get(id)?.position.toArray()).toEqual(position.toArray());
  expect(first.remainingOverlaps).toBeLessThanOrEqual(first.entries.length);
}

describe('E5 generic exploded-view acceptance', () => {
  it('accepts a multilevel robot assembly with rigid left/right modules', () => {
    const scene = new THREE.Group(); const trunk = new THREE.Group(); trunk.name = 'trunk'; trunk.add(part('battery', [1, .5, .5], [0, 0, 0]));
    const left = new THREE.Group(); left.name = 'left-leg'; left.position.x = -1.2; left.add(part('left-foot', [.5, 1, .4], [0, -1, 0]));
    const right = new THREE.Group(); right.name = 'right-leg'; right.position.x = 1.2; right.add(part('right-foot', [.5, 1, .4], [0, -1, 0]));
    const head = part('head', [1.2, .8, .7], [0, 1.5, 0]); scene.add(trunk, left, right, head);
    const nodes: ProductStructureNode[] = [
      { nodeId: 'root', label: 'robot', nodeType: 'collection', nodeName: 'root', bodyId: 'root' },
      ...[['trunk', trunk], ['left', left], ['right', right], ['head', head]].map(([id, object]) => ({ nodeId: id as string, label: id as string, nodeType: object instanceof THREE.Group ? 'collection' as const : 'part' as const, parentNodeId: 'root', nodeName: (object as THREE.Object3D).name, bodyId: id as string })),
    ];
    assertUniversalPlan(scene, nodes, new Map([['trunk', trunk], ['left', left], ['right', right], ['head', head]]));
  });

  it('accepts a coaxial shaft assembly', () => {
    const scene = new THREE.Group(); const mapped = new Map<string, THREE.Object3D>();
    const nodes: ProductStructureNode[] = [{ nodeId: 'root', label: 'shaft', nodeType: 'collection', nodeName: 'root', bodyId: 'root' }];
    ['shaft-core', 'bearing-a', 'spacer', 'bearing-b', 'cover'].forEach((id, index) => {
      const object = part(id, [.4 + index * .08, .4 + index * .08, .3], [0, 0, (index - 2) * .32]); scene.add(object); mapped.set(id, object);
      nodes.push({ nodeId: id, label: id, nodeType: 'part', parentNodeId: 'root', nodeName: id, bodyId: id });
    });
    assertUniversalPlan(scene, nodes, mapped);
  });

  it('accepts a housing assembly with cover and internal parts at millimetre and metre scales', () => {
    for (const scale of [.001, 1000]) {
      const scene = new THREE.Group(); const mapped = new Map<string, THREE.Object3D>();
      const nodes: ProductStructureNode[] = [{ nodeId: 'root', label: 'housing', nodeType: 'collection', nodeName: 'root', bodyId: 'root' }];
      [['shell', [2, 1.5, 1], [0, 0, 0]], ['cover', [2, .12, 1], [0, 1, 0]], ['motor', [.6, .6, .8], [-.4, 0, 0]], ['board', [.8, .08, .5], [.5, 0, 0]]] .forEach(([id, size, position]) => {
        const object = part(id as string, (size as number[]).map((value) => value * scale) as [number, number, number], (position as number[]).map((value) => value * scale) as [number, number, number]);
        scene.add(object); mapped.set(id as string, object); nodes.push({ nodeId: id as string, label: id as string, nodeType: 'part', parentNodeId: 'root', nodeName: id as string, bodyId: id as string });
      });
      assertUniversalPlan(scene, nodes, mapped);
    }
  });

  it('infers a product tree from a configuration-free third-party STEP-to-GLB hierarchy', () => {
    const importedScene = new THREE.Group(); importedScene.name = 'third-party-step';
    const wrapper = new THREE.Group(); wrapper.name = 'STEP assembly'; importedScene.add(wrapper);
    for (let index = 0; index < 5; index += 1) wrapper.add(part(`STEP solid ${index + 1}`, [1, .4 + index * .1, .3], [(index - 2) * .8, 0, 0]));
    const inferred = inferProductStructure(importedScene);
    expect(inferred.entries.filter((entry) => entry.nodeType === 'part')).toHaveLength(5);
    expect(inferred.entries.filter((entry) => !entry.parentNodeId)).toHaveLength(1);
    assertUniversalPlan(importedScene, inferred.entries, inferred.nodes, inferred.entries[0].nodeId);
  });
});
