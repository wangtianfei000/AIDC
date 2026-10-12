import type { ELK, ElkExtendedEdge, ElkNode, ElkPort } from 'elkjs';
import type {
  DiagramBounds,
  DiagramDirection,
  DiagramEdgeSection,
  DiagramModel,
  DiagramPoint,
  DiagramPort,
  DiagramPortSide,
} from './model';

const NODE_WIDTH = 184;
const NODE_MIN_HEIGHT = 82;
const PORT_SIZE = 8;
const CONTEXT_PORT_NODE_WIDTH = 72;
const CONTEXT_PORT_NODE_HEIGHT = 30;

let elkPromise: Promise<ELK> | undefined;

async function getElk() {
  elkPromise ??= import('elkjs/lib/elk.bundled.js').then(({ default: ElkConstructor }) =>
    new ElkConstructor());
  return elkPromise;
}

function inputSide(direction: DiagramDirection): DiagramPortSide {
  return direction === 'RIGHT' ? 'WEST' : 'NORTH';
}

function outputSide(direction: DiagramDirection): DiagramPortSide {
  return direction === 'RIGHT' ? 'EAST' : 'SOUTH';
}

function portSides(model: DiagramModel, direction: DiagramDirection) {
  const sources = new Set(model.edges.map((edge) => edge.sourcePortId));
  const targets = new Set(model.edges.map((edge) => edge.targetPortId));
  return new Map(model.ports.map((port) => {
    if (port.side !== 'UNDEFINED') return [port.id, port.side] as const;
    const source = sources.has(port.id);
    const target = targets.has(port.id);
    const side = source && !target ? outputSide(direction)
      : target && !source ? inputSide(direction)
        : port.side === 'UNDEFINED' ? outputSide(direction) : port.side;
    return [port.id, side] as const;
  }));
}

function toElkPort(port: DiagramPort, side: DiagramPortSide): ElkPort {
  return {
    id: port.id,
    width: PORT_SIZE,
    height: PORT_SIZE,
    layoutOptions: {
      'elk.port.side': side,
      'elk.port.index': String(port.order),
    },
  };
}

function contextPortNodeId(portId: string) {
  return `context-port-node:${portId}`;
}

function nodeHeight(portCount: number) {
  return Math.max(NODE_MIN_HEIGHT, 48 + portCount * 18);
}

function clamp(value: number, minimum = 0, maximum = 1) {
  return Math.max(minimum, Math.min(maximum, value));
}

function positionOnBoundary(bounds: DiagramBounds, side: DiagramPortSide, offset: number): DiagramPoint {
  if (side === 'WEST') return { x: bounds.x, y: bounds.y + bounds.height * offset };
  if (side === 'EAST') return { x: bounds.x + bounds.width, y: bounds.y + bounds.height * offset };
  if (side === 'NORTH') return { x: bounds.x + bounds.width * offset, y: bounds.y };
  return { x: bounds.x + bounds.width * offset, y: bounds.y + bounds.height };
}

function orthogonalSection(start: DiagramPoint, end: DiagramPoint, direction: DiagramDirection): DiagramEdgeSection {
  const middle = direction === 'RIGHT'
    ? [{ x: (start.x + end.x) / 2, y: start.y }, { x: (start.x + end.x) / 2, y: end.y }]
    : [{ x: start.x, y: (start.y + end.y) / 2 }, { x: end.x, y: (start.y + end.y) / 2 }];
  const bendPoints = middle.filter((point, index, values) => {
    const previous = index ? values[index - 1] : start;
    const next = index === values.length - 1 ? end : values[index + 1];
    return (point.x !== previous.x || point.y !== previous.y) && (point.x !== next.x || point.y !== next.y);
  });
  return { start, end, bendPoints };
}

function intersects(left: DiagramBounds, right: DiagramBounds, gap = 24) {
  return left.x < right.x + right.width + gap && left.x + left.width + gap > right.x
    && left.y < right.y + right.height + gap && left.y + left.height + gap > right.y;
}

