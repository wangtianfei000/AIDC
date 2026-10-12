import * as THREE from 'three';
import { MeshBVH } from 'three-mesh-bvh';
import type { InterferencePairResult, Model3DManifest } from '@aidc/model3d-contracts';

interface CollisionMesh {
  mesh: THREE.Mesh;
  bvh: MeshBVH;
  box: THREE.Box3;
}

export interface CollisionProxy {
  componentId: string;
  bodyId?: string;
  localBox?: THREE.Box3;
  box: THREE.Box3;
  meshes: CollisionMesh[];
}

const boundsTrees = new WeakMap<THREE.BufferGeometry, MeshBVH>();

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
    const worldBox = new THREE.Box3().setFromPoints(boxCorners(localBox).map((point) => point.applyMatrix4(object.matrixWorld)));
    const meshes = collectCollisionMeshes(object);
    if (!meshes.length) continue;
    result.push({ componentId, bodyId: byId.get(componentId)?.bodyId, localBox, box: worldBox, meshes });
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
    if (gap > tolerance) continue;
    const proximity = meshProximity(first, second, tolerance);
    if (!proximity) continue;
    const intersects = proximity.intersects;
    const id = pairId(first.componentId, second.componentId);
    const allowedReason = allowed.get(id);
    results.push({
      pairId: id,
      componentA: first.componentId,
      componentB: second.componentId,
      status: allowedReason ? 'ALLOWED_CONTACT' : 'WARNING',
      precision: 'MESH_PREVIEW',
      minimumDistance: proximity.distance,
      tolerance,
      checkedAt: new Date().toISOString(),
      pose: { ...pose },
      message: allowedReason ?? (intersects ? '三角网格相交，建议进行精确校核' : `实际网格间隙小于预警阈值 ${formatDistance(tolerance)}`),
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
    for (const item of proxy.meshes) item.box.copy(meshWorldBox(item.mesh));
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

function collectCollisionMeshes(root: THREE.Object3D): CollisionMesh[] {
  const meshes: CollisionMesh[] = [];
  root.traverse((object) => {
    if (!(object instanceof THREE.Mesh) || !(object.geometry instanceof THREE.BufferGeometry)) return;
    let bvh = boundsTrees.get(object.geometry);
    if (!bvh) {
      bvh = new MeshBVH(object.geometry, { indirect: true });
      boundsTrees.set(object.geometry, bvh);
    }
    meshes.push({ mesh: object, bvh, box: meshWorldBox(object) });
  });
  return meshes;
}

function meshWorldBox(mesh: THREE.Mesh): THREE.Box3 {
  mesh.geometry.computeBoundingBox();
  return mesh.geometry.boundingBox?.clone().applyMatrix4(mesh.matrixWorld) ?? new THREE.Box3();
}

function meshProximity(first: CollisionProxy, second: CollisionProxy, tolerance: number): { intersects: boolean; distance: number } | undefined {
  let minimumDistance = Number.POSITIVE_INFINITY;
  for (const left of first.meshes) for (const right of second.meshes) {
    const broadGap = boxDistance(left.box, right.box);
    if (broadGap > tolerance) continue;
    const rightToLeft = left.mesh.matrixWorld.clone().invert().multiply(right.mesh.matrixWorld);
    if (left.bvh.intersectsGeometry(right.mesh.geometry, rightToLeft)) return { intersects: true, distance: 0 };
    if (tolerance <= 0) continue;
    const worldScale = maximumWorldScale(left.mesh);
    const hit = left.bvh.closestPointToGeometry(right.mesh.geometry, rightToLeft, undefined, undefined, 0, tolerance / worldScale);
    if (hit) minimumDistance = Math.min(minimumDistance, hit.distance * worldScale);
  }
  return Number.isFinite(minimumDistance) ? { intersects: false, distance: minimumDistance } : undefined;
}

function maximumWorldScale(object: THREE.Object3D): number {
  const scale = object.getWorldScale(new THREE.Vector3());
  return Math.max(Math.abs(scale.x), Math.abs(scale.y), Math.abs(scale.z), Number.EPSILON);
}

function boxDistance(a: THREE.Box3, b: THREE.Box3): number {
  const dx = Math.max(0, a.min.x - b.max.x, b.min.x - a.max.x);
  const dy = Math.max(0, a.min.y - b.max.y, b.min.y - a.max.y);
  const dz = Math.max(0, a.min.z - b.max.z, b.min.z - a.max.z);
  return Math.hypot(dx, dy, dz);
}

function statusRank(status: InterferencePairResult['status']): number {
  return status === 'CONFIRMED' ? 0 : status === 'WARNING' ? 1 : status === 'UNVERIFIED' ? 2 : status === 'CLEARED' ? 3 : 4;
}
