import * as THREE from 'three';
import { GLTFLoader } from 'three/examples/jsm/loaders/GLTFLoader.js';
import {
  KINEMATICS_SCHEMA_VERSION,
  type Model3DKinematics,
  type Model3DManifest,
} from '@aidc/model3d-contracts';
import { RobotRig } from './kinematics';

export interface LoadedRobotModel {
  manifest: Model3DManifest;
  displayRoot: THREE.Group;
  modelRoot: THREE.Object3D;
  rig: RobotRig;
}

export interface LoadRobotModelOptions {
  signal?: AbortSignal;
  onProgress?: (loaded: number, total?: number) => void;
}

export async function loadRobotModel(manifestUrl: string, options: LoadRobotModelOptions = {}): Promise<LoadedRobotModel> {
  const response = await fetch(manifestUrl, { signal: options.signal });
  if (!response.ok) throw new Error(`模型清单加载失败: ${response.status} ${response.statusText}`);
  const manifest = (await response.json()) as Model3DManifest;
  const kinematicsAsset = manifest.assets.find((candidate) => candidate.role === 'kinematics');
  if (kinematicsAsset) {
    const kinematicsUrl = new URL(kinematicsAsset.uri, new URL(manifestUrl, window.location.href)).toString();
    const kinematicsResponse = await fetch(kinematicsUrl, { signal: options.signal });
    if (!kinematicsResponse.ok) throw new Error(`运动学文件加载失败: ${kinematicsResponse.status} ${kinematicsResponse.statusText}`);
    applyKinematics(manifest, await kinematicsResponse.json() as Model3DKinematics);
  }
  const asset = manifest.assets.find((candidate) => candidate.role === 'visual');
  if (!asset) throw new Error('模型包没有 visual 资产');
  const assetUrl = new URL(asset.uri, new URL(manifestUrl, window.location.href)).toString();
  const gltf = await new Promise<Awaited<ReturnType<GLTFLoader['loadAsync']>>>((resolve, reject) => {
    new GLTFLoader().load(assetUrl, resolve, (event) => options.onProgress?.(event.loaded, event.total || undefined), reject);
  });
  if (options.signal?.aborted) throw new DOMException('模型加载已取消', 'AbortError');
  // Model contract is Z-up; Three.js viewers conventionally render Y-up.
  const displayRoot = new THREE.Group();
  displayRoot.name = 'aidc-z-up-to-y-up';
  displayRoot.rotation.x = -Math.PI / 2;
  displayRoot.add(gltf.scene);
  const rig = new RobotRig(manifest, gltf.scene);
  return { manifest, displayRoot, modelRoot: gltf.scene, rig };
}

/** Makes the separately versioned kinematics document authoritative at runtime. */
export function applyKinematics(manifest: Model3DManifest, kinematics: Model3DKinematics): Model3DManifest {
  if (kinematics.schemaVersion !== KINEMATICS_SCHEMA_VERSION) throw new Error(`不支持的运动学文件版本: ${kinematics.schemaVersion}`);
  if (kinematics.modelId !== manifest.modelId || kinematics.modelRevision !== manifest.revision) {
    throw new Error('运动学文件与模型版本不一致');
  }
  manifest.rootBodyId = kinematics.rootBodyId;
  manifest.bodies = kinematics.bodies;
  manifest.joints = kinematics.joints;
  return manifest;
}
