import type { Quat, Vec3 } from './model-package';

export const ASSEMBLY_SEQUENCE_SCHEMA_VERSION = 'aidc.model3d.assembly/0.1' as const;

export interface AssemblyKeyframe {
  time: number;
  /** Offset from the assembled pose, expressed in the model coordinate system. */
  position: Vec3;
  quaternion: Quat;
}

export interface AssemblyStep {
  stepId: string;
  label: string;
  componentIds: string[];
  dependsOn: string[];
  pathStatus: 'AUTO_SUGGESTED' | 'MANUAL_DEMO' | 'ENGINEERING_VERIFIED';
  description?: string;
  tool?: { label: string; direction: Vec3 };
  keyframes: AssemblyKeyframe[];
}

export interface AssemblySequence {
  schemaVersion: typeof ASSEMBLY_SEQUENCE_SCHEMA_VERSION;
  modelId: string;
  modelRevision: string;
  steps: AssemblyStep[];
}
