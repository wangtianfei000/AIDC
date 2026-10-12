import { describe, expect, it } from 'vitest';
import type { ModelElement } from './types';
import { visibleInspectorProperties, visibleModelProperties } from './propertyVisibility';

describe('model property visibility', () => {
  it('hides parser defaults and keeps business values, including zero', () => {
    expect(Object.fromEntries(visibleModelProperties({
      direction: 0, isConst: false, isDerived: false, isCustomFlag: false, isRequired: true,
      empty: '', emptyList: [], mass: 0, componentId: 'SYS-01', valueExpression: '6.4[V]',
    }))).toEqual({ isRequired: true, mass: 0, componentId: 'SYS-01', valueExpression: '6.4[V]' });
  });

  it('projects owned attribute values as component model properties', () => {
    const source = null;
    const component = { id: 'robot', kind: 'PartDefinition', name: 'Robot', qualifiedName: 'Robot', ownerId: null, source, properties: { isAbstract: false } } satisfies ModelElement;
    const children = [
      { id: 'id', kind: 'Feature', name: 'componentId', qualifiedName: 'Robot::componentId', ownerId: 'robot', source, properties: { valueExpression: '"SYS-01"' } },
      { id: 'mass', kind: 'Feature', name: 'maximumMass', qualifiedName: 'Robot::maximumMass', ownerId: 'robot', source, properties: { valueExpression: '800[g]', unitExpression: 'g' } },
      { id: 'part', kind: 'PartUsage', name: 'leg', qualifiedName: 'Robot::leg', ownerId: 'robot', source, properties: {} },
    ] satisfies ModelElement[];
    expect(Object.fromEntries(visibleInspectorProperties(component, children))).toEqual({ componentId: 'SYS-01', maximumMass: '800[g]' });
  });

  it('hides expression fields from the property inspector', () => {
    const feature = { id: 'result', kind: 'Feature', name: 'massResult', qualifiedName: 'Analysis::massResult', ownerId: 'analysis', source: null, properties: { valueExpression: 'ComputeMass(robot.mass)', expressionAst: { kind: 'call' } } } satisfies ModelElement;
    expect(visibleInspectorProperties(feature, [])).toEqual([]);
  });
});
