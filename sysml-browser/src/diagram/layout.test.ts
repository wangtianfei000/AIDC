import { describe, expect, it } from 'vitest';
import expectations from '../../../samples/jingang-10axis-system-design/views/diagram-expectations.json';
import snapshotJson from '../../../samples/jingang-10axis-system-design/g1-parser-result.json';
import type { ModelSnapshot } from '../model/types';
import type { DiagramBounds } from './model';
import { layoutDiagram } from './layout';
import { projectInterconnection, type DiagramSelection } from './projection';

const snapshot = snapshotJson as ModelSnapshot;

function elementId(qualifiedName: string) {
  const element = snapshot.elements.find((item) => item.qualifiedName === qualifiedName);
  if (!element) throw new Error(`Missing fixture element: ${qualifiedName}`);
  return element.id;
}

function overlaps(left: DiagramBounds, right: DiagramBounds) {
  return left.x < right.x + right.width
    && left.x + left.width > right.x
    && left.y < right.y + right.height
    && left.y + left.height > right.y;
}

function segmentCrossesInterior(
  start: { x: number; y: number },
  end: { x: number; y: number },
  bounds: DiagramBounds,
) {
  const epsilon = 0.01;
  if (Math.abs(start.y - end.y) < epsilon) {
    const minimumX = Math.min(start.x, end.x);
    const maximumX = Math.max(start.x, end.x);
    return start.y > bounds.y + epsilon
      && start.y < bounds.y + bounds.height - epsilon
      && maximumX > bounds.x + epsilon
      && minimumX < bounds.x + bounds.width - epsilon;
  }
  if (Math.abs(start.x - end.x) < epsilon) {
    const minimumY = Math.min(start.y, end.y);
    const maximumY = Math.max(start.y, end.y);
    return start.x > bounds.x + epsilon
      && start.x < bounds.x + bounds.width - epsilon
      && maximumY > bounds.y + epsilon
      && minimumY < bounds.y + bounds.height - epsilon;
  }
  return true;
}

function project(view: typeof expectations.views[number]) {
  const selection: DiagramSelection = view.selection.mode === 'connected'
    ? { mode: 'connected', seedElementId: elementId(`${view.context}::${view.selection.seed}`) }
    : { mode: 'owned', depth: view.selection.depth };
  return projectInterconnection(snapshot, {
    viewId: view.id,
    contextElementId: elementId(view.context),
    selection,
    direction: view.layout.direction as 'RIGHT' | 'DOWN',
    routing: 'ORTHOGONAL',
  }).model;
}

describe('ELK DiagramModel layout', () => {
  for (const expectedView of expectations.views) {
    it(`lays out ${expectedView.id} with boundary ports and orthogonal sections`, async () => {
      const result = await layoutDiagram(project(expectedView));
      const parts = result.nodes.filter((node) => node.role === 'part');
      for (const node of parts) {
        expect(node.bounds?.width).toBeGreaterThan(0);
        expect(node.bounds?.height).toBeGreaterThan(0);
      }
      for (let leftIndex = 0; leftIndex < parts.length; leftIndex += 1) {
        for (let rightIndex = leftIndex + 1; rightIndex < parts.length; rightIndex += 1) {
          expect(overlaps(parts[leftIndex].bounds!, parts[rightIndex].bounds!)).toBe(false);
        }
      }
      for (const port of result.ports) {
        expect(port.position).not.toBeNull();
        if (port.nodeId === result.nodes.find((node) => node.role === 'context')?.id) continue;
        const bounds = result.nodes.find((node) => node.id === port.nodeId)!.bounds!;
        const onBoundary = Math.abs(port.position!.x - bounds.x) < 0.01
          || Math.abs(port.position!.x - (bounds.x + bounds.width)) < 0.01
          || Math.abs(port.position!.y - bounds.y) < 0.01
          || Math.abs(port.position!.y - (bounds.y + bounds.height)) < 0.01;
        expect(onBoundary, `${port.id} is not on its node boundary`).toBe(true);
      }
      for (const edge of result.edges) {
        expect(edge.routing).toBe('ORTHOGONAL');
        expect(edge.sections.length).toBeGreaterThan(0);
        for (const section of edge.sections) {
          const points = [section.start, ...section.bendPoints, section.end];
          for (let index = 1; index < points.length; index += 1) {
            const horizontal = Math.abs(points[index - 1].y - points[index].y) < 0.01;
            const vertical = Math.abs(points[index - 1].x - points[index].x) < 0.01;
            expect(horizontal || vertical, `${edge.id} contains a diagonal segment`).toBe(true);
            for (const node of parts) {
              expect(
                segmentCrossesInterior(points[index - 1], points[index], node.bounds!),
                `${edge.id} crosses ${node.label}`,
              ).toBe(false);
            }
          }
        }
        const source = result.ports.find((port) => port.id === edge.sourcePortId)!;
        const target = result.ports.find((port) => port.id === edge.targetPortId)!;
        expect(edge.sections[0].start).toEqual(source.position);
        expect(edge.sections[edge.sections.length - 1].end).toEqual(target.position);
      }
    });
  }

  it('is deterministic for the same model, direction, order, and random seed', async () => {
    const model = project(expectations.views[0]);
    const first = await layoutDiagram(model);
    const second = await layoutDiagram(model);
    expect(second).toEqual(first);
  });

  it('supports DOWN direction without changing semantic identity', async () => {
    const model = project(expectations.views[0]);
    const down = await layoutDiagram(model, 'DOWN');
    expect(down.layout.direction).toBe('DOWN');
    expect(down.nodes.map((node) => node.semanticElementId))
      .toEqual(model.nodes.map((node) => node.semanticElementId));
    expect(down.edges.map((edge) => edge.semanticRelationshipId))
      .toEqual(model.edges.map((edge) => edge.semanticRelationshipId));
  });
});
