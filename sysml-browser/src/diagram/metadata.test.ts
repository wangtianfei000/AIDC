import { describe, expect, it } from 'vitest';
import snapshotJson from '../../../samples/jingang-10axis-system-design/views/graphic-metadata-parser-result.json';
import referenceJson from '../../../samples/jingang-10axis-system-design/views/g3-reference-graphic-metadata-parser-result.json';
import type { ModelSnapshot } from '../model/types';
import { layoutDiagram } from './layout';
import { applyGraphicMetadata } from './metadata';
import { projectInterconnection } from './projection';

const snapshot = snapshotJson as ModelSnapshot;
const reference = referenceJson as ModelSnapshot;

function fixture(source: ModelSnapshot = snapshot) {
  const context = source.elements.find((item) => item.qualifiedName === 'DiagramGraphicMetadataFixture::GraphicMetadataFixture');
  if (!context) throw new Error('Missing GraphicMetadata fixture context');
  return projectInterconnection(source, {
    viewId: 'graphic-metadata-fixture',
    contextElementId: context.id,
    selection: { mode: 'owned', depth: 1 },
  });
}

function elementId(qualifiedName: string) {
  const element = snapshot.elements.find((item) => item.qualifiedName === qualifiedName);
  if (!element) throw new Error(`Missing fixture element ${qualifiedName}`);
  return element.id;
}

describe('@GraphicMetadata compatibility', () => {
  it('maps reference metadata to DiagramModel coordinates, ports, label and routing', () => {
    const applied = applyGraphicMetadata(snapshot, fixture().model);
    const source = applied.model.nodes.find((node) => node.label === 'source')!;
    const sink = applied.model.nodes.find((node) => node.label === 'sink')!;
    const output = applied.model.ports.find((port) => port.label === 'outputPort')!;
    const input = applied.model.ports.find((port) => port.label === 'inputPort')!;

    expect(applied.diagnostics).toEqual([]);
    expect(applied.model.layoutSource).toBe('graphic-metadata');
    expect(source.bounds).toEqual({ x: 60, y: 100, width: 140, height: 80 });
    expect(sink.bounds).toEqual({ x: 440, y: 100, width: 140, height: 80 });
    expect(output.position).toEqual({ x: 200, y: 140 });
    expect(output.side).toBe('EAST');
    expect(input.position).toEqual({ x: 440, y: 140 });
    expect(input.side).toBe('WEST');
    expect(applied.model.edges[0]).toMatchObject({ label: 'fixture', routing: 'ORTHOGONAL' });
  });

  it('preserves a complete reference layout without invoking ELK coordinates', async () => {
    const applied = applyGraphicMetadata(snapshot, fixture().model);
    const result = await layoutDiagram(applied.model);
    expect(result.nodes.find((node) => node.role === 'context')?.bounds)
      .toEqual({ x: 0, y: 0, width: 640, height: 320 });
    expect(result.nodes.find((node) => node.label === 'source')?.bounds)
      .toEqual({ x: 60, y: 100, width: 140, height: 80 });
    expect(result.edges[0].sections[0]).toEqual({
      start: { x: 200, y: 140 },
      bendPoints: [],
      end: { x: 440, y: 140 },
    });
  });

  it('pins valid nodes and lets ELK fill a partial layout without overlap', async () => {
    const partial = structuredClone(snapshot);
    const sinkId = elementId('DiagramGraphicMetadataFixture::GraphicMetadataFixture::sink');
    partial.elements = partial.elements.filter((item) => !(item.kind === 'Annotation' && item.ownerId === sinkId));
    const applied = applyGraphicMetadata(partial, fixture(partial).model);
    const result = await layoutDiagram(applied.model);
    const source = result.nodes.find((node) => node.label === 'source')!;
    const sink = result.nodes.find((node) => node.label === 'sink')!;

    expect(applied.model.layoutSource).toBe('mixed');
    expect(source.bounds).toEqual({ x: 60, y: 100, width: 140, height: 80 });
    expect(sink.bounds).not.toBeNull();
    expect(sink.bounds!.x >= source.bounds!.x + source.bounds!.width
      || sink.bounds!.y >= source.bounds!.y + source.bounds!.height
      || source.bounds!.x >= sink.bounds!.x + sink.bounds!.width
      || source.bounds!.y >= sink.bounds!.y + sink.bounds!.height).toBe(true);
  });

  it('reports invalid node dimensions and falls back for that target', () => {
    const invalid = structuredClone(snapshot);
    const sourceId = elementId('DiagramGraphicMetadataFixture::GraphicMetadataFixture::source');
    const annotation = invalid.elements.find((item) => item.kind === 'Annotation' && item.ownerId === sourceId)!;
    (annotation.properties.metadataAssignments as Record<string, unknown>).graphicWidth = -1;
    const applied = applyGraphicMetadata(invalid, fixture(invalid).model);

    expect(applied.model.layoutSource).toBe('mixed');
    expect(applied.model.nodes.find((node) => node.label === 'source')?.pinned).toBe(false);
    expect(applied.diagnostics.join(' ')).toContain('无效');
  });

  it('accepts the reference project metadata vocabulary on a real architecture context', async () => {
    const context = reference.elements.find((item) => item.qualifiedName === 'ArchitectureViewTest::BrakeSystemAssembly')!;
    const projection = projectInterconnection(reference, {
      viewId: 'reference-brake-system',
      contextElementId: context.id,
      selection: { mode: 'owned', depth: 1 },
    });
    const applied = applyGraphicMetadata(reference, projection.model);
    const result = await layoutDiagram(applied.model);

    expect(projection.diagnostics).toHaveLength(2);
    expect(applied.model.layoutSource).toBe('graphic-metadata');
    expect(applied.model.nodes.filter((node) => node.role === 'part' && node.pinned)).toHaveLength(6);
    expect(result.edges).toHaveLength(5);
    expect(result.edges.every((edge) => edge.sections.length > 0)).toBe(true);
  });
});
