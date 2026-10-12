const hiddenDefaultKeys = new Set([
  'isAbstract', 'isIndividual', 'isVariation', 'isConst', 'isDerived', 'isRedefines', 'isRef', 'isSimplifiedAttribute',
]);

function isEmptyValue(value: unknown): boolean {
  if (value == null || value === '') return true;
  if (Array.isArray(value)) return value.length === 0;
  if (typeof value === 'object') return Object.keys(value as Record<string, unknown>).length === 0;
  return false;
}

export function visibleModelProperties(properties: Record<string, unknown>): Array<[string, unknown]> {
  return Object.entries(properties).filter(([key, value]) => {
    if (key.startsWith('_') || isEmptyValue(value)) return false;
    if (value === false && (hiddenDefaultKeys.has(key) || /^is[A-Z]/.test(key))) return false;
    if (key === 'direction' && (value === 0 || value === '0' || value === 'none')) return false;
    return true;
  });
}

export function visibleInspectorProperties(element: ModelElement, children: ModelElement[]): Array<[string, unknown]> {
  const expressionKeys = new Set(['valueExpression', 'expression', 'formula', 'formulaAst', 'expressionAst']);
  const direct = visibleModelProperties(element.properties).filter(([key]) => !expressionKeys.has(key));
  if (!/^Part(Definition|Usage)$/.test(element.kind)) return direct;
  const ownedAttributes = children.flatMap((child): Array<[string, unknown]> => {
    if (child.kind !== 'Feature' || !child.name) return [];
    const value = child.properties.valueExpression;
    return isEmptyValue(value) ? [] : [[child.name, displayFeatureValue(value)]];
  });
  const names = new Set(direct.map(([name]) => name));
  return [...direct, ...ownedAttributes.filter(([name]) => !names.has(name))];
}

function displayFeatureValue(value: unknown): unknown {
  if (typeof value !== 'string' || !/^"(?:[^"\\]|\\.)*"$/.test(value)) return value;
  try { return JSON.parse(value); } catch { return value.slice(1, -1); }
}
import type { ModelElement } from './types';
