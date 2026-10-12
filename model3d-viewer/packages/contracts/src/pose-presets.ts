export const POSE_PRESETS_SCHEMA_VERSION = 'aidc.model3d.pose-presets/0.1' as const;

export interface Model3DPosePreset {
  poseId: string;
  name: string;
  jointValues: Record<string, number>;
  updatedAt: string;
}

export interface Model3DPosePresetDocument {
  schemaVersion: typeof POSE_PRESETS_SCHEMA_VERSION;
  modelId: string;
  modelRevision: string;
  poses: Model3DPosePreset[];
}
