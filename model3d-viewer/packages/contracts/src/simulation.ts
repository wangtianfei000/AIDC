import type { Quat, Vec3 } from './model-package';

export const SIMULATION_FRAME_SCHEMA_VERSION = 'aidc.model3d.simulation-frame/0.1' as const;

export interface SimulationFrame {
  schemaVersion: typeof SIMULATION_FRAME_SCHEMA_VERSION;
  time: number;
  base?: { position: Vec3; quaternion: Quat };
  joints: Record<string, number>;
  contacts?: Array<{ a: string; b: string; point: Vec3; force?: Vec3 }>;
  scalars?: Record<string, number>;
  events?: string[];
}
