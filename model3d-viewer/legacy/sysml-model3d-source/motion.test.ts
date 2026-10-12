import { describe, expect, it } from 'vitest';
import type { Model3DManifest } from '@aidc/model3d-contracts';
import { clampJointValue, interpolatePose, isAtJointLimit, standardPosePresets } from './motion';

const manifest = {
  joints: [
    { jointId: 'ACT-L-HIP-PITCH', type: 'revolute', neutralValue: 0, zeroOffset: 0, parentBodyId: 'a', childBodyId: 'b', axis: [0, 0, 1], limit: { lower: -1, upper: 1 } },
    { jointId: 'ACT-L-KNEE-PITCH', type: 'revolute', neutralValue: .1, zeroOffset: 0, parentBodyId: 'b', childBodyId: 'c', axis: [0, 0, 1], limit: { lower: .1, upper: 1.1 } },
  ],
} as Model3DManifest;

describe('Model3D motion helpers', () => {
  it('interpolates smoothly and clamps joint values', () => {
    expect(interpolatePose({ a: 0 }, { a: 10 }, .5).a).toBe(5);
    expect(interpolatePose({ a: 0 }, { a: 10 }, 2).a).toBe(10);
    expect(clampJointValue(manifest.joints[0], 3)).toBe(1);
    expect(isAtJointLimit(manifest.joints[0], 1)).toBe('upper');
  });

  it('creates bounded, model-driven review presets', () => {
    const presets = standardPosePresets(manifest);
    expect(presets.map((item) => item.id)).toEqual(['neutral', 'crouch', 'stride', 'inspect']);
    for (const preset of presets) for (const joint of manifest.joints) {
      expect(preset.pose[joint.jointId]).toBeGreaterThanOrEqual(joint.limit!.lower);
      expect(preset.pose[joint.jointId]).toBeLessThanOrEqual(joint.limit!.upper);
    }
  });
});
