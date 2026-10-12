import * as THREE from 'three';
import type { AssemblySequence, AssemblyStep, Model3DManifest, ProductStructureNode, Vec3 } from '@aidc/model3d-contracts';

type Target = {
  object: THREE.Object3D;
  basePosition: THREE.Vector3;
  baseQuaternion: THREE.Quaternion;
  explodeDirection: THREE.Vector3;
  explodeDistance?: number;
};

export type ExplosionDepth = 'current' | 'two-levels' | 'leaves';

export interface ExplosionPlanEntry {
  nodeId: string;
  direction: Vec3;
  distance: number;
  lane: number;
}

export interface ExplosionPlan {
  scopeNodeId: string;
  depth: ExplosionDepth;
  origin: Vec3;
  cameraBasis: { right: Vec3; up: Vec3; view: Vec3 };
  entries: ExplosionPlanEntry[];
  relaxationIterations: number;
  remainingOverlaps: number;
}

/** Applies reversible exploded and assembly-step transforms without cumulative drift. */
export class AssemblyRig {
  private readonly targets = new Map<string, Target>();
  private readonly modelWorldQuaternion = new THREE.Quaternion();

  constructor(readonly manifest: Model3DManifest, readonly scene: THREE.Object3D, components: Map<string, THREE.Object3D>) {
    scene.updateWorldMatrix(true, true);
    scene.getWorldQuaternion(this.modelWorldQuaternion);
    const used = new Set<THREE.Object3D>();
    for (const component of manifest.components) {
      const object = components.get(component.componentId);
      if (!object || used.has(object) || component.componentId === manifest.modelId) continue;
      // Parent assembly nodes are anchors; moving them together with mapped descendants
      // would apply the explosion twice.
      const hasMappedDescendant = manifest.components.some((candidate) => {
        const child = components.get(candidate.componentId);
        return child && child !== object && isDescendant(child, object);
      });
      if (hasMappedDescendant) continue;
      used.add(object);
      this.targets.set(component.componentId, {
        object, basePosition: object.position.clone(), baseQuaternion: object.quaternion.clone(), explodeDirection: new THREE.Vector3(),
      });
    }
  }

  get componentIds(): string[] { return [...this.targets.keys()]; }

  applySequence(sequence: AssemblySequence, stepIndex: number, progress: number): void {
    this.reset();
    const boundedIndex = Math.max(-1, Math.min(stepIndex, sequence.steps.length - 1));
    for (let index = 0; index <= boundedIndex; index += 1) {
      const stepProgress = index < boundedIndex ? 1 : THREE.MathUtils.clamp(progress, 0, 1);
      this.applyStep(sequence.steps[index], stepProgress);
    }
  }

  reset(): void {
    for (const target of this.targets.values()) {
      target.object.position.copy(target.basePosition);
      target.object.quaternion.copy(target.baseQuaternion);
    }
    this.scene.updateWorldMatrix(true, true);
  }

  private applyStep(step: AssemblyStep, progress: number): void {
    const frame = interpolateKeyframes(step, progress);
    for (const componentId of step.componentIds) {
      const target = this.targets.get(componentId); if (!target) continue;
      this.applyModelOffset(target, new THREE.Vector3(...frame.position));
      target.object.quaternion.copy(target.baseQuaternion).multiply(new THREE.Quaternion(...frame.quaternion)).normalize();
    }
  }

  private applyModelOffset(target: Target, offset: THREE.Vector3): void {
    const worldOffset = offset.applyQuaternion(this.modelWorldQuaternion);
    const parentQuaternion = target.object.parent?.getWorldQuaternion(new THREE.Quaternion()) ?? new THREE.Quaternion();
    const parentScale = target.object.parent?.getWorldScale(new THREE.Vector3()) ?? new THREE.Vector3(1, 1, 1);
    const localOffset = worldOffset.applyQuaternion(parentQuaternion.invert()).divide(parentScale);
    target.object.position.copy(target.basePosition).add(localOffset);
  }
}

