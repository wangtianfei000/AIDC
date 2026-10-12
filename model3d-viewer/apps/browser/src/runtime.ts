import * as THREE from 'three';
import type { Model3DManifest } from '@aidc/model3d-contracts';

export interface Model3DViewState extends Record<string, unknown> {
  projection: 'perspective' | 'orthographic';
  cameraPosition?: [number, number, number];
  cameraTarget?: [number, number, number];
  cameraZoom?: number;
  selectedComponentId?: string;
  hiddenComponentIds: string[];
  isolatedComponentId?: string;
  transparent: boolean;
  wireframe: boolean;
  jointValues: Record<string, number>;
  selectedJointId?: string;
  ghostEnabled: boolean;
  explosionFactor: number;
  bomLabelsEnabled: boolean;
}

export const defaultModel3DViewState: Model3DViewState = {
  projection: 'perspective', hiddenComponentIds: [], transparent: false, wireframe: false,
  jointValues: {}, ghostEnabled: false, explosionFactor: 0, bomLabelsEnabled: false,
};

export function parseModel3DViewState(value: Record<string, unknown> | undefined): Model3DViewState {
  return {
    projection: value?.projection === 'orthographic' ? 'orthographic' : 'perspective',
    cameraPosition: tuple3(value?.cameraPosition), cameraTarget: tuple3(value?.cameraTarget),
    cameraZoom: typeof value?.cameraZoom === 'number' ? value.cameraZoom : undefined,
    selectedComponentId: typeof value?.selectedComponentId === 'string' ? value.selectedComponentId : undefined,
    hiddenComponentIds: Array.isArray(value?.hiddenComponentIds) ? value.hiddenComponentIds.filter((item): item is string => typeof item === 'string') : [],
    isolatedComponentId: typeof value?.isolatedComponentId === 'string' ? value.isolatedComponentId : undefined,
    transparent: value?.transparent === true, wireframe: value?.wireframe === true,
    jointValues: numericRecord(value?.jointValues),
    selectedJointId: typeof value?.selectedJointId === 'string' ? value.selectedJointId : undefined,
    ghostEnabled: value?.ghostEnabled === true,
    explosionFactor: finiteRange(value?.explosionFactor, 0, 1),
    bomLabelsEnabled: value?.bomLabelsEnabled === true,
  };
}

function finiteRange(value: unknown, minimum: number, maximum: number): number {
  return typeof value === 'number' && Number.isFinite(value) ? Math.min(maximum, Math.max(minimum, value)) : minimum;
}

function numericRecord(value: unknown): Record<string, number> {
  if (!value || typeof value !== 'object' || Array.isArray(value)) return {};
  return Object.fromEntries(Object.entries(value).filter((entry): entry is [string, number] => typeof entry[1] === 'number' && Number.isFinite(entry[1])));
}

function tuple3(value: unknown): [number, number, number] | undefined {
  return Array.isArray(value) && value.length === 3 && value.every((item) => typeof item === 'number')
    ? [value[0], value[1], value[2]] : undefined;
}

export function componentObjects(scene: THREE.Object3D, manifest?: Model3DManifest): Map<string, THREE.Object3D> {
  const result = new Map<string, THREE.Object3D>();
  for (const component of manifest?.components ?? []) {
    const object = scene.getObjectByName(component.nodeName) ?? scene.getObjectByName(component.bodyId);
    if (object) result.set(component.componentId, object);
  }
  return result;
}

export function applyDisplayState(
  scene: THREE.Object3D,
  components: Map<string, THREE.Object3D>,
  state: Pick<Model3DViewState, 'hiddenComponentIds' | 'isolatedComponentId' | 'transparent' | 'wireframe'>,
): void {
  scene.traverse((object) => {
    if (!(object instanceof THREE.Mesh)) return;
    object.visible = true;
    for (const material of materials(object)) {
      material.wireframe = state.wireframe;
      material.transparent = state.transparent || material.opacity < 1;
      material.opacity = state.transparent ? 0.28 : Number(material.userData.aidcOriginalOpacity ?? material.opacity);
      material.depthWrite = !state.transparent;
    }
  });
  for (const id of state.hiddenComponentIds) components.get(id)?.traverse((object) => { if (object instanceof THREE.Mesh) object.visible = false; });
  if (state.isolatedComponentId) {
    const target = components.get(state.isolatedComponentId);
    const allowed = new Set<THREE.Object3D>();
    target?.traverse((object) => allowed.add(object));
    scene.traverse((object) => { if (object instanceof THREE.Mesh && !allowed.has(object)) object.visible = false; });
  }
}

export function prepareMaterials(scene: THREE.Object3D): void {
  scene.traverse((object) => {
    if (!(object instanceof THREE.Mesh)) return;
    const cloned = materials(object).map((material) => {
      const copy = material.clone();
      copy.userData.aidcOriginalOpacity = material.opacity;
      copy.userData.aidcOriginalColor = copy.color.getHex();
      copy.userData.aidcOriginalEmissive = copy.emissive.getHex();
      copy.userData.aidcOriginalEmissiveIntensity = copy.emissiveIntensity;
      copy.userData.aidcOriginalDepthTest = copy.depthTest;
      copy.userData.aidcOriginalDepthWrite = copy.depthWrite;
      copy.envMapIntensity = .7;
      return copy;
    });
    object.material = Array.isArray(object.material) ? cloned : cloned[0];
    object.castShadow = true;
    object.receiveShadow = true;
  });
}

function materials(mesh: THREE.Mesh): THREE.MeshStandardMaterial[] {
  const values = Array.isArray(mesh.material) ? mesh.material : [mesh.material];
  return values.filter((material): material is THREE.MeshStandardMaterial => 'wireframe' in material);
}

export function disposeObject3D(root: THREE.Object3D): void {
  const textures = new Set<THREE.Texture>();
  root.traverse((object) => {
    if (!(object instanceof THREE.Mesh)) return;
    object.geometry.dispose();
    for (const material of Array.isArray(object.material) ? object.material : [object.material]) {
      for (const value of Object.values(material)) if (value instanceof THREE.Texture) textures.add(value);
      material.dispose();
    }
  });
  textures.forEach((texture) => texture.dispose());
  root.clear();
}

export function componentIdFromObject(object: THREE.Object3D | null, components: Map<string, THREE.Object3D>): string | undefined {
  if (!object) return undefined;
  const idsByObject = new Map<THREE.Object3D, string>();
  for (const [componentId, root] of components) idsByObject.set(root, componentId);
  let current: THREE.Object3D | null = object;
  while (current) {
    const componentId = idsByObject.get(current);
    if (componentId) return componentId;
    current = current.parent;
  }
  return undefined;
}
