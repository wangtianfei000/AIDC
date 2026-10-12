import type { ModelElement, ModelSnapshot } from '../model/types';
import type {
  DiagramBounds,
  DiagramEdgeSection,
  DiagramModel,
  DiagramPoint,
  DiagramPortSide,
  DiagramRouting,
} from './model';

export interface GraphicMetadataResult {
  model: DiagramModel;
  diagnostics: string[];
  appliedTargets: number;
}

type Assignments = Record<string, unknown>;

function assignments(annotation: ModelElement): Assignments | undefined {
  const value = annotation.properties.metadataAssignments;
  return value && typeof value === 'object' && !Array.isArray(value)
    ? value as Assignments
    : undefined;
}

function finite(value: unknown) {
  return typeof value === 'number' && Number.isFinite(value) ? value : undefined;
}

function side(value: unknown): DiagramPortSide | undefined {
  if (typeof value !== 'string') return undefined;
  const normalized = value.trim().toUpperCase();
  const aliases: Record<string, DiagramPortSide> = {
    TOP: 'NORTH', RIGHT: 'EAST', BOTTOM: 'SOUTH', LEFT: 'WEST',
  };
  if (aliases[normalized]) return aliases[normalized];
  return ['NORTH', 'EAST', 'SOUTH', 'WEST'].includes(normalized)
    ? normalized as DiagramPortSide
    : undefined;
}

function routing(value: unknown): DiagramRouting | undefined {
  if (typeof value !== 'string') return undefined;
  const normalized = value.trim().toUpperCase();
  if (normalized === 'ORTHOGONAL' || normalized === 'POLYLINE' || normalized === 'SPLINE') return normalized;
  return normalized === 'MANHATTAN' ? 'ORTHOGONAL' : undefined;
}

function points(value: unknown): DiagramPoint[] | undefined {
  let candidate = value;
  if (typeof candidate === 'string') {
    try { candidate = JSON.parse(candidate); } catch { return undefined; }
  }
  if (!Array.isArray(candidate)) return undefined;
  const result = candidate.map((item) => {
    if (!item || typeof item !== 'object') return undefined;
    const x = finite((item as Record<string, unknown>).x);
    const y = finite((item as Record<string, unknown>).y);
    return x === undefined || y === undefined ? undefined : { x, y };
  });
  return result.every((item): item is DiagramPoint => !!item) ? result : undefined;
}

function sectionFromPoints(value: DiagramPoint[]): DiagramEdgeSection[] {
  if (value.length < 2) return [];
  return [{ start: value[0], end: value[value.length - 1], bendPoints: value.slice(1, -1) }];
}

function portPosition(bounds: DiagramBounds, portSide: DiagramPortSide, offset: number): DiagramPoint {
  if (portSide === 'WEST') return { x: bounds.x, y: bounds.y + bounds.height * offset };
  if (portSide === 'EAST') return { x: bounds.x + bounds.width, y: bounds.y + bounds.height * offset };
  if (portSide === 'NORTH') return { x: bounds.x + bounds.width * offset, y: bounds.y };
  return { x: bounds.x + bounds.width * offset, y: bounds.y + bounds.height };
}

/** Applies the conservative @GraphicMetadata subset used by the reference project. */
export function applyGraphicMetadata(snapshot: ModelSnapshot, source: DiagramModel): GraphicMetadataResult {
  const model = structuredClone(source);
  const diagnostics: string[] = [];
  const byOwner = new Map<string, Assignments>();
  for (const element of snapshot.elements) {
    if (element.kind !== 'Annotation' || element.properties.annotationName !== 'GraphicMetadata' || !element.ownerId) continue;
    const value = assignments(element);
    if (!value) {
      diagnostics.push(`GraphicMetadata ${element.id} 缺少 metadataAssignments`);
      continue;
    }
    byOwner.set(element.ownerId, { ...(byOwner.get(element.ownerId) ?? {}), ...value });
  }

  let appliedTargets = 0;
  for (const node of model.nodes) {
    const value = byOwner.get(node.semanticElementId);
    if (!value) continue;
    const width = finite(value.graphicWidth);
    const height = finite(value.graphicHeight);
    const x = finite(value.graphicX);
    const y = finite(value.graphicY);
    const validSize = width !== undefined && height !== undefined && width > 0 && height > 0;
    const validOrigin = node.role === 'context' || (x !== undefined && y !== undefined);
    if (!validSize || !validOrigin) {
      diagnostics.push(`GraphicMetadata ${node.label} 的节点坐标或尺寸不完整/无效，已交给自动布局`);
      continue;
    }
    node.bounds = { x: x ?? 0, y: y ?? 0, width, height };
    node.pinned = true;
    appliedTargets += 1;
  }

  const nodeById = new Map(model.nodes.map((node) => [node.id, node]));
  for (const port of model.ports) {
    const value = byOwner.get(port.semanticElementId);
    if (!value) continue;
    const owner = nodeById.get(port.nodeId);
    const portSide = side(value.portPosition);
    const offset = finite(value.portOffset);
    const localX = finite(value.graphicX);
    const localY = finite(value.graphicY);
    if (value.portPosition !== undefined && !portSide) diagnostics.push(`GraphicMetadata ${port.label} 的 portPosition 无效`);
    if (offset !== undefined && (offset < 0 || offset > 1)) diagnostics.push(`GraphicMetadata ${port.label} 的 portOffset 必须位于 0..1`);
    if (portSide) port.side = portSide;
    if (offset !== undefined && offset >= 0 && offset <= 1) port.offset = offset;
    if (owner?.bounds && localX !== undefined && localY !== undefined) {
      port.position = { x: owner.bounds.x + localX, y: owner.bounds.y + localY };
    } else if (owner?.bounds && portSide && port.offset !== null) {
      port.position = portPosition(owner.bounds, portSide, port.offset);
    } else {
      diagnostics.push(`GraphicMetadata ${port.label} 的端口位置不完整，已交给自动布局`);
      continue;
    }
    appliedTargets += 1;
  }

  for (const edge of model.edges) {
    const value = byOwner.get(edge.semanticRelationshipId);
    if (!value) continue;
    if (typeof value.labelText === 'string') edge.label = value.labelText;
    edge.routing = routing(value.pathType) ?? routing(value.routingStyle) ?? edge.routing;
    const path = points(value.pathPoints);
    if (value.pathPoints !== undefined && !path) diagnostics.push(`GraphicMetadata ${edge.semanticRelationshipId} 的 pathPoints 无效`);
    if (path) edge.sections = sectionFromPoints(path);
    appliedTargets += 1;
  }

  const partNodes = model.nodes.filter((node) => node.role === 'part');
  const anyMetadata = appliedTargets > 0;
  const complete = partNodes.length > 0 && partNodes.every((node) => node.pinned);
  model.layoutSource = complete ? 'graphic-metadata' : anyMetadata ? 'mixed' : 'auto';
  return { model, diagnostics, appliedTargets };
}