/** Applies a reversible exploded view to CAD product-structure nodes. */
export class ExplosionRig {
  private readonly targets = new Map<string, Target>();
  private readonly baselines = new Map<string, Target>();
  private readonly returnedNodeIds = new Set<string>();
  private readonly modelWorldQuaternion = new THREE.Quaternion();
  private spacing = 1;
  private planValue?: ExplosionPlan;

  constructor(readonly productStructure: ProductStructureNode[], readonly scene: THREE.Object3D, productNodes: Map<string, THREE.Object3D>) {
    scene.updateWorldMatrix(true, true);
    scene.getWorldQuaternion(this.modelWorldQuaternion);
    const used = new Set<THREE.Object3D>();
    for (const node of productStructure) {
      const object = productNodes.get(node.nodeId);
      if (!object || used.has(object)) continue;
      used.add(object);
      this.baselines.set(node.nodeId, {
        object, basePosition: object.position.clone(), baseQuaternion: object.quaternion.clone(), explodeDirection: new THREE.Vector3(1, 0, 0),
      });
    }
    const root = productStructure.find((node) => !node.parentNodeId);
    if (root) this.layout(root.nodeId, 'current');
  }

  get nodeIds(): string[] { return [...this.targets.keys()]; }
  get plan(): ExplosionPlan | undefined { return this.planValue; }

  layout(scopeNodeId: string, depth: ExplosionDepth, camera?: THREE.Camera, spacing = this.spacing): ExplosionPlan {
    this.reset();
    this.targets.clear();
    this.spacing = THREE.MathUtils.clamp(spacing, 0.5, 2.5);
    const root = this.productStructure.find((node) => !node.parentNodeId);
    const scope = this.productStructure.find((node) => node.nodeId === scopeNodeId && node.nodeType === 'collection') ?? root;
    if (!scope) throw new Error('产品结构缺少可爆炸的根集合');
    const candidates = this.resolveTargets(scope.nodeId, depth).filter((node) => this.baselines.has(node.nodeId));
    const fallback = candidates.length ? candidates : this.productStructure.filter((node) => node.nodeType === 'part' && this.baselines.has(node.nodeId));
    const scopeObject = this.baselines.get(scope.nodeId)?.object ?? this.scene;
    this.scene.updateWorldMatrix(true, true);
    const scopeBox = new THREE.Box3().setFromObject(scopeObject, true);
    const scopeCenter = scopeBox.getCenter(new THREE.Vector3());
    const scopeDiagonal = Math.max(scopeBox.getSize(new THREE.Vector3()).length(), 1e-4);
    const basis = cameraBasis(camera);
    const ordered = fallback.map((node) => {
      const baseline = this.baselines.get(node.nodeId)!;
      const box = new THREE.Box3().setFromObject(baseline.object, true);
      const center = box.getCenter(new THREE.Vector3());
      const delta = center.clone().sub(scopeCenter);
      const x = delta.dot(basis.right); const y = delta.dot(basis.up);
      return { node, baseline, box, center, angle: Math.atan2(y, x), radius: Math.hypot(x, y) };
    }).sort((left, right) => left.angle - right.angle || left.node.nodeId.localeCompare(right.node.nodeId));
    const angularLanes = new Map<number, number>();
    const minimumRadialDistance = scopeDiagonal * 1e-5;
    const prepared = ordered.map((item, index) => {
      const sectorAngle = ordered.length > 1 ? -Math.PI + index * Math.PI * 2 / ordered.length : 0;
      const radial = item.center.clone().sub(scopeCenter);
      const radialDistance = radial.length();
      const angle = item.radius > scopeDiagonal * .015 ? item.angle : sectorAngle;
      const laneKey = Math.round(angle / (Math.PI / 8));
      const lane = angularLanes.get(laneKey) ?? 0; angularLanes.set(laneKey, lane + 1);
      const worldDirection = radialDistance > minimumRadialDistance
        ? radial.normalize()
        : basis.right.clone().multiplyScalar(Math.cos(sectorAngle)).addScaledVector(basis.up, Math.sin(sectorAngle)).normalize();
      const size = item.box.getSize(new THREE.Vector3());
      const projectedSize = Math.abs(size.x * worldDirection.x) + Math.abs(size.y * worldDirection.y) + Math.abs(size.z * worldDirection.z);
      const requiredDistance = scopeDiagonal * .22 + projectedSize * .55;
      return { ...item, angle, lane, worldDirection, radialDistance, requiredDistance };
    });
    const uniformRadialScale = THREE.MathUtils.clamp(Math.max(.35, ...prepared
      .filter((item) => item.radialDistance > minimumRadialDistance)
      .map((item) => item.requiredDistance / item.radialDistance)), .35, 6);
    const drafts = prepared.map((item) => ({
      ...item,
      distance: (item.radialDistance > minimumRadialDistance
        ? item.radialDistance * uniformRadialScale
        : item.requiredDistance) * this.spacing,
    }));
    const relaxation = camera ? relaxScreenOverlaps(drafts, camera) : { iterations: 0, overlaps: 0 };
    const entries: ExplosionPlanEntry[] = drafts.map((item) => {
      const localDirection = item.worldDirection.clone().applyQuaternion(this.modelWorldQuaternion.clone().invert()).normalize();
      const target: Target = { ...item.baseline, explodeDirection: localDirection, explodeDistance: item.distance };
      this.targets.set(item.node.nodeId, target);
      return { nodeId: item.node.nodeId, direction: localDirection.toArray() as Vec3, distance: item.distance, lane: item.lane };
    });
    this.planValue = {
      scopeNodeId: scope.nodeId,
      depth,
      origin: this.scene.worldToLocal(scopeCenter.clone()).toArray() as Vec3,
      cameraBasis: { right: basis.right.toArray() as Vec3, up: basis.up.toArray() as Vec3, view: basis.view.toArray() as Vec3 },
      entries,
      relaxationIterations: relaxation.iterations,
      remainingOverlaps: relaxation.overlaps,
    };
    return this.planValue;
  }

