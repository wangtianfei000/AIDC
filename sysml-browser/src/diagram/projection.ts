import type { ModelElement, ModelRelationship, ModelSnapshot } from '../model/types';
import {
  DIAGRAM_MODEL_SCHEMA_VERSION,
  diagramEdgeId,
  diagramNodeId,
  diagramPortId,
  type DiagramDirection,
  type DiagramModel,
  type DiagramPort,
  type DiagramRouting,
} from './model';

export type DiagramSelection =
  | { mode: 'owned'; depth?: number }
  | { mode: 'connected'; seedElementId: string };

export interface InterconnectionProjectionOptions {
  viewId: string;
  contextElementId: string;
  selection?: DiagramSelection;
  direction?: DiagramDirection;
  routing?: DiagramRouting;
}

export interface DiagramProjectionResult {
  model: DiagramModel;
  diagnostics: string[];
}

interface ResolvedEndpoint {
  path: string;
  owner: ModelElement;
  port: ModelElement;
  projection: 'declared' | 'typed';
}

function stringProperty(element: ModelElement, name: string) {
  const value = element.properties[name];
  return typeof value === 'string' ? value : undefined;
}

function connectionEnds(relationship: ModelRelationship): string[] {
  const value = relationship.properties.ends;
  return Array.isArray(value) ? value.filter((item): item is string => typeof item === 'string') : [];
}

function directChild(elements: ModelElement[], ownerId: string, kind: string, name: string) {
  return elements.find((element) =>
    element.ownerId === ownerId && element.kind === kind && element.name === name);
}

function resolveEndpoint(
  snapshot: ModelSnapshot,
  context: ModelElement,
  path: string,
): ResolvedEndpoint | undefined {
  const segments = path.split('.').filter(Boolean);
  if (!segments.length) return undefined;
  const portName = segments.pop()!;
  let owner = context;
  for (const segment of segments) {
    const part = directChild(snapshot.elements, owner.id, 'PartUsage', segment);
    if (!part) return undefined;
    owner = part;
  }

  const declared = directChild(snapshot.elements, owner.id, 'PortUsage', portName);
  if (declared) return { path, owner, port: declared, projection: 'declared' };

  const typeId = stringProperty(owner, 'typeId');
  if (!typeId) return undefined;
  const typed = directChild(snapshot.elements, typeId, 'PortUsage', portName);
  return typed ? { path, owner, port: typed, projection: 'typed' } : undefined;
}

function partAtEndpoint(snapshot: ModelSnapshot, context: ModelElement, path: string) {
  const first = path.split('.').filter(Boolean)[0];
  return first ? directChild(snapshot.elements, context.id, 'PartUsage', first) : undefined;
}

function ownedParts(snapshot: ModelSnapshot, context: ModelElement, depth: number) {
  const result: ModelElement[] = [];
  const visit = (ownerId: string, remaining: number) => {
    if (remaining <= 0) return;
    for (const element of snapshot.elements) {
      if (element.ownerId !== ownerId || element.kind !== 'PartUsage') continue;
      result.push(element);
      visit(element.id, remaining - 1);
    }
  };
  visit(context.id, depth);
  return result;
}

