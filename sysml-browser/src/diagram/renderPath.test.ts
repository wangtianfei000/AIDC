import { describe, expect, it } from 'vitest';
import type { DiagramEdge, DiagramModel } from './model';
import { buildEdgeRenderGeometry, buildEdgeRenderPaths } from './renderPath';

function edge(id: string, sourcePortId: string, targetPortId: string, points: Array<{ x: number; y: number }>): DiagramEdge {
  return {
    id,
    semanticRelationshipId: id,
    sourcePortId,
    targetPortId,
    label: null,
    routing: 'ORTHOGONAL',
    sections: [{ start: points[0], bendPoints: points.slice(1, -1), end: points[points.length - 1] }],
  };
}

function model(edges: DiagramEdge[]): DiagramModel {
  return {
    schemaVersion: '0.1', viewId: 'render-test', kind: 'interconnection', contextElementId: 'context',
    layoutSource: 'auto', layout: { algorithm: 'elk.layered', direction: 'RIGHT', routing: 'ORTHOGONAL', hierarchy: 'INCLUDE_CHILDREN', randomSeed: 1 },
    nodes: [], ports: [], edges,
  };
}

describe('schematic connection crossings', () => {
  it('adds a jump arc when unrelated connections cross', () => {
    const fixture = model([
      edge('horizontal', 'a', 'b', [{ x: 0, y: 50 }, { x: 100, y: 50 }]),
      edge('vertical', 'c', 'd', [{ x: 50, y: 0 }, { x: 50, y: 100 }]),
    ]);
    const paths = buildEdgeRenderPaths(fixture);
    const geometry = buildEdgeRenderGeometry(fixture);
    expect(paths.get('horizontal')).toContain('Q50,36 56,50');
    expect(paths.get('vertical')).not.toContain('Q');
    expect([...geometry.jumpPaths.values()]).toEqual(['M44,50 Q50,36 56,50']);
  });

  it('does not add a jump when connections share an endpoint port', () => {
    const fixture = model([
      edge('horizontal', 'shared', 'b', [{ x: 0, y: 50 }, { x: 100, y: 50 }]),
      edge('vertical', 'shared', 'd', [{ x: 50, y: 0 }, { x: 50, y: 100 }]),
    ]);
    const paths = buildEdgeRenderPaths(fixture);
    expect(paths.get('horizontal')).not.toContain('Q');
    expect(paths.get('vertical')).not.toContain('Q');
    expect(buildEdgeRenderGeometry(fixture).jumpPaths.size).toBe(0);
  });
});
