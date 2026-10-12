import type { DiagramEdge, DiagramModel, DiagramPoint } from './model';

interface Segment {
  edgeId: string;
  sectionIndex: number;
  segmentIndex: number;
  start: DiagramPoint;
  end: DiagramPoint;
}

interface Jump {
  x: number;
  y: number;
}

const JUMP_RADIUS = 6;
// A quadratic curve reaches half the control-point height at its midpoint.
const JUMP_HEIGHT = 14;

function edgePoints(edge: DiagramEdge) {
  return edge.sections.map((section) => [section.start, ...section.bendPoints, section.end]);
}

function segments(edge: DiagramEdge): Segment[] {
  return edgePoints(edge).flatMap((points, sectionIndex) => points.slice(0, -1).map((start, segmentIndex) => ({
    edgeId: edge.id,
    sectionIndex,
    segmentIndex,
    start,
    end: points[segmentIndex + 1],
  })));
}

function horizontal(segment: Segment) {
  return segment.start.y === segment.end.y && segment.start.x !== segment.end.x;
}

function vertical(segment: Segment) {
  return segment.start.x === segment.end.x && segment.start.y !== segment.end.y;
}

function between(value: number, first: number, second: number, margin: number) {
  return value > Math.min(first, second) + margin && value < Math.max(first, second) - margin;
}

function sharesEndpoint(left: DiagramEdge, right: DiagramEdge) {
  return left.sourcePortId === right.sourcePortId
    || left.sourcePortId === right.targetPortId
    || left.targetPortId === right.sourcePortId
    || left.targetPortId === right.targetPortId;
}

function segmentKey(segment: Pick<Segment, 'edgeId' | 'sectionIndex' | 'segmentIndex'>) {
  return `${segment.edgeId}:${segment.sectionIndex}:${segment.segmentIndex}`;
}

/**
 * Builds orthogonal SVG paths with schematic jump arcs. Only geometrical crossings
 * between edges that do not share a semantic port receive a jump.
 */
export function buildEdgeRenderGeometry(model: DiagramModel) {
  const jumps = new Map<string, Jump[]>();
  const edgeSegments = new Map(model.edges.map((edge) => [edge.id, segments(edge)]));

  for (let leftIndex = 0; leftIndex < model.edges.length; leftIndex += 1) {
    const left = model.edges[leftIndex];
    for (let rightIndex = leftIndex + 1; rightIndex < model.edges.length; rightIndex += 1) {
      const right = model.edges[rightIndex];
      if (sharesEndpoint(left, right)) continue;
      for (const leftSegment of edgeSegments.get(left.id) ?? []) {
        for (const rightSegment of edgeSegments.get(right.id) ?? []) {
          const horizontalSegment = horizontal(leftSegment) ? leftSegment : horizontal(rightSegment) ? rightSegment : undefined;
          const verticalSegment = vertical(leftSegment) ? leftSegment : vertical(rightSegment) ? rightSegment : undefined;
          if (!horizontalSegment || !verticalSegment || horizontalSegment.edgeId === verticalSegment.edgeId) continue;
          const x = verticalSegment.start.x;
          const y = horizontalSegment.start.y;
          if (!between(x, horizontalSegment.start.x, horizontalSegment.end.x, JUMP_RADIUS)
            || !between(y, verticalSegment.start.y, verticalSegment.end.y, 1)) continue;
          const key = segmentKey(horizontalSegment);
          const values = jumps.get(key) ?? [];
          if (!values.some((point) => point.x === x && point.y === y)) values.push({ x, y });
          jumps.set(key, values);
        }
      }
    }
  }

  const jumpPaths = new Map<string, string>();
  const paths = new Map(model.edges.map((edge) => {
    const path = edgePoints(edge).map((points, sectionIndex) => {
      let value = `M${points[0].x},${points[0].y}`;
      for (let segmentIndex = 0; segmentIndex < points.length - 1; segmentIndex += 1) {
        const start = points[segmentIndex];
        const end = points[segmentIndex + 1];
        const key = segmentKey({ edgeId: edge.id, sectionIndex, segmentIndex });
        const crossings = [...(jumps.get(key) ?? [])].sort((left, right) =>
          end.x >= start.x ? left.x - right.x : right.x - left.x);
        if (start.y === end.y) {
          const direction = end.x >= start.x ? 1 : -1;
          for (const crossing of crossings) {
            const jump = `M${crossing.x - direction * JUMP_RADIUS},${crossing.y} Q${crossing.x},${crossing.y - JUMP_HEIGHT} ${crossing.x + direction * JUMP_RADIUS},${crossing.y}`;
            jumpPaths.set(`${key}:${crossing.x}:${crossing.y}`, jump);
            value += ` L${crossing.x - direction * JUMP_RADIUS},${crossing.y}`;
            value += ` Q${crossing.x},${crossing.y - JUMP_HEIGHT} ${crossing.x + direction * JUMP_RADIUS},${crossing.y}`;
          }
        }
        value += ` L${end.x},${end.y}`;
      }
      return value;
    }).join(' ');
    return [edge.id, path] as const;
  }));
  return { paths, jumpPaths };
}

export function buildEdgeRenderPaths(model: DiagramModel) {
  return buildEdgeRenderGeometry(model).paths;
}