  setSpacing(spacing: number): void {
    const next = THREE.MathUtils.clamp(spacing, 0.5, 2.5);
    const ratio = next / this.spacing; this.spacing = next;
    for (const target of this.targets.values()) target.explodeDistance = (target.explodeDistance ?? 0) * ratio;
    if (this.planValue) this.planValue = { ...this.planValue, entries: this.planValue.entries.map((entry) => ({ ...entry, distance: entry.distance * ratio })) };
  }

  getDirection(nodeId: string): Vec3 | undefined {
    const direction = this.targets.get(nodeId)?.explodeDirection;
    return direction ? direction.toArray() as Vec3 : undefined;
  }

  setDirection(nodeId: string, direction: Vec3): void {
    const target = this.targets.get(nodeId); if (!target) return;
    const next = new THREE.Vector3(...direction);
    if (next.lengthSq() > 1e-10) target.explodeDirection.copy(next.normalize());
  }

  setExplode(factor: number): void {
    const amount = THREE.MathUtils.clamp(factor, 0, 1);
    this.restoreBaselines();
    if (amount <= 0) this.returnedNodeIds.clear();
    for (const [nodeId, target] of this.targets) {
      if (this.returnedNodeIds.has(nodeId)) continue;
      this.applyModelOffset(target, target.explodeDirection.clone().multiplyScalar((target.explodeDistance ?? 0) * amount));
    }
  }

  restoreNode(nodeId: string): boolean {
    const target = this.targets.get(nodeId);
    if (!target) return false;
    target.object.position.copy(target.basePosition);
    target.object.quaternion.copy(target.baseQuaternion);
    this.returnedNodeIds.add(nodeId);
    this.scene.updateWorldMatrix(true, true);
    return true;
  }

  isNodeRestored(nodeId: string): boolean {
    return this.returnedNodeIds.has(nodeId);
  }

  reset(): void {
    this.returnedNodeIds.clear();
    this.restoreBaselines();
  }