function finishMetadataLayout(model: DiagramModel, direction: DiagramDirection, explicitSections: Set<string>) {
  const context = model.nodes.find((node) => node.role === 'context')!;
  const nodesById = new Map(model.nodes.map((node) => [node.id, node]));
  const portsByNode = new Map<string, DiagramPort[]>();
  for (const port of model.ports) {
    const values = portsByNode.get(port.nodeId) ?? [];
    values.push(port);
    portsByNode.set(port.nodeId, values);
  }
  for (const values of portsByNode.values()) values.sort((a, b) => a.order - b.order || a.id.localeCompare(b.id));

  for (const port of model.ports) {
    if (port.position) continue;
    const owner = nodesById.get(port.nodeId);
    if (!owner?.bounds) continue;
    const peers = portsByNode.get(port.nodeId) ?? [];
    const offset = port.offset ?? (peers.indexOf(port) + 1) / (peers.length + 1);
    port.offset = clamp(offset);
    port.position = positionOnBoundary(owner.bounds, port.side, port.offset);
  }

  const portsById = new Map(model.ports.map((port) => [port.id, port]));
  for (const edge of model.edges) {
    if (explicitSections.has(edge.id) && edge.sections.length) continue;
    const start = portsById.get(edge.sourcePortId)?.position;
    const end = portsById.get(edge.targetPortId)?.position;
    edge.sections = start && end ? [orthogonalSection(start, end, direction)] : [];
  }

  if (!context.bounds) {
    const bounds = model.nodes.filter((node) => node.role === 'part' && node.bounds).map((node) => node.bounds!);
    const maxX = Math.max(1, ...bounds.map((item) => item.x + item.width + 42));
    const maxY = Math.max(1, ...bounds.map((item) => item.y + item.height + 42));
    context.bounds = { x: 0, y: 0, width: maxX, height: maxY };
  }
  return model;
}

