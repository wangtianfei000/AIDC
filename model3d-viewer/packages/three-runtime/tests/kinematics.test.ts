import { describe, expect, test } from 'vitest';
import * as THREE from 'three';
import { readFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import type { Model3DManifest } from '@aidc/model3d-contracts';
import { RobotRig } from '../src/kinematics';

const manifest: Model3DManifest = {
  schemaVersion: 'aidc.model3d.package/0.1', modelId: 'test', revision: '1', name: 'test', manufacturingStatus: 'TEST',
  coordinateSystem: { lengthUnit: 'm', angleUnit: 'rad', forward: '+X', left: '+Y', up: '+Z', quaternionOrder: 'xyzw' },
  source: { cadUri: '', cadSha256: '', sysmlUri: '', sysmlSha256: '', publisherVersion: 'test' }, assets: [], rootBodyId: 'root',
  bodies: [
    { bodyId: 'root', label: 'Root', nodeName: 'root', translation: [0, 0, 0], rotation: [0, 0, 0, 1] },
    { bodyId: 'moving', label: 'Moving', nodeName: 'moving', parentBodyId: 'root', translation: [0, 0, 1], rotation: [0, 0, 0, 1] },
  ], components: [],
  joints: [{ jointId: 'joint', type: 'revolute', parentBodyId: 'root', childBodyId: 'moving', axis: [0, 0, 1], neutralValue: 0.25, zeroOffset: 0, limit: { lower: -1, upper: 1 }, drive: { channel: 'joint', sign: -1, scale: 1 } }],
  statistics: { sourceObjectCount: 0, exportedGeometryCount: 0, triangleCount: 0, componentCount: 0, jointCount: 1, visualBounds: { minimum: [0, 0, 0], maximum: [0, 0, 1] } },
};

describe('RobotRig', () => {
  test('uses named bodies, clamps limits and preserves the base pose', () => {
    const scene = new THREE.Group();
    const unrelated = new THREE.Object3D(); unrelated.name = 'unrelated'; scene.add(unrelated);
    const root = new THREE.Object3D(); root.name = 'root'; scene.add(root);
    const moving = new THREE.Object3D(); moving.name = 'moving'; root.add(moving);
    const rig = new RobotRig(manifest, scene);
    expect(rig.setJoint('joint', 2)).toBe(1);
    const expected = new THREE.Quaternion().setFromAxisAngle(new THREE.Vector3(0, 0, 1), -0.75);
    expect(moving.quaternion.angleTo(expected)).toBeLessThan(1e-10);
    expect(unrelated.quaternion.equals(new THREE.Quaternion())).toBe(true);
    rig.reset();
    expect(moving.quaternion.angleTo(new THREE.Quaternion())).toBeLessThan(1e-10);
    expect(rig.getValue('joint')).toBe(0.25);
  });

  test('evaluates fixed, revolute and prismatic joints without moving unrelated branches', () => {
    const scene = new THREE.Group();
    const root = new THREE.Object3D(); root.name = 'root'; scene.add(root);
    const moving = new THREE.Object3D(); moving.name = 'moving'; root.add(moving);
    const slider = new THREE.Object3D(); slider.name = 'slider'; slider.position.set(1, 2, 3); moving.add(slider);
    const fixed = new THREE.Object3D(); fixed.name = 'fixed'; scene.add(fixed);
    const unrelated = new THREE.Object3D(); unrelated.name = 'unrelated'; scene.add(unrelated);
    const extended: Model3DManifest = {
      ...manifest,
      bodies: [...manifest.bodies, { bodyId: 'slider', label: 'Slider', nodeName: 'slider', parentBodyId: 'moving', translation: [1, 2, 3], rotation: [0, 0, 0, 1] }, { bodyId: 'fixed', label: 'Fixed', nodeName: 'fixed', translation: [0, 0, 0], rotation: [0, 0, 0, 1] }],
      joints: [
        ...manifest.joints,
        { jointId: 'slide', type: 'prismatic', parentBodyId: 'moving', childBodyId: 'slider', axis: [1, 0, 0], neutralValue: 0, zeroOffset: 0, limit: { lower: 0, upper: .2 }, drive: { channel: 'slide', sign: 1, scale: 1 } },
        { jointId: 'fixed-joint', type: 'fixed', parentBodyId: 'root', childBodyId: 'fixed', axis: [0, 0, 1], neutralValue: 0, zeroOffset: 0 },
      ],
    };
    const rig = new RobotRig(extended, scene);
    const unrelatedBefore = unrelated.matrix.clone();
    const applied = rig.setPose({ joint: -.5, slide: .3, 'fixed-joint': 1, unknown: 2 });
    expect(applied).toEqual({ joint: -.5, slide: .2, 'fixed-joint': 0 });
    expect(slider.position.toArray()).toEqual([1.2, 2, 3]);
    expect(fixed.position.toArray()).toEqual([0, 0, 0]);
    expect(fixed.quaternion.equals(new THREE.Quaternion())).toBe(true);
    expect(unrelated.matrix.equals(unrelatedBefore)).toBe(true);
    expect(rig.getPose().slide).toBe(.2);
    rig.reset();
    expect(slider.position.toArray()).toEqual([1, 2, 3]);
  });

  test('matches the frozen asymmetric Jingang pose within tolerance', () => {
    const sample = fileURLToPath(new URL('../../../../samples/jingang-10axis-system-design/model3d/', import.meta.url));
    const actualManifest = JSON.parse(readFileSync(`${sample}generated/model3d.manifest.json`, 'utf8')) as Model3DManifest;
    const baseline = JSON.parse(readFileSync(`${sample}expected/v2-asymmetric-motion-pose.json`, 'utf8')) as {
      joints: Record<string, number>; bodies: Record<string, { translation: [number, number, number]; rotation: [number, number, number, number] }>; tolerance: number;
    };
    const scene = new THREE.Group();
    const nodes = new Map(actualManifest.bodies.map((body) => {
      const node = new THREE.Object3D(); node.name = body.nodeName; node.position.fromArray(body.translation); node.quaternion.fromArray(body.rotation);
      return [body.bodyId, node] as const;
    }));
    for (const body of actualManifest.bodies) (body.parentBodyId ? nodes.get(body.parentBodyId) : scene)?.add(nodes.get(body.bodyId)!);
    const rig = new RobotRig(actualManifest, scene); rig.setPose(baseline.joints); scene.updateMatrixWorld(true);
    for (const [bodyId, expected] of Object.entries(baseline.bodies)) {
      const node = nodes.get(bodyId)!;
      const position = node.getWorldPosition(new THREE.Vector3());
      const quaternion = node.getWorldQuaternion(new THREE.Quaternion());
      expect(position.distanceTo(new THREE.Vector3(...expected.translation)), bodyId).toBeLessThan(baseline.tolerance);
      expect(quaternion.angleTo(new THREE.Quaternion(...expected.rotation)), bodyId).toBeLessThan(baseline.tolerance);
    }
  });
});
