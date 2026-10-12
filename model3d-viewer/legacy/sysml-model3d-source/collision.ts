import * as THREE from 'three';
import { OBB } from 'three/examples/jsm/math/OBB.js';
import type { InterferencePairResult, Model3DManifest } from '@aidc/model3d-contracts';

export interface CollisionProxy {
  componentId: string;
  bodyId?: string;
  localBox?: THREE.Box3;
  box: THREE.Box3;
  obb?: OBB;
}

export function pairId(a: string, b: string): string {
  return [a, b].sort().join('::');
}

export function collectCollisionProxies(manifest: Model3DManifest, components: Map<string, THREE.Object3D>): CollisionProxy[] {
  const enabled = new Set(manifest.collision?.enabledComponentIds ?? manifest.components.map((item) => item.componentId));
  const byId = new Map(manifest.components.map((item) => [item.componentId, item]));
  const result: CollisionProxy[] = [];
  for (const componentId of enabled) {
    const object = components.get(componentId);
    if (!object) continue;
    object.updateWorldMatrix(true, true);
    const localBox = objectLocalBounds(object);
    if (localBox.isEmpty()) continue;
    const obb = new OBB().fromBox3(localBox).applyMatrix4(object.matrixWorld);
    const worldBox = new THREE.Box3().setFromPoints(boxCorners(localBox).map((point) => point.applyMatrix4(object.matrixWorld)));
    result.push({ componentId, bodyId: byId.get(componentId)?.bodyId, localBox, box: worldBox, obb });
  }
  return result;
}

export function detectInterferences(
  proxies: CollisionProxy[],
  manifest: Model3DManifest,
  pose: Record<string, number> = {},
): InterferencePairResult[] {
  const tolerance = manifest.collision?.tolerance ?? 0;
  const allowed = new Map((manifest.collision?.allowedContacts ?? []).map((rule) => [pairId(rule.componentA, rule.componentB), rule.reason]));
  const results: InterferencePairResult[] = [];
  for (let a = 0; a < proxies.length; a += 1) for (let b = a + 1; b < proxies.length; b += 1) {
    const first = proxies[a]; const second = proxies[b];
    if (first.bodyId && first.bodyId === second.bodyId) continue;
    const gap = boxDistance(first.box, second.box);
    const intersects = first.obb && second.obb ? first.obb.intersectsOBB(second.obb) : first.box.intersectsBox(second.box);
    if (!intersects && gap > tolerance) continue;
    const id = pairId(first.componentId, second.componentId);
    const allowedReason = allowed.get(id);
    results.push({
      pairId: id,
      componentA: first.componentId,
      componentB: second.componentId,
      status: allowedReason ? 'ALLOWED_CONTACT' : 'WARNING',
      precision: 'MESH_PREVIEW',
      minimumDistance: intersects ? 0 : gap,
      commonVolume: intersects ? overlapVolume(first.box, second.box) : 0,
      tolerance,
      checkedAt: new Date().toISOString(),
      pose: { ...pose },
      message: allowedReason ?? (intersects ? '碰撞代理相交，需精确校核' : `间隙小于预警阈值 ${formatDistance(tolerance)}`),
    });
  }
  return results.sort((left, right) => statusRank(left.status) - statusRank(right.status) || left.pairId!.localeCompare(right.pairId!));
}

export function refreshCollisionProxies(proxies: CollisionProxy[], components: Map<string, THREE.Object3D>): void {
  for (const proxy of proxies) {
    const object = components.get(proxy.componentId);
    if (!object) continue;
    object.updateWorldMatrix(true, true);
    const local = proxy.localBox ?? objectLocalBounds(object);
    proxy.localBox = local;
    proxy.box.setFromPoints(boxCorners(local).map((point) => point.applyMatrix4(object.matrixWorld)));
    proxy.obb = new OBB().fromBox3(local).applyMatrix4(object.matrixWorld);
  }
}

export function formatDistance(value: number | undefined): string {
  if (value == null || !Number.isFinite(value)) return '—';
  return `${(value * 1000).toFixed(value < 0.001 ? 2 : 1)} mm`;
}

function boxCorners(box: THREE.Box3): THREE.Vector3[] {
  const { min, max } = box;
  return [
    new THREE.Vector3(min.x, min.y, min.z), new THREE.Vector3(max.x, min.y, min.z),
    new THREE.Vector3(min.x, max.y, min.z), new THREE.Vector3(max.x, max.y, min.z),
    new THREE.Vector3(min.x, min.y, max.z), new THREE.Vector3(max.x, min.y, max.z),
    new THREE.Vector3(min.x, max.y, max.z), new THREE.Vector3(max.x, max.y, max.z),
  ];
}

function objectLocalBounds(root: THREE.Object3D): THREE.Box3 {
  root.updateWorldMatrix(true, true);
  const inverseRoot = root.matrixWorld.clone().invert();
  const result = new THREE.Box3();
  root.traverse((object) => {
    if (!(object instanceof THREE.Mesh) || !object.geometry) return;
    object.geometry.computeBoundingBox();
    const bounds = object.geometry.boundingBox;
    if (!bounds || bounds.isEmpty()) return;
    const toRoot = inverseRoot.clone().multiply(object.matrixWorld);
    for (const point of boxCorners(bounds)) result.expandByPoint(point.applyMatrix4(toRoot));
  });
  return result;
}

function boxDistance(a: THREE.Box3, b: THREE.Box3): number {
  const dx = Math.max(0, a.min.x - b.max.x, b.min.x - a.max.x);
  const dy = Math.max(0, a.min.y - b.max.y, b.min.y - a.max.y);
  const dz = Math.max(0, a.min.z - b.max.z, b.min.z - a.max.z);
  return Math.hypot(dx, dy, dz);
}

function overlapVolume(a: THREE.Box3, b: THREE.Box3): number {
  return Math.max(0, Math.min(a.max.x, b.max.x) - Math.max(a.min.x, b.min.x))
    * Math.max(0, Math.min(a.max.y, b.max.y) - Math.max(a.min.y, b.min.y))
    * Math.max(0, Math.min(a.max.z, b.max.z) - Math.max(a.min.z, b.min.z));
}

function statusRank(status: InterferencePairResult['status']): number {
  return status === 'CONFIRMED' ? 0 : status === 'WARNING' ? 1 : status === 'UNVERIFIED' ? 2 : status === 'CLEARED' ? 3 : 4;
}
