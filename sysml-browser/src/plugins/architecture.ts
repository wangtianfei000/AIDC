import type { ModelElement, ModelRelationship, ModelSnapshot } from '../model/types';
import type { ModelQuery } from '../model/query';

export const ARCHITECTURE_KINDS = new Set([
  'PartDefinition', 'PartUsage', 'PortDefinition', 'PortUsage',
  'InterfaceDefinition', 'InterfaceUsage', 'ConnectorDefinition', 'ConnectorUsage',
]);

export interface ArchitectureModel {
  elements: ModelElement[];
  parts: ModelElement[];
  ports: ModelElement[];
  interfaces: ModelElement[];
  connectors: ModelElement[];
  connections: ModelRelationship[];
  containers: ModelElement[];
}

export function buildArchitectureModel(snapshot: ModelSnapshot, query: ModelQuery): ArchitectureModel {
  const elements = snapshot.elements.filter((item) => ARCHITECTURE_KINDS.has(item.kind));
  const parts = elements.filter((item) => item.kind.startsWith('Part'));
  const ports = elements.filter((item) => item.kind.startsWith('Port'));
  const interfaces = elements.filter((item) => item.kind.startsWith('Interface'));
  const connectors = elements.filter((item) => item.kind.startsWith('Connector'));
  const connections = snapshot.relationships.filter((item) => /connection|connector/i.test(item.kind));
  const containers = snapshot.elements.filter((item) =>
    ['PartDefinition', 'PartUsage', 'Package'].includes(item.kind) &&
    hasDrawableArchitectureContent(query, item.id),
  );
  return { elements, parts, ports, interfaces, connectors, connections, containers };
}

export function hasDrawableArchitectureContent(query: ModelQuery, elementId: string): boolean {
  return query.children(elementId).some((child) => child.kind === 'PartUsage');
}

export function architectureChildren(query: ModelQuery, ownerId: string | null) {
  return query.children(ownerId).filter((item) => item.kind === 'Package' || ARCHITECTURE_KINDS.has(item.kind));
}
