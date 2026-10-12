import { readFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import { join } from 'node:path';
import { expect, test } from 'vitest';
import * as THREE from 'three';
import { GLTFLoader } from 'three/examples/jsm/loaders/GLTFLoader.js';
import { RobotRig } from '../src/kinematics';
import type { Model3DManifest } from '@aidc/model3d-contracts';

const project = new URL('../../../../../Jingang/apps/hardware-test/model3d/generated/', import.meta.url);
const read = (name: string) => readFileSync(process.env.MICRODUCK_BASELINE_PACKAGE
  ? join(process.env.MICRODUCK_BASELINE_PACKAGE, name) : fileURLToPath(new URL(name, project)));

test('native12 GLB follows current FreeCAD at zero, each positive/negative joint, and stand', async () => {
  const manifest: Model3DManifest = JSON.parse(read('model3d.manifest.json').toString());
  const expected = JSON.parse(read('cad-pose-reference.json').toString()) as Array<{
    name: string; anglesDeg: Record<string, number>;
    bodies: Record<string, { translation: number[]; rotation: number[] }>;
  }>;
  const bytes = read('visual.glb');
  const gltf = await new GLTFLoader().parseAsync(bytes.buffer.slice(bytes.byteOffset, bytes.byteOffset + bytes.byteLength), '');
  const rig = new RobotRig(manifest, gltf.scene);
  expect(rig.jointIds).toHaveLength(12);
  expect(manifest.bodies).toHaveLength(13);
  expect(expected).toHaveLength(26);
  for (const c of expected) {
    rig.reset();
    rig.setPose(Object.fromEntries(Object.entries(c.anglesDeg).map(([n, d]) => [n, THREE.MathUtils.degToRad(d)])));
    gltf.scene.updateMatrixWorld(true);
    for (const [name, pose] of Object.entries(c.bodies)) {
      const node = gltf.scene.getObjectByName(name)!;
      expect(node, `${c.name}/${name}`).toBeDefined();
      expect(node.getWorldPosition(new THREE.Vector3()).distanceTo(new THREE.Vector3(...pose.translation)), `${c.name}/${name}:position`).toBeLessThan(1e-7);
      expect(node.getWorldQuaternion(new THREE.Quaternion()).angleTo(new THREE.Quaternion(...pose.rotation)), `${c.name}/${name}:rotation`).toBeLessThan(1e-6);
    }
  }
  const map = JSON.parse(read('hardware-map.json').toString()).joints;
  expect(map.map((j: {servoId: number}) => j.servoId).sort((a: number,b: number) => a-b)).toEqual([10,11,12,13,14,20,21,22,23,24,30,32]);
  for (const row of map) {
    expect(rig.getDefinition(row.jointId).childBodyId).toBe(row.bodyId);
  }
});
