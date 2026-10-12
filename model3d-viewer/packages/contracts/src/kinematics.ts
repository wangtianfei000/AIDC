import type { JointDefinition, RigidBodyDefinition } from './model-package';

export const KINEMATICS_SCHEMA_VERSION = 'aidc.model3d.kinematics/0.1' as const;

/**
 * Runtime motion semantics kept separately from visual geometry.
 * GLB node names are the stable join points between this document and the scene.
 */
export interface Model3DKinematics {
  schemaVersion: typeof KINEMATICS_SCHEMA_VERSION;
  modelId: string;
  modelRevision: string;
  rootBodyId: string;
  bodies: RigidBodyDefinition[];
  joints: JointDefinition[];
}
