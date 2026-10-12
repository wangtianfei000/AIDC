import * as THREE from 'three';
import type { JointDefinition, Model3DManifest } from '@aidc/model3d-contracts';

type JointState = {
  definition: JointDefinition;
  body: THREE.Object3D;
  basePosition: THREE.Vector3;
  baseQuaternion: THREE.Quaternion;
  value: number;
};

/** Applies joint motion to named rigid-body nodes; it never depends on GLB node order. */
export class RobotRig {
  private readonly joints = new Map<string, JointState>();

  constructor(readonly manifest: Model3DManifest, readonly scene: THREE.Object3D) {
    for (const definition of manifest.joints) {
      const child = scene.getObjectByName(definition.childBodyId) ?? scene.getObjectByName(
        manifest.bodies.find((body) => body.bodyId === definition.childBodyId)?.nodeName ?? '',
      );
      if (!child) throw new Error(`GLB 中缺少关节刚体: ${definition.childBodyId}`);
      const basePosition = child.position.clone();
      const baseQuaternion = child.quaternion.clone().normalize();
      this.joints.set(definition.jointId, { definition, body: child, basePosition, baseQuaternion, value: definition.neutralValue });
    }
  }

  get jointIds(): string[] {
    return [...this.joints.keys()];
  }

  getValue(jointId: string): number {
    return this.requireJoint(jointId).value;
  }

  getDefinition(jointId: string): JointDefinition {
    return this.requireJoint(jointId).definition;
  }

  getBody(jointId: string): THREE.Object3D {
    return this.requireJoint(jointId).body;
  }

  getPose(): Record<string, number> {
    return Object.fromEntries([...this.joints].map(([jointId, state]) => [jointId, state.value]));
  }

  setJoint(jointId: string, requestedValue: number): number {
    const state = this.requireJoint(jointId);
    const { definition } = state;
    const lower = definition.limit?.lower ?? Number.NEGATIVE_INFINITY;
    const upper = definition.limit?.upper ?? Number.POSITIVE_INFINITY;
    const value = definition.type === 'fixed' ? definition.neutralValue : THREE.MathUtils.clamp(requestedValue, lower, upper);
    const signedValue = definition.zeroOffset + (value - definition.neutralValue) * (definition.drive?.sign ?? 1) * (definition.drive?.scale ?? 1);
    const axis = new THREE.Vector3(...definition.axis).normalize();
    state.body.position.copy(state.basePosition);
    state.body.quaternion.copy(state.baseQuaternion);
    if (definition.type === 'revolute') {
      const delta = new THREE.Quaternion().setFromAxisAngle(axis, signedValue);
      state.body.quaternion.multiply(delta).normalize();
    } else if (definition.type === 'prismatic') {
      state.body.position.addScaledVector(axis, signedValue);
    }
    state.value = value;
    return value;
  }

  setPose(values: Readonly<Record<string, number>>): Record<string, number> {
    const applied: Record<string, number> = {};
    for (const [jointId, value] of Object.entries(values)) {
      if (!this.joints.has(jointId) || !Number.isFinite(value)) continue;
      applied[jointId] = this.setJoint(jointId, value);
    }
    return applied;
  }

  reset(): void {
    for (const state of this.joints.values()) {
      state.body.position.copy(state.basePosition);
      state.body.quaternion.copy(state.baseQuaternion);
      state.value = state.definition.neutralValue;
    }
  }

  private requireJoint(jointId: string): JointState {
    const state = this.joints.get(jointId);
    if (!state) throw new Error(`未知关节: ${jointId}`);
    return state;
  }
}
