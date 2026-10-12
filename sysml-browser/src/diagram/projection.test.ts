import { describe, expect, it } from 'vitest';
import expectations from '../../../samples/jingang-10axis-system-design/views/diagram-expectations.json';
import snapshotJson from '../../../samples/jingang-10axis-system-design/g1-parser-result.json';
import type { ModelSnapshot } from '../model/types';
import { validateDiagramModel } from './model';
import { projectInterconnection, type DiagramSelection } from './projection';

const snapshot = snapshotJson as ModelSnapshot;

function elementId(qualifiedName: string) {
  const element = snapshot.elements.find((item) => item.qualifiedName === qualifiedName);
  if (!element) throw new Error(`Missing fixture element: ${qualifiedName}`);
  return element.id;
}

describe('interconnection DiagramProjection', () => {
  for (const expectedView of expectations.views) {
    it(`projects ${expectedView.id} from semantic elements and relationships`, () => {
      const contextElementId = elementId(expectedView.context);
      let selection: DiagramSelection;
      if (expectedView.selection.mode === 'connected') {
        selection = {
          mode: 'connected',
          seedElementId: elementId(`${expectedView.context}::${expectedView.selection.seed}`),
        };
      } else {
        selection = { mode: 'owned', depth: expectedView.selection.depth };
      }
      const result = projectInterconnection(snapshot, {
        viewId: expectedView.id,
        contextElementId,
        selection,
        direction: expectedView.layout.direction as 'RIGHT' | 'DOWN',
        routing: expectedView.layout.routing as 'ORTHOGONAL',
      });

      expect(result.diagnostics).toEqual([]);
      expect(validateDiagramModel(result.model)).toEqual([]);
      expect(result.model.nodes.filter((node) => node.role === 'part').map((node) => node.label))
        .toEqual(expectedView.expected.nodes);
      expect(result.model.ports).toHaveLength(expectedView.expected.ports.length);
      expect(result.model.edges).toHaveLength(expectedView.expected.connections.length);

      const expectedTypedCount = expectedView.expected.ports.filter((port) => port.projection === 'typed').length;
      expect(result.model.ports.filter((port) => port.projection === 'typed')).toHaveLength(expectedTypedCount);
      for (const edge of result.model.edges) {
        const sourcePort = result.model.ports.find((port) => port.id === edge.sourcePortId);
        const targetPort = result.model.ports.find((port) => port.id === edge.targetPortId);
        expect(sourcePort).toBeDefined();
        expect(targetPort).toBeDefined();
        const relationship = snapshot.relationships.find((item) => item.id === edge.semanticRelationshipId)!;
        expect(relationship.sourceIds).toEqual([sourcePort?.semanticElementId]);
        expect(relationship.targetIds).toEqual([targetPort?.semanticElementId]);
      }
    });
  }

  it('uses the instance owner in projected typed-port identity', () => {
    const contextElementId = elementId('JingangNative10Architecture::JingangNative10::leftLeg');
    const result = projectInterconnection(snapshot, {
      viewId: 'typed-port-identity',
      contextElementId,
      selection: { mode: 'owned', depth: 1 },
    });
    const powerPorts = result.model.ports.filter((port) =>
      port.label === 'powerIn' && port.projection === 'typed');
    expect(powerPorts).toHaveLength(4);
    expect(new Set(powerPorts.map((port) => port.semanticElementId)).size).toBe(1);
    expect(new Set(powerPorts.map((port) => port.semanticOwnerId)).size).toBe(4);
    expect(new Set(powerPorts.map((port) => port.id)).size).toBe(4);
  });
});
