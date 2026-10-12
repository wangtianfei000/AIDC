import { describe, expect, it } from 'vitest';
import expectations from '../../../samples/jingang-10axis-system-design/views/diagram-expectations.json';
import snapshot from '../../../samples/jingang-10axis-system-design/g0-parser-result.json';
import {
  diagramEdgeId,
  diagramNodeId,
  diagramPortId,
  validateDiagramModel,
  type DiagramModel,
} from './model';

const validModel: DiagramModel = {
  schemaVersion: '0.1',
  viewId: 'fixture',
  kind: 'interconnection',
  contextElementId: 'system',
  layoutSource: 'auto',
  layout: {
    algorithm: 'elk.layered',
    direction: 'RIGHT',
    routing: 'ORTHOGONAL',
    hierarchy: 'INCLUDE_CHILDREN',
    randomSeed: 1,
  },
  nodes: [
    { id: diagramNodeId('fixture', 'system'), semanticElementId: 'system', role: 'context', parentId: null, typeElementId: null, label: 'System', bounds: null, pinned: false },
    { id: diagramNodeId('fixture', 'source'), semanticElementId: 'source', role: 'part', parentId: diagramNodeId('fixture', 'system'), typeElementId: null, label: 'Source', bounds: null, pinned: false },
    { id: diagramNodeId('fixture', 'sink'), semanticElementId: 'sink', role: 'part', parentId: diagramNodeId('fixture', 'system'), typeElementId: null, label: 'Sink', bounds: null, pinned: false },
  ],
  ports: [
    { id: diagramPortId('fixture', 'source', 'out'), semanticElementId: 'out', semanticOwnerId: 'source', nodeId: diagramNodeId('fixture', 'source'), label: 'out', projection: 'declared', side: 'EAST', order: 0, offset: null, position: null },
    { id: diagramPortId('fixture', 'sink', 'in'), semanticElementId: 'in', semanticOwnerId: 'sink', nodeId: diagramNodeId('fixture', 'sink'), label: 'in', projection: 'typed', side: 'WEST', order: 0, offset: 0.5, position: null },
  ],
  edges: [
    { id: diagramEdgeId('fixture', 'connection'), semanticRelationshipId: 'connection', sourcePortId: diagramPortId('fixture', 'source', 'out'), targetPortId: diagramPortId('fixture', 'sink', 'in'), label: null, routing: 'ORTHOGONAL', sections: [] },
  ],
};

describe('DiagramModel 0.1 contract', () => {
  it('accepts a graph with a context, child nodes, declared and projected ports, and an edge', () => {
    expect(validateDiagramModel(validModel)).toEqual([]);
  });

  it('rejects orphan port and edge references', () => {
    const broken = structuredClone(validModel);
    broken.ports[0].nodeId = 'missing-node';
    broken.edges[0].targetPortId = 'missing-port';
    expect(validateDiagramModel(broken)).toEqual([
      `port node does not exist: ${broken.ports[0].id}`,
      `edge target port does not exist: ${broken.edges[0].id}`,
    ]);
  });

  it('rejects an out-of-range port offset', () => {
    const broken = structuredClone(validModel);
    broken.ports[0].offset = 1.25;
    expect(validateDiagramModel(broken)).toEqual([
      `port offset is invalid: ${broken.ports[0].id}`,
    ]);
  });

  it('freezes whole-system, left-leg, and power-network acceptance views', () => {
    expect(expectations.schemaVersion).toBe('0.1');
    expect(expectations.views.map((view) => view.id)).toEqual([
      'jingang.whole-system',
      'jingang.left-leg',
      'jingang.power-network',
    ]);
    for (const view of expectations.views) {
      expect(new Set(view.expected.nodes).size).toBe(view.expected.nodes.length);
      expect(new Set(view.expected.ports.map((port) => port.path)).size).toBe(view.expected.ports.length);
      expect(new Set(view.expected.connections.map((connection) =>
        `${connection.source}->${connection.target}`)).size).toBe(view.expected.connections.length);
      const portPaths = new Set(view.expected.ports.map((port) => port.path));
      for (const connection of view.expected.connections) {
        expect(portPaths.has(connection.source)).toBe(true);
        expect(portPaths.has(connection.target)).toBe(true);
      }
    }
    expect(expectations.views[0].expected.nodes).toHaveLength(8);
    expect(expectations.views[0].expected.connections).toHaveLength(11);
    expect(expectations.views[1].expected.nodes).toHaveLength(4);
    expect(expectations.views[1].expected.connections).toHaveLength(8);
    expect(expectations.views[2].expected.nodes).toHaveLength(7);
    expect(expectations.views[2].expected.connections).toHaveLength(6);
  });

  it('keeps every acceptance element anchored to the parsed G0 semantic model', () => {
    const elements = snapshot.elements as Array<{
      id: string;
      kind: string;
      name?: string;
      ownerId?: string;
      qualifiedName?: string;
      properties: { typeId?: string };
    }>;
    const relationships = snapshot.relationships as Array<{
      kind: string;
      ownerId?: string;
      properties: { ends?: string[] };
    }>;

    for (const view of expectations.views) {
      const context = elements.find((element) => element.qualifiedName === view.context);
      expect(context, `missing context ${view.context}`).toBeDefined();

      for (const nodeName of view.expected.nodes) {
        expect(elements.some((element) =>
          element.kind === 'PartUsage'
          && element.ownerId === context?.id
          && element.name === nodeName,
        ), `missing node ${view.id}:${nodeName}`).toBe(true);
      }

      for (const port of view.expected.ports) {
        const segments = port.path.split('.');
        const portName = segments.pop()!;
        const ownerPath = segments.join('::');
        const ownerQualifiedName = ownerPath ? `${view.context}::${ownerPath}` : view.context;
        const owner = elements.find((element) => element.qualifiedName === ownerQualifiedName);
        expect(owner, `missing port owner ${view.id}:${ownerQualifiedName}`).toBeDefined();
        if (port.projection === 'declared') {
          expect(elements.some((element) =>
            element.kind === 'PortUsage'
            && element.ownerId === owner?.id
            && element.name === portName,
          ), `missing declared port ${view.id}:${port.path}`).toBe(true);
        } else {
          const definition = 'definition' in port ? port.definition : undefined;
          const definitionPort = elements.find((element) =>
            element.kind === 'PortUsage'
            && element.qualifiedName === definition,
          );
          expect(definitionPort, `missing definition port ${definition}`).toBeDefined();
          expect(owner?.properties.typeId, `wrong typed owner ${view.id}:${port.path}`)
            .toBe(definitionPort?.ownerId);
        }
      }

      for (const connection of view.expected.connections) {
        expect(relationships.some((relationship) =>
          relationship.kind === 'Connection'
          && relationship.ownerId === context?.id
          && relationship.properties.ends?.[0] === connection.source
          && relationship.properties.ends?.[1] === connection.target,
        ), `missing connection ${view.id}:${connection.source}->${connection.target}`).toBe(true);
      }
    }
  });
});