  private restoreBaselines(): void {
    for (const target of this.targets.values()) {
      target.object.position.copy(target.basePosition);
      target.object.quaternion.copy(target.baseQuaternion);
    }
    this.scene.updateWorldMatrix(true, true);
  }

  private applyModelOffset(target: Target, offset: THREE.Vector3): void {
    const worldOffset = offset.applyQuaternion(this.modelWorldQuaternion);
    const parentQuaternion = target.object.parent?.getWorldQuaternion(new THREE.Quaternion()) ?? new THREE.Quaternion();
    const parentScale = target.object.parent?.getWorldScale(new THREE.Vector3()) ?? new THREE.Vector3(1, 1, 1);
    const localOffset = worldOffset.applyQuaternion(parentQuaternion.invert()).divide(parentScale);
    target.object.position.copy(target.basePosition).add(localOffset);
  }

  private resolveTargets(scopeNodeId: string, depth: ExplosionDepth): ProductStructureNode[] {
    const children = new Map<string, ProductStructureNode[]>();
    for (const node of this.productStructure) {
      if (!node.parentNodeId) continue;
      const list = children.get(node.parentNodeId) ?? []; list.push(node); children.set(node.parentNodeId, list);
    }
    for (const list of children.values()) list.sort((left, right) => left.nodeId.localeCompare(right.nodeId));
    const direct = children.get(scopeNodeId) ?? [];
    if (depth === 'current') return direct;
    if (depth === 'two-levels') return direct.flatMap((node) => node.nodeType === 'collection' && children.get(node.nodeId)?.length ? children.get(node.nodeId)! : [node]);
    const leaves: ProductStructureNode[] = [];
    const visit = (node: ProductStructureNode) => {
      const descendants = children.get(node.nodeId) ?? [];
      if (!descendants.length || node.nodeType === 'part') { if (node.nodeId !== scopeNodeId) leaves.push(node); return; }
      descendants.forEach(visit);
    };
    const scope = this.productStructure.find((node) => node.nodeId === scopeNodeId);
    if (scope) visit(scope);
    return leaves;
  }
}

function cameraBasis(camera?: THREE.Camera) {
  if (!camera) return { right: new THREE.Vector3(1, 0, 0), up: new THREE.Vector3(0, 1, 0), view: new THREE.Vector3(0, 0, -1) };
  camera.updateWorldMatrix(true, false);
  const right = new THREE.Vector3().setFromMatrixColumn(camera.matrixWorld, 0).normalize();
  const up = new THREE.Vector3().setFromMatrixColumn(camera.matrixWorld, 1).normalize();
  const view = camera.getWorldDirection(new THREE.Vector3()).normalize();
  return { right, up, view };
}

/** Builds a configuration-free CAD product tree from a glTF/GLB scene hierarchy. */
export function inferProductStructure(scene: THREE.Object3D): { entries: ProductStructureNode[]; nodes: Map<string, THREE.Object3D> } {
  const hasGeometry = (object: THREE.Object3D): boolean => object instanceof THREE.Mesh || object.children.some(hasGeometry);
  const childrenOf = (object: THREE.Object3D) => object.children.filter((child) => !(child instanceof THREE.Light) && !(child instanceof THREE.Camera) && hasGeometry(child));
  let structuralRoot = scene;
  while (!(structuralRoot instanceof THREE.Mesh)) {
    const children = childrenOf(structuralRoot);
    if (children.length !== 1 || !childrenOf(children[0]).length) break;
    structuralRoot = children[0];
  }
  const entries: ProductStructureNode[] = [];
  const nodes = new Map<string, THREE.Object3D>();
  const visit = (object: THREE.Object3D, nodeId: string, parentNodeId?: string) => {
    const children = childrenOf(object); const isRoot = !parentNodeId;
    const nodeType: ProductStructureNode['nodeType'] = isRoot || children.length ? 'collection' : 'part';
    const label = object.name?.trim() || (isRoot ? '装配体' : `部件 ${entries.length}`);
    entries.push({ nodeId, label, nodeType, ...(parentNodeId ? { parentNodeId } : {}), nodeName: object.name || nodeId, bodyId: nodeId });
    nodes.set(nodeId, object);
    children.forEach((child, index) => visit(child, `${nodeId}/${index}:${stableNodeName(child.name || child.type)}`, nodeId));
  };
  visit(structuralRoot, 'AUTO:ROOT');
  return { entries, nodes };
}

