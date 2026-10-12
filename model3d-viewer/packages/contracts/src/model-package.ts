export const MODEL_PACKAGE_SCHEMA_VERSION = 'aidc.model3d.package/0.1' as const;

export type Vec3 = [number, number, number];
export type Quat = [number, number, number, number];

export interface ModelAsset {
  role: 'visual' | 'collision' | 'kinematics' | 'exact';
  uri: string;
  mediaType: string;
  sha256: string;
  byteLength: number;
}

export interface ModelComponent {
  componentId: string;
  label: string;
  nodeName: string;
  bodyId: string;
  geometryStatus: string;
  sourceObject: string;
}

export interface ProductStructureNode {
  nodeId: string;
  label: string;
  nodeType: 'collection' | 'part';
  parentNodeId?: string;
  nodeName: string;
  bodyId: string;
}

export interface AllowedContactRule {
  componentA: string;
  componentB: string;
  reason: string;
}

export interface CollisionConfiguration {
  enabledComponentIds: string[];
  tolerance: number;
  allowedContacts: AllowedContactRule[];
}

export interface RigidBodyDefinition {
  bodyId: string;
  label: string;
  nodeName: string;
  parentBodyId?: string;
  translation: Vec3;
  rotation: Quat;
}

export interface JointDefinition {
  jointId: string;
  type: 'fixed' | 'revolute' | 'prismatic';
  parentBodyId: string;
  childBodyId: string;
  axis: Vec3;
  neutralValue: number;
  zeroOffset: number;
  limit?: {
    lower: number;
    upper: number;
    velocity?: number;
  };
  drive?: {
    channel: string;
    sign: number;
    scale: number;
  };
}

export interface Model3DManifest {
  schemaVersion: typeof MODEL_PACKAGE_SCHEMA_VERSION;
  modelId: string;
  revision: string;
  name: string;
  manufacturingStatus: string;
  coordinateSystem: {
    lengthUnit: 'm';
    angleUnit: 'rad';
    forward: '+X';
    left: '+Y';
    up: '+Z';
    quaternionOrder: 'xyzw';
  };
  source?: {
    cadUri?: string;
    cadSha256?: string;
    sysmlUri?: string;
    sysmlSha256?: string;
    publisherVersion: string;
  };
  assets: ModelAsset[];
  rootBodyId: string;
  bodies: RigidBodyDefinition[];
  components: ModelComponent[];
  productStructure: ProductStructureNode[];
  joints: JointDefinition[];
  collision?: CollisionConfiguration;
  statistics: {
    sourceObjectCount: number;
    exportedGeometryCount: number;
    triangleCount: number;
    componentCount: number;
    jointCount: number;
    visualBounds: { minimum: Vec3; maximum: Vec3 };
  };
}