export async function layoutDiagram(
  source: DiagramModel,
  direction: DiagramDirection = source.layout.direction,
): Promise<DiagramModel> {
  const model = structuredClone(source);
  model.layout.direction = direction;
  const context = model.nodes.find((node) => node.role === 'context');
  if (!context) throw new Error('Diagram layout requires a context node');

  const preservedBounds = new Map(model.nodes.filter((node) => node.pinned && node.bounds)
    .map((node) => [node.id, structuredClone(node.bounds!)]));
  const preservedPositions = new Map(model.ports.filter((port) => port.position)
    .map((port) => [port.id, structuredClone(port.position!)]));
  const explicitSections = new Set(model.edges.filter((edge) => edge.sections.length).map((edge) => edge.id));

  const sides = portSides(model, direction);
  const portsByNode = new Map<string, DiagramPort[]>();
  for (const port of model.ports) {
    port.side = sides.get(port.id) ?? 'UNDEFINED';
    const values = portsByNode.get(port.nodeId) ?? [];
    values.push(port);
    portsByNode.set(port.nodeId, values);
  }
  for (const values of portsByNode.values()) values.sort((a, b) => a.order - b.order || a.id.localeCompare(b.id));

  if (model.layoutSource === 'graphic-metadata') {
    return finishMetadataLayout(model, direction, explicitSections);
  }

  const childrenByParent = new Map<string, DiagramModel['nodes']>();
  for (const node of model.nodes) {
    if (node.role === 'context' || !node.parentId) continue;
    const values = childrenByParent.get(node.parentId) ?? [];
    values.push(node);
    childrenByParent.set(node.parentId, values);
  }

  const toElkNode = (node: DiagramModel['nodes'][number]): ElkNode => {
    const ports = portsByNode.get(node.id) ?? [];
    const children = childrenByParent.get(node.id) ?? [];
    return {
      id: node.id,
      width: NODE_WIDTH,
      height: nodeHeight(ports.length),
      ports: ports.map((port) => toElkPort(port, sides.get(port.id) ?? 'UNDEFINED')),
      children: children.map(toElkNode),
      layoutOptions: {
        'elk.portConstraints': 'FIXED_ORDER',
        'elk.padding': '[top=42,left=26,bottom=26,right=26]',
      },
    };
  };

  const contextPorts = portsByNode.get(context.id) ?? [];
  const contextPortNodes: ElkNode[] = contextPorts.map((port) => ({
    id: contextPortNodeId(port.id),
    width: CONTEXT_PORT_NODE_WIDTH,
    height: CONTEXT_PORT_NODE_HEIGHT,
    ports: [toElkPort(port, sides.get(port.id) ?? 'UNDEFINED')],
    layoutOptions: { 'elk.portConstraints': 'FIXED_ORDER' },
  }));

  const graph: ElkNode = {
    id: context.id,
    children: [
      ...(childrenByParent.get(context.id) ?? []).map(toElkNode),
      ...contextPortNodes,
    ],
    edges: model.edges.map<ElkExtendedEdge>((edge) => ({
      id: edge.id,
      sources: [edge.sourcePortId],
      targets: [edge.targetPortId],
    })),
    layoutOptions: {
      'elk.algorithm': 'layered',
      'elk.direction': direction,
      'elk.edgeRouting': model.layout.routing,
      'elk.hierarchyHandling': 'INCLUDE_CHILDREN',
      'elk.padding': '[top=42,left=42,bottom=42,right=42]',
      'elk.portConstraints': 'FIXED_ORDER',
      'elk.spacing.nodeNode': '48',
      'elk.spacing.edgeEdge': '14',
      'elk.spacing.portPort': '16',
      'elk.spacing.edgeNode': '24',
      'elk.layered.spacing.nodeNodeBetweenLayers': '76',
      'elk.layered.spacing.edgeEdgeBetweenLayers': '14',
      'elk.layered.spacing.edgeNodeBetweenLayers': '28',
      'elk.layered.mergeEdges': 'false',
      'elk.layered.considerModelOrder.strategy': 'NODES_AND_EDGES',
      'elk.separateConnectedComponents': 'false',
      'elk.randomSeed': String(model.layout.randomSeed),
    },
  };

  const elk = await getElk();
  const laidOut = await elk.layout(graph);
  context.bounds = {
    x: 0,
    y: 0,
    width: laidOut.width ?? 1,
    height: laidOut.height ?? 1,
  };

  const elkNodes = new Map<string, { node: ElkNode; absoluteX: number; absoluteY: number }>();
  const visit = (node: ElkNode, parentX: number, parentY: number) => {
    const absoluteX = parentX + (node.x ?? 0);
    const absoluteY = parentY + (node.y ?? 0);
    elkNodes.set(node.id, { node, absoluteX, absoluteY });
    for (const child of node.children ?? []) visit(child, absoluteX, absoluteY);
  };
  for (const child of laidOut.children ?? []) visit(child, 0, 0);

  for (const node of model.nodes) {
    if (node.role === 'context') continue;
    const result = elkNodes.get(node.id);
    if (!result) throw new Error(`ELK did not return node ${node.id}`);
    node.bounds = {
      x: result.absoluteX,
      y: result.absoluteY,
      width: result.node.width ?? NODE_WIDTH,
      height: result.node.height ?? NODE_MIN_HEIGHT,
    };
  }

  for (const port of model.ports) {
    const owner = port.nodeId === context.id
      ? elkNodes.get(contextPortNodeId(port.id))
      : elkNodes.get(port.nodeId);
    const result = owner?.node.ports?.find((candidate) => candidate.id === port.id);
    if (!owner || !result) throw new Error(`ELK did not return port ${port.id}`);
    const rawPosition = {
      x: owner.absoluteX + (result.x ?? 0) + (result.width ?? PORT_SIZE) / 2,
      y: owner.absoluteY + (result.y ?? 0) + (result.height ?? PORT_SIZE) / 2,
    };
    const ownerBounds = port.nodeId === context.id ? undefined : model.nodes.find((node) => node.id === port.nodeId)?.bounds;
    if (ownerBounds) {
      port.position = {
        x: port.side === 'WEST' ? ownerBounds.x
          : port.side === 'EAST' ? ownerBounds.x + ownerBounds.width
            : rawPosition.x,
        y: port.side === 'NORTH' ? ownerBounds.y
          : port.side === 'SOUTH' ? ownerBounds.y + ownerBounds.height
            : rawPosition.y,
      };
      port.offset = port.side === 'EAST' || port.side === 'WEST'
        ? Math.max(0, Math.min(1, (port.position.y - ownerBounds.y) / ownerBounds.height))
        : Math.max(0, Math.min(1, (port.position.x - ownerBounds.x) / ownerBounds.width));
    } else {
      port.position = rawPosition;
    }
  }

  const edgesById = new Map((laidOut.edges ?? []).map((edge) => [edge.id, edge]));
  for (const edge of model.edges) {
    const result = edgesById.get(edge.id);
    if (!result) throw new Error(`ELK did not return edge ${edge.id}`);
    edge.sections = (result.sections ?? []).map((section) => ({
      start: section.startPoint,
      end: section.endPoint,
      bendPoints: section.bendPoints ?? [],
    }));
    const sourcePort = model.ports.find((port) => port.id === edge.sourcePortId);
    const targetPort = model.ports.find((port) => port.id === edge.targetPortId);
    if (edge.sections.length && sourcePort?.position && targetPort?.position) {
      edge.sections[0].start = sourcePort.position;
      edge.sections[edge.sections.length - 1].end = targetPort.position;
    }
  }


  if (model.layoutSource === 'mixed') {
    for (const node of model.nodes) {
      const preserved = preservedBounds.get(node.id);
      if (preserved) node.bounds = preserved;
    }
    const occupied = model.nodes.filter((node) => node.role === 'part' && node.pinned && node.bounds).map((node) => node.bounds!);
    for (const node of model.nodes.filter((candidate) => candidate.role === 'part' && !candidate.pinned && candidate.bounds)) {
      while (occupied.some((item) => intersects(node.bounds!, item))) {
        if (direction === 'RIGHT') node.bounds!.y += node.bounds!.height + 48;
        else node.bounds!.x += node.bounds!.width + 48;
      }
      occupied.push(node.bounds!);
    }
    for (const port of model.ports) {
      const preserved = preservedPositions.get(port.id);
      if (preserved) {
        port.position = preserved;
        continue;
      }
      const owner = model.nodes.find((node) => node.id === port.nodeId);
      if (owner?.bounds) port.position = positionOnBoundary(owner.bounds, port.side, port.offset ?? 0.5);
    }
    return finishMetadataLayout(model, direction, explicitSections);
  }
  return model;
}