function stableNodeName(value: string): string {
  return value.normalize('NFKC').replace(/[^\p{L}\p{N}_.-]+/gu, '_').replace(/^_+|_+$/g, '').slice(0, 48) || 'node';
}

type LayoutDraft = {
  node: ProductStructureNode;
  box: THREE.Box3;
  angle: number;
  worldDirection: THREE.Vector3;
  distance: number;
};

function relaxScreenOverlaps(drafts: LayoutDraft[], camera: THREE.Camera) {
  const maximumIterations = 28; const margin = .018;
  let iterations = 0; let overlaps = countProjectedOverlaps(drafts, camera, margin);
  while (overlaps > 0 && iterations < maximumIterations) {
    for (const item of drafts) item.distance *= 1.08;
    iterations += 1; overlaps = countProjectedOverlaps(drafts, camera, margin);
  }
  return { iterations, overlaps };
}

function countProjectedOverlaps(drafts: LayoutDraft[], camera: THREE.Camera, margin: number): number {
  const rectangles = drafts.map((item) => projectBox(item.box, item.worldDirection, item.distance, camera));
  let count = 0;
  for (let left = 0; left < rectangles.length; left += 1) for (let right = left + 1; right < rectangles.length; right += 1) {
    if (rectanglesOverlap(rectangles[left], rectangles[right], margin)) count += 1;
  }
  return count;
}

function projectBox(box: THREE.Box3, direction: THREE.Vector3, distance: number, camera: THREE.Camera) {
  const offset = direction.clone().multiplyScalar(distance); const points: THREE.Vector3[] = [];
  for (const x of [box.min.x, box.max.x]) for (const y of [box.min.y, box.max.y]) for (const z of [box.min.z, box.max.z]) {
    points.push(new THREE.Vector3(x, y, z).add(offset).project(camera));
  }
  return {
    minX: Math.min(...points.map((point) => point.x)), maxX: Math.max(...points.map((point) => point.x)),
    minY: Math.min(...points.map((point) => point.y)), maxY: Math.max(...points.map((point) => point.y)),
  };
}

function rectanglesOverlap(left: ReturnType<typeof projectBox>, right: ReturnType<typeof projectBox>, margin: number) {
  return left.minX - margin < right.maxX && left.maxX + margin > right.minX
    && left.minY - margin < right.maxY && left.maxY + margin > right.minY;
}

export function interpolateKeyframes(step: AssemblyStep, progress: number) {
  const frames = [...step.keyframes].sort((a, b) => a.time - b.time);
  if (!frames.length) return { time: 0, position: [0, 0, 0] as Vec3, quaternion: [0, 0, 0, 1] as [number, number, number, number] };
  const duration = frames.at(-1)!.time || 1;
  const time = THREE.MathUtils.clamp(progress, 0, 1) * duration;
  const rightIndex = Math.max(1, frames.findIndex((frame) => frame.time >= time));
  const left = frames[Math.min(rightIndex - 1, frames.length - 1)];
  const right = frames[Math.min(rightIndex, frames.length - 1)];
  const span = right.time - left.time;
  const alpha = span > 0 ? (time - left.time) / span : 0;
  const position = new THREE.Vector3(...left.position).lerp(new THREE.Vector3(...right.position), alpha).toArray() as Vec3;
  const quaternion = new THREE.Quaternion(...left.quaternion).slerp(new THREE.Quaternion(...right.quaternion), alpha).toArray() as [number, number, number, number];
  return { time, position, quaternion };
}

function isDescendant(candidate: THREE.Object3D, ancestor: THREE.Object3D): boolean {
  let current: THREE.Object3D | null = candidate.parent;
  while (current) { if (current === ancestor) return true; current = current.parent; }
  return false;
}
