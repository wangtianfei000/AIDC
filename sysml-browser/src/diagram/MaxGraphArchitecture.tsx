import { useEffect, useRef } from 'react';
import {
  FitPlugin,
  Geometry,
  Graph,
  InternalEvent,
  Point,
  Shape,
  ShapeRegistry,
  type AbstractCanvas2D,
  type Cell,
  type CellStyle,
  type PerimeterFunction,
} from '@maxgraph/core';
import type { DiagramEdge, DiagramModel, DiagramPoint } from './model';
import { buildEdgeRenderGeometry } from './renderPath';

type Props = {
  diagram: DiagramModel;
  selectedId?: string;
  onSelect: (id: string) => void;
  canOpenNode: (id: string) => boolean;
  onOpenNode: (id: string) => void;
};

type PathCommand =
  | { kind: 'M' | 'L'; x: number; y: number }
  | { kind: 'Q'; cx: number; cy: number; x: number; y: number };

type SemanticCell = { cell: Cell; kind: 'node' | 'port' };
type CellAction = { semanticId: string; openable: boolean };
type Runtime = { graph: Graph; semanticCells: Map<string, SemanticCell[]> };

const cellActions = new WeakMap<Cell, CellAction>();

/** Parses the deliberately small M/L/Q path subset emitted by renderPath.ts. */
export function parseArchitecturePath(path: string): PathCommand[] {
  const tokens = [...path.matchAll(/([MLQ])\s*(-?\d+(?:\.\d+)?),(-?\d+(?:\.\d+)?)(?:\s+(-?\d+(?:\.\d+)?),(-?\d+(?:\.\d+)?))?/g)];
  return tokens.map((match) => match[1] === 'Q'
    ? { kind: 'Q', cx: Number(match[2]), cy: Number(match[3]), x: Number(match[4]), y: Number(match[5]) }
    : { kind: match[1] as 'M' | 'L', x: Number(match[2]), y: Number(match[3]) });
}

class ArchitectureJumpShape extends Shape {
  override paintEdgeShape(canvas: AbstractCanvas2D, points: Point[]) {
    if (points.length < 3) return;
    canvas.begin();
    canvas.moveTo(points[0].x, points[0].y);
    canvas.quadTo(points[1].x, points[1].y, points.at(-1)!.x, points.at(-1)!.y);
    canvas.stroke();
  }
}

class ArchitectureJumpGapShape extends Shape {
  override paintEdgeShape(canvas: AbstractCanvas2D, points: Point[]) {
    if (points.length < 2) return;
    canvas.begin();
    canvas.moveTo(points[0].x, points[0].y);
    canvas.lineTo(points.at(-1)!.x, points.at(-1)!.y);
    canvas.stroke();
  }
}

ShapeRegistry.add('aidcArchitectureJump', ArchitectureJumpShape);
ShapeRegistry.add('aidcArchitectureJumpGap', ArchitectureJumpGapShape);

const centerPerimeter: PerimeterFunction = (bounds) => new Point(bounds.getCenterX(), bounds.getCenterY());

const readonlyStyle = {
  movable: false,
  resizable: false,
  editable: false,
  deletable: false,
} satisfies CellStyle;

function addSemanticCell(map: Map<string, SemanticCell[]>, semanticId: string, value: SemanticCell) {
  map.set(semanticId, [...(map.get(semanticId) ?? []), value]);
}

function portLabelWidth(label: string) {
  const glyphWidth = [...label].reduce((total, character) =>
    total + (character.codePointAt(0)! > 0x7f ? 8 : 4.8), 0);
  return Math.max(24, Math.ceil(glyphWidth) + 8);
}

function edgePoints(edge: DiagramEdge) {
  return edge.sections.flatMap((section, sectionIndex) => {
    const points = [section.start, ...section.bendPoints, section.end];
    return sectionIndex ? points.slice(1) : points;
  });
}

function edgeGeometry(points: DiagramPoint[]) {
  const geometry = new Geometry();
  geometry.relative = true;
  geometry.sourcePoint = new Point(points[0]?.x ?? 0, points[0]?.y ?? 0);
  geometry.targetPoint = new Point(points.at(-1)?.x ?? 0, points.at(-1)?.y ?? 0);
  geometry.points = points.slice(1, -1).map((point) => new Point(point.x, point.y));
  return geometry;
}

