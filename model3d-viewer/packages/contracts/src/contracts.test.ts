import { describe, expect, it } from 'vitest';
import {
  ASSEMBLY_SEQUENCE_SCHEMA_VERSION,
  INTERFERENCE_REPORT_SCHEMA_VERSION,
  KINEMATICS_SCHEMA_VERSION,
  MODEL_PACKAGE_SCHEMA_VERSION,
  POSE_PRESETS_SCHEMA_VERSION,
  SIMULATION_FRAME_SCHEMA_VERSION,
} from './index';

describe('Model3D contract versions', () => {
  it('keeps independently versioned public contracts', () => {
    expect(MODEL_PACKAGE_SCHEMA_VERSION).toBe('aidc.model3d.package/0.1');
    expect(ASSEMBLY_SEQUENCE_SCHEMA_VERSION).toBe('aidc.model3d.assembly/0.1');
    expect(SIMULATION_FRAME_SCHEMA_VERSION).toBe('aidc.model3d.simulation-frame/0.1');
    expect(INTERFERENCE_REPORT_SCHEMA_VERSION).toBe('aidc.model3d.interference/0.1');
    expect(POSE_PRESETS_SCHEMA_VERSION).toBe('aidc.model3d.pose-presets/0.1');
    expect(KINEMATICS_SCHEMA_VERSION).toBe('aidc.model3d.kinematics/0.1');
  });
});
