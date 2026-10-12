import type { JointDefinition, Model3DManifest } from '@aidc/model3d-contracts';

export type JointPose = Record<string, number>;
export type PosePreset = { id: string; label: string; pose: JointPose };

export function neutralPose(manifest: Model3DManifest): JointPose {
  return Object.fromEntries(manifest.joints.map((joint) => [joint.jointId, joint.neutralValue]));
}

export function clampJointValue(joint: JointDefinition, value: number): number {
  if (joint.type === 'fixed') return joint.neutralValue;
  return Math.min(joint.limit?.upper ?? Number.POSITIVE_INFINITY, Math.max(joint.limit?.lower ?? Number.NEGATIVE_INFINITY, value));
}

export function interpolatePose(from: JointPose, to: JointPose, progress: number): JointPose {
  const t = Math.min(1, Math.max(0, progress));
  const ids = new Set([...Object.keys(from), ...Object.keys(to)]);
  return Object.fromEntries([...ids].map((id) => {
    const start = from[id] ?? to[id] ?? 0;
    const end = to[id] ?? start;
    return [id, start + (end - start) * (t * t * (3 - 2 * t))];
  }));
}

function toward(joint: JointDefinition, fraction: number, side: 'lower' | 'upper'): number {
  const edge = side === 'lower' ? joint.limit?.lower : joint.limit?.upper;
  return edge == null ? joint.neutralValue : joint.neutralValue + (edge - joint.neutralValue) * fraction;
}

export function standardPosePresets(manifest: Model3DManifest): PosePreset[] {
  const neutral = neutralPose(manifest);
  const crouch = { ...neutral };
  const stride = { ...neutral };
  const inspect = { ...neutral };
  for (const joint of manifest.joints) {
    if (/KNEE-PITCH/.test(joint.jointId)) crouch[joint.jointId] = toward(joint, .7, 'upper');
    if (/HIP-PITCH/.test(joint.jointId)) crouch[joint.jointId] = toward(joint, .32, 'lower');
    if (/ANKLE-PITCH/.test(joint.jointId)) crouch[joint.jointId] = toward(joint, .28, 'upper');

    if (/L-HIP-PITCH/.test(joint.jointId)) stride[joint.jointId] = toward(joint, .58, 'upper');
    if (/R-HIP-PITCH/.test(joint.jointId)) stride[joint.jointId] = toward(joint, .58, 'lower');
    if (/L-KNEE-PITCH/.test(joint.jointId)) stride[joint.jointId] = toward(joint, .42, 'upper');
    if (/R-KNEE-PITCH/.test(joint.jointId)) stride[joint.jointId] = toward(joint, .18, 'upper');

    if (/HEAD-YAW/.test(joint.jointId)) inspect[joint.jointId] = toward(joint, .7, 'upper');
    if (/NECK-PITCH/.test(joint.jointId)) inspect[joint.jointId] = toward(joint, .55, 'upper');
  }
  return [
    { id: 'neutral', label: '中性姿态', pose: neutral },
    { id: 'crouch', label: '下蹲检查', pose: crouch },
    { id: 'stride', label: '迈步检查', pose: stride },
    { id: 'inspect', label: '头部检查', pose: inspect },
  ];
}

export function jointLabel(jointId: string): string {
  const labels: Record<string, string> = {
    'L-HIP-ROLL': '左髋横滚', 'L-HIP-PITCH': '左髋俯仰', 'L-KNEE-PITCH': '左膝俯仰', 'L-ANKLE-PITCH': '左踝俯仰',
    'R-HIP-ROLL': '右髋横滚', 'R-HIP-PITCH': '右髋俯仰', 'R-KNEE-PITCH': '右膝俯仰', 'R-ANKLE-PITCH': '右踝俯仰',
    'NECK-PITCH': '颈部俯仰', 'HEAD-YAW': '头部偏航',
  };
  const short = jointId.replace(/^ACT-/, '');
  return labels[short] ?? short;
}

export function isAtJointLimit(joint: JointDefinition, value: number, tolerance = 1e-7): 'lower' | 'upper' | undefined {
  if (joint.limit?.lower != null && Math.abs(value - joint.limit.lower) <= tolerance) return 'lower';
  if (joint.limit?.upper != null && Math.abs(value - joint.limit.upper) <= tolerance) return 'upper';
  return undefined;
}