function insertPathEdge(graph: Graph, parent: Cell, id: string, points: DiagramPoint[], style: CellStyle) {
  const cell = graph.insertEdge({ parent, id, value: '', style: { ...readonlyStyle, noEdgeStyle: true, startArrow: 'none', endArrow: 'none', ...style } });
  cell.setGeometry(edgeGeometry(points));
  return cell;
}

function commandPoints(commands: PathCommand[]): DiagramPoint[] {
  return commands.flatMap((command) => command.kind === 'Q'
    ? [{ x: command.cx, y: command.cy }, { x: command.x, y: command.y }]
    : [{ x: command.x, y: command.y }]);
}

export function MaxGraphArchitecture({ diagram, selectedId, onSelect, canOpenNode, onOpenNode }: Props) {
  const containerRef = useRef<HTMLDivElement>(null);
  const runtimeRef = useRef<Runtime | null>(null);
  const onSelectRef = useRef(onSelect);
  const onOpenNodeRef = useRef(onOpenNode);
  const canOpenNodeRef = useRef(canOpenNode);
  onSelectRef.current = onSelect;
  onOpenNodeRef.current = onOpenNode;
  canOpenNodeRef.current = canOpenNode;

  useEffect(() => {
    const container = containerRef.current;
    if (!container) return;
    const graph = new Graph(container);
    graph.setConnectable(false);
    graph.setCellsMovable(false);
    graph.setCellsResizable(false);
    graph.setCellsEditable(false);
    graph.setCellsDeletable(false);
    graph.setCellsBendable(false);
    graph.setCellsDisconnectable(false);
    graph.setCellsSelectable(false);
    graph.setPanning(false);
    graph.setTooltips(true);
    const parent = graph.getDefaultParent();
    const semanticCells = new Map<string, SemanticCell[]>();
    const portCells = new Map<string, Cell>();
    runtimeRef.current = { graph, semanticCells };
    const contextNode = diagram.nodes.find((node) => node.role === 'context');
    const parts = diagram.nodes.filter((node) => node.role === 'part' && node.bounds);
    const paths = buildEdgeRenderGeometry(diagram);

    graph.batchUpdate(() => {
      for (const node of parts) {
        const bounds = node.bounds!;
        const openable = canOpenNodeRef.current(node.semanticElementId);
        const cell = graph.insertVertex({
          parent,
          id: node.id,
          value: node.label,
          position: [bounds.x, bounds.y],
          size: [bounds.width, bounds.height],
          style: {
            ...readonlyStyle,
            shape: 'rectangle',
            rounded: true,
            arcSize: 14,
            fillColor: '#ffffff',
            strokeColor: '#bdc8d5',
            strokeWidth: 1.3,
            shadow: true,
            fontColor: '#263449',
            fontSize: 11,
            fontStyle: 1,
            align: 'center',
            verticalAlign: 'middle',
          },
        });
        cellActions.set(cell, { semanticId: node.semanticElementId, openable });
        addSemanticCell(semanticCells, node.semanticElementId, { cell, kind: 'node' });
      }

      for (const port of diagram.ports) {
        const point = port.position;
        if (!point) continue;
        const portCell = graph.insertVertex({
          parent,
          id: port.id,
          value: '',
          position: [point.x - 5, point.y - 5],
          size: [10, 10],
          style: { ...readonlyStyle, shape: 'ellipse', perimeter: centerPerimeter, fillColor: '#ffffff', strokeColor: '#27877d', strokeWidth: 2 },
        });
        portCells.set(port.id, portCell);
        cellActions.set(portCell, { semanticId: port.semanticElementId, openable: false });
        addSemanticCell(semanticCells, port.semanticElementId, { cell: portCell, kind: 'port' });

        const contextPort = port.nodeId === contextNode?.id;
        const labelToStart = contextPort || port.side === 'WEST' || port.side === 'NORTH';
        const owner = parts.find((node) => node.id === port.nodeId)?.bounds;
        const titleY = owner ? owner.y + owner.height / 2 + 4 : undefined;
        const naturalLabelY = point.y - 7;
        const labelY = titleY !== undefined && Math.abs(naturalLabelY - titleY) < 18
          ? titleY + (point.y <= titleY ? -20 : 20)
          : naturalLabelY;
        const labelWidth = portLabelWidth(port.label);
        const labelCell = graph.insertVertex({
          parent,
          id: `${port.id}:label`,
          value: port.label,
          position: [labelToStart ? point.x + 9 : point.x - 9 - labelWidth, labelY - 10],
          size: [labelWidth, 14],
          style: { ...readonlyStyle, shape: 'label', fillColor: 'none', strokeColor: 'none', fontColor: '#617184', fontSize: 8, align: labelToStart ? 'left' : 'right', verticalAlign: 'middle' },
        });
        cellActions.set(labelCell, { semanticId: port.semanticElementId, openable: false });
      }

      for (const edge of diagram.edges) {
        const path = paths.paths.get(edge.id);
        const points = edgePoints(edge);
        if (!path || points.length < 2) continue;
        const source = portCells.get(edge.sourcePortId);
        const target = portCells.get(edge.targetPortId);
        const cell = graph.insertEdge({
          parent,
          id: edge.id,
          value: '',
          source,
          target,
          style: { ...readonlyStyle, noEdgeStyle: true, rounded: false, strokeColor: '#8797aa', strokeWidth: 1.5, startArrow: 'none', endArrow: 'none' },
        });
        cell.setGeometry(edgeGeometry(points));
      }

      for (const [id, path] of paths.jumpPaths) {
        const commands = parseArchitecturePath(path);
        const points = commandPoints(commands);
        if (points.length < 2) continue;
        insertPathEdge(graph, parent, `${id}:gap`, [points[0], points.at(-1)!], { shape: 'aidcArchitectureJumpGap', strokeColor: '#fbfcfd', strokeWidth: 6 });
        insertPathEdge(graph, parent, `${id}:bridge`, points, { shape: 'aidcArchitectureJump', strokeColor: '#8797aa', strokeWidth: 1.5 });
      }
    });

    const click = (_sender: unknown, event: { getProperty: (name: string) => unknown }) => {
      const cell = event.getProperty('cell') as Cell | undefined;
      const action = cell ? cellActions.get(cell) : undefined;
      if (action) onSelectRef.current(action.semanticId);
    };
    const doubleClick = (_sender: unknown, event: { getProperty: (name: string) => unknown }) => {
      const cell = event.getProperty('cell') as Cell | undefined;
      const action = cell ? cellActions.get(cell) : undefined;
      if (action?.openable) onOpenNodeRef.current(action.semanticId);
    };
    graph.addListener(InternalEvent.CLICK, click);
    graph.addListener(InternalEvent.DOUBLE_CLICK, doubleClick);
    const fit = () => {
      graph.getPlugin<FitPlugin>('fit')?.fit({ border: 24 });
      graph.center(true, true);
    };
    requestAnimationFrame(fit);
    const observer = new ResizeObserver(() => { if (graph.getGraphBounds().width) fit(); });
    observer.observe(container);
    return () => {
      observer.disconnect();
      graph.removeListener(click);
      graph.removeListener(doubleClick);
      runtimeRef.current = null;
      graph.destroy();
      container.replaceChildren();
    };
  }, [diagram]);

  useEffect(() => {
    const runtime = runtimeRef.current;
    if (!runtime) return;
    for (const [semanticId, entries] of runtime.semanticCells) {
      const selected = semanticId === selectedId;
      for (const { cell, kind } of entries) {
        if (kind === 'node') {
          runtime.graph.setCellStyles('fillColor', selected ? '#e8f5f2' : '#ffffff', [cell]);
          runtime.graph.setCellStyles('strokeColor', selected ? '#0d8b7c' : '#bdc8d5', [cell]);
          runtime.graph.setCellStyles('strokeWidth', selected ? 2 : 1.3, [cell]);
        } else if (kind === 'port') {
          runtime.graph.setCellStyles('fillColor', selected ? '#27877d' : '#ffffff', [cell]);
          runtime.graph.setCellStyles('strokeColor', selected ? '#145f58' : '#27877d', [cell]);
          runtime.graph.setCellStyles('strokeWidth', selected ? 2.5 : 2, [cell]);
        }
      }
    }
  }, [selectedId]);

  return <div className="architecture-canvas architecture-maxgraph" role="img" aria-label="架构结构图（只读）">
    <div ref={containerRef} className="architecture-maxgraph-container" />
    <span className="architecture-readonly-badge">只读 · maxGraph</span>
  </div>;
}
