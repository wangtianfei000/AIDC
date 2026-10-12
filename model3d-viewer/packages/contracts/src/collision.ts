export const INTERFERENCE_REPORT_SCHEMA_VERSION = 'aidc.model3d.interference/0.1' as const;

export type InterferencePrecision = 'MESH_PREVIEW' | 'MESH_EXACT' | 'BREP_EXACT';
export type InterferenceStatus = 'WARNING' | 'CONFIRMED' | 'CLEARED' | 'ALLOWED_CONTACT' | 'UNVERIFIED';

export interface InterferencePairResult {
  pairId?: string;
  componentA: string;
  componentB: string;
  status: InterferenceStatus;
  precision: InterferencePrecision;
  minimumDistance?: number;
  commonVolume?: number;
  tolerance?: number;
  checkedAt?: string;
  pose?: Record<string, number>;
  message?: string;
}

export interface InterferenceReport {
  schemaVersion: typeof INTERFERENCE_REPORT_SCHEMA_VERSION;
  modelId: string;
  modelRevision: string;
  generatedAt?: string;
  results: InterferencePairResult[];
}


export interface ExactInterferenceRequest {
  schemaVersion: typeof INTERFERENCE_REPORT_SCHEMA_VERSION;
  modelId: string;
  modelRevision: string;
  cadSha256: string;
  componentA: { componentId: string; sourceObject: string; matrixWorld: number[] };
  componentB: { componentId: string; sourceObject: string; matrixWorld: number[] };
  tolerance: number;
  pose: Record<string, number>;
}
