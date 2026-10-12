import type { ModelElement } from '../model/types';
import type { ModelQuery } from '../model/query';

export interface Model3DRegistration {
  modelId: string;
  manifestUrl: string;
  cadRefs: string[];
  componentIds: string[];
}

export interface ResolvedModel3D {
  registration: Model3DRegistration;
  contextElement: ModelElement;
  focusComponentId?: string;
}

export const model3dRegistry: Model3DRegistration[] = [{
  modelId: 'SYS-JG10',
  manifestUrl: './model3d/jingang/model3d.manifest.json',
  cadRefs: ['layout/jingang-10axis-layout.FCStd'],
  componentIds: [
    'SYS-JG10', 'ASM-BODY', 'ASM-LEG-L', 'ACT-L-HIP-ROLL', 'ACT-L-HIP-PITCH', 'ACT-L-KNEE-PITCH', 'ACT-L-ANKLE-PITCH',
    'ASM-LEG-R', 'ACT-R-HIP-ROLL', 'ACT-R-HIP-PITCH', 'ACT-R-KNEE-PITCH', 'ACT-R-ANKLE-PITCH', 'ASM-HEAD-NECK',
    'ACT-NECK-PITCH', 'ACT-HEAD-YAW', 'SYS-POWER', 'SYS-COMPUTE', 'SYS-MOTION-CONTROL', 'SYS-SENSING',
  ],
}];

function unwrapString(value: unknown): string | undefined {
  if (typeof value !== 'string') return undefined;
  if (/^"(?:[^"\\]|\\.)*"$/.test(value)) {
    try { return JSON.parse(value); } catch { return value.slice(1, -1); }
  }
  return value;
}

export function featureValue(element: ModelElement, name: string, query: ModelQuery): string | undefined {
  const direct = unwrapString(element.properties[name]);
  if (direct) return direct;
  const feature = query.children(element.id).find((child) => child.kind === 'Feature' && child.name === name);
  return unwrapString(feature?.properties.valueExpression);
}

export function componentIdForElement(element: ModelElement | undefined, query: ModelQuery): string | undefined {
  return element ? featureValue(element, 'componentId', query) : undefined;
}

export function elementByComponentId(query: ModelQuery): Map<string, ModelElement> {
  const result = new Map<string, ModelElement>();
  for (const element of query.snapshot.elements) {
    const componentId = componentIdForElement(element, query);
    if (componentId) result.set(componentId, element);
  }
  return result;
}

export function resolveModel3D(element: ModelElement | undefined, query: ModelQuery): ResolvedModel3D | undefined {
  if (!element) return undefined;
  for (const candidate of [element, ...query.ancestors(element.id)]) {
    const componentId = componentIdForElement(candidate, query);
    const cadRef = featureValue(candidate, 'cadRef', query);
    const registration = model3dRegistry.find((item) =>
      (componentId && item.componentIds.includes(componentId)) || (cadRef && item.cadRefs.includes(cadRef)),
    );
    if (registration) return { registration, contextElement: candidate, focusComponentId: componentId };

    // A configuration usage normally carries only its declared type. Resolve the
    // type definition so instances can open the same published model package.
    const typeName = unwrapString(candidate.properties.typeName)?.split('::').pop();
    if (typeName) {
      const definition = query.snapshot.elements.find((item) => item.kind === 'PartDefinition' && item.name === typeName);
      if (definition) {
        const definitionComponentId = componentIdForElement(definition, query);
        const typedRegistration = model3dRegistry.find((item) => definitionComponentId && item.componentIds.includes(definitionComponentId));
        if (typedRegistration) return { registration: typedRegistration, contextElement: candidate, focusComponentId: definitionComponentId };
      }
    }
  }
  return undefined;
}

export function hasRegisteredModel3D(element: ModelElement, query: ModelQuery): boolean {
  return Boolean(resolveModel3D(element, query));
}