export function projectInterconnection(
  snapshot: ModelSnapshot,
  options: InterconnectionProjectionOptions,
): DiagramProjectionResult {
  const diagnostics: string[] = [];
  const context = snapshot.elements.find((element) => element.id === options.contextElementId);
  if (!context) throw new Error(`Diagram context does not exist: ${options.contextElementId}`);

  const selection = options.selection ?? { mode: 'owned', depth: 1 };
  const relationships = snapshot.relationships.filter((relationship) =>
    relationship.kind === 'Connection' && relationship.ownerId === context.id);

  let parts: ModelElement[];
  let selectedRelationships = relationships;
  if (selection.mode === 'connected') {
    const seed = snapshot.elements.find((element) => element.id === selection.seedElementId);
    if (!seed || seed.ownerId !== context.id || seed.kind !== 'PartUsage') {
      throw new Error(`Connected-view seed is not a direct part of the context: ${selection.seedElementId}`);
    }
    selectedRelationships = relationships.filter((relationship) =>
      connectionEnds(relationship).some((path) => partAtEndpoint(snapshot, context, path)?.id === seed.id));
    const selectedPartIds = new Set<string>([seed.id]);
    for (const relationship of selectedRelationships) {
      for (const path of connectionEnds(relationship)) {
        const part = partAtEndpoint(snapshot, context, path);
        if (part) selectedPartIds.add(part.id);
      }
    }
    parts = [
      seed,
      ...snapshot.elements.filter((element) => selectedPartIds.has(element.id) && element.id !== seed.id),
    ];
  } else {
    parts = ownedParts(snapshot, context, selection.depth ?? 1);
  }

  const selectedPartIds = new Set(parts.map((part) => part.id));
  const contextNodeId = diagramNodeId(options.viewId, context.id);
  const nodes: DiagramModel['nodes'] = [{
    id: contextNodeId,
    semanticElementId: context.id,
    role: 'context',
    parentId: null,
    typeElementId: stringProperty(context, 'typeId') ?? null,
    label: context.name ?? context.qualifiedName ?? context.kind,
    bounds: null,
    pinned: false,
  }];
  for (const part of parts) {
    const semanticParentId = part.ownerId && selectedPartIds.has(part.ownerId)
      ? part.ownerId
      : context.id;
    nodes.push({
      id: diagramNodeId(options.viewId, part.id),
      semanticElementId: part.id,
      role: 'part',
      parentId: diagramNodeId(options.viewId, semanticParentId),
      typeElementId: stringProperty(part, 'typeId') ?? null,
      label: part.name ?? part.qualifiedName ?? part.kind,
      bounds: null,
      pinned: false,
    });
  }

  const portById = new Map<string, DiagramPort>();
  const resolvedEdges: Array<{
    relationship: ModelRelationship;
    source: ResolvedEndpoint;
    target: ResolvedEndpoint;
  }> = [];
  const addPort = (endpoint: ResolvedEndpoint) => {
    const nodeId = diagramNodeId(options.viewId, endpoint.owner.id);
    if (endpoint.owner.id !== context.id && !selectedPartIds.has(endpoint.owner.id)) return undefined;
    const id = diagramPortId(options.viewId, endpoint.owner.id, endpoint.port.id);
    if (!portById.has(id)) {
      portById.set(id, {
        id,
        semanticElementId: endpoint.port.id,
        semanticOwnerId: endpoint.owner.id,
        nodeId,
        label: endpoint.port.name ?? endpoint.path,
        projection: endpoint.projection,
        side: 'UNDEFINED',
        order: 0,
        offset: null,
        position: null,
      });
    }
    return id;
  };

  for (const relationship of selectedRelationships) {
    const ends = connectionEnds(relationship);
    if (ends.length !== 2) {
      diagnostics.push(`Connection ${relationship.id} must have exactly two ends`);
      continue;
    }
    const source = resolveEndpoint(snapshot, context, ends[0]);
    const target = resolveEndpoint(snapshot, context, ends[1]);
    if (!source || !target) {
      diagnostics.push(`Connection ${relationship.id} has an unresolved diagram endpoint: ${ends.join(' -> ')}`);
      continue;
    }
    if (!addPort(source) || !addPort(target)) continue;
    resolvedEdges.push({ relationship, source, target });
  }

  const ports = [...portById.values()].sort((left, right) =>
    left.nodeId.localeCompare(right.nodeId) || left.label.localeCompare(right.label) || left.id.localeCompare(right.id));
  const orderByNode = new Map<string, number>();
  for (const port of ports) {
    const order = orderByNode.get(port.nodeId) ?? 0;
    port.order = order;
    orderByNode.set(port.nodeId, order + 1);
  }

  const routing = options.routing ?? 'ORTHOGONAL';
  const edges: DiagramModel['edges'] = resolvedEdges.map(({ relationship, source, target }) => ({
    id: diagramEdgeId(options.viewId, relationship.id),
    semanticRelationshipId: relationship.id,
    sourcePortId: diagramPortId(options.viewId, source.owner.id, source.port.id),
    targetPortId: diagramPortId(options.viewId, target.owner.id, target.port.id),
    label: null,
    routing,
    sections: [],
  }));

  return {
    model: {
      schemaVersion: DIAGRAM_MODEL_SCHEMA_VERSION,
      viewId: options.viewId,
      kind: 'interconnection',
      contextElementId: context.id,
      layoutSource: 'auto',
      layout: {
        algorithm: 'elk.layered',
        direction: options.direction ?? 'RIGHT',
        routing,
        hierarchy: 'INCLUDE_CHILDREN',
        randomSeed: 1,
      },
      nodes,
      ports,
      edges,
    },
    diagnostics,
  };
}
