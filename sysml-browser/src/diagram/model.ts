export const DIAGRAM_MODEL_SCHEMA_VERSION = '0.1' as const;

export type DiagramKind = 'interconnection';
export type DiagramDirection = 'RIGHT' | 'DOWN';
export type DiagramLayoutSource = 'auto' | 'graphic-metadata' | 'mixed';
export type DiagramPortSide = 'NORTH' | 'EAST' | 'SOUTH' | 'WEST' | 'UNDEFINED';
export type DiagramPortProjection = 'declared' | 'typed';
export type DiagramRouting = 'ORTHOGONAL' | 'POLYLINE' | 'SPLINE';

export interface DiagramPoint {
  x: number;
  y: number;
}

export interface DiagramBounds extends DiagramPoint {
  width: number;
  height: number;
}

export interface DiagramNode {
  id: string;
  semanticElementId: string;
  role: 'context' | 'part';
  parentId: string | null;
  typeElementId: string | null;
  label: string;
  bounds: DiagramBounds | null;
  pinned: boolean;
}

export interface DiagramPort {
  id: string;
  semanticElementId: string;
  semanticOwnerId: string;
  nodeId: string;
  label: string;
  projection: DiagramPortProjection;
  side: DiagramPortSide;
  order: number;
  offset: number | null;
  position: DiagramPoint | null;
}

export interface DiagramEdgeSection {
  start: DiagramPoint;
  end: DiagramPoint;
  bendPoints: DiagramPoint[];
}

export interface DiagramEdge {
  id: string;
  semanticRelationshipId: string;
  sourcePortId: string;
  targetPortId: string;
  label: string | null;
  routing: DiagramRouting;
  sections: DiagramEdgeSection[];
}

export interface DiagramLayoutOptions {
  algorithm: 'elk.layered';
  direction: DiagramDirection;
  routing: DiagramRouting;
  hierarchy: 'INCLUDE_CHILDREN';
  randomSeed: number;
}

export interface DiagramModel {
  schemaVersion: typeof DIAGRAM_MODEL_SCHEMA_VERSION;
  viewId: string;
  kind: DiagramKind;
  contextElementId: string;
  layoutSource: DiagramLayoutSource;
  layout: DiagramLayoutOptions;
  nodes: DiagramNode[];
  ports: DiagramPort[];
  edges: DiagramEdge[];
}

export function diagramNodeId(viewId: string, semanticElementId: string) {
  return `${viewId}:node:${semanticElementId}`;
}

export function diagramPortId(
  viewId: string,
  semanticOwnerId: string,
  semanticPortId: string,
) {
  return `${viewId}:port:${semanticOwnerId}:${semanticPortId}`;
}

export function diagramEdgeId(viewId: string, semanticRelationshipId: string) {
  return `${viewId}:edge:${semanticRelationshipId}`;
}

export function validateDiagramModel(model: DiagramModel): string[] {
  const errors: string[] = [];
  if (model.schemaVersion !== DIAGRAM_MODEL_SCHEMA_VERSION) errors.push('unsupported schemaVersion');
  if (!model.viewId) errors.push('viewId is required');
  if (!model.contextElementId) errors.push('contextElementId is required');

  const nodeIds = new Set<string>();
  const portIds = new Set<string>();
  const edgeIds = new Set<string>();
  for (const node of model.nodes) {
    if (nodeIds.has(node.id)) errors.push(`duplicate node id: ${node.id}`);
    nodeIds.add(node.id);
    if (node.bounds && (node.bounds.width <= 0 || node.bounds.height <= 0)) {
      errors.push(`node has invalid bounds: ${node.id}`);
    }
  }
  const contextNodes = model.nodes.filter((node) =>
    node.role === 'context' && node.semanticElementId === model.contextElementId);
  if (contextNodes.length !== 1) errors.push('exactly one context node is required');
  for (const node of model.nodes) {
    if (node.parentId !== null && !nodeIds.has(node.parentId)) {
      errors.push(`node parent does not exist: ${node.id}`);
    }
  }
  for (const port of model.ports) {
    if (portIds.has(port.id)) errors.push(`duplicate port id: ${port.id}`);
    portIds.add(port.id);
    if (!nodeIds.has(port.nodeId)) errors.push(`port node does not exist: ${port.id}`);
    if (port.order < 0 || !Number.isInteger(port.order)) errors.push(`port order is invalid: ${port.id}`);
    if (port.offset !== null && (port.offset < 0 || port.offset > 1)) {
      errors.push(`port offset is invalid: ${port.id}`);
    }
  }
  for (const edge of model.edges) {
    if (edgeIds.has(edge.id)) errors.push(`duplicate edge id: ${edge.id}`);
    edgeIds.add(edge.id);
    if (!portIds.has(edge.sourcePortId)) errors.push(`edge source port does not exist: ${edge.id}`);
    if (!portIds.has(edge.targetPortId)) errors.push(`edge target port does not exist: ${edge.id}`);
  }
  return errors;
}
