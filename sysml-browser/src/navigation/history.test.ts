import { describe, expect, it } from 'vitest';
import {
  canMoveNavigation, createNavigationState, currentNavigationEntry, moveNavigationState,
  navigateNavigationState, resetNavigationState, selectInCurrentNavigation,
  updateCurrentNavigationViewState,
} from './history';

describe('view navigation history', () => {
  it('restores result selection across table, dependency graph and source history', () => {
    let state = resetNavigationState({ viewId: 'analysis.table', selectedId: 'enduranceResult' });
    state = selectInCurrentNavigation(state, 'enduranceResult');
    state = navigateNavigationState(state, { viewId: 'analysis.dependencies', selectedId: 'enduranceResult' });
    state = navigateNavigationState(state, { viewId: 'core.source', selectedId: 'batteryCapacity' });

    state = moveNavigationState(state, -1);
    expect(currentNavigationEntry(state)).toEqual({ viewId: 'analysis.dependencies', selectedId: 'enduranceResult' });
    state = moveNavigationState(state, -1);
    expect(currentNavigationEntry(state)).toEqual({ viewId: 'analysis.table', selectedId: 'enduranceResult' });
    expect(canMoveNavigation(state, 1)).toBe(true);
    state = moveNavigationState(state, 1);
    expect(currentNavigationEntry(state)).toEqual({ viewId: 'analysis.dependencies', selectedId: 'enduranceResult' });
  });

  it('updates only the current entry and drops the forward branch after new navigation', () => {
    let state = createNavigationState({ viewId: 'architecture.diagram', selectedId: 'JingangNative10' });
    state = navigateNavigationState(state, { viewId: 'architecture.diagram', selectedId: 'leftLeg' });
    state = moveNavigationState(state, -1);
    state = selectInCurrentNavigation(state, 'JingangNative10');
    state = navigateNavigationState(state, { viewId: 'core.source', selectedId: 'JingangNative10' });

    expect(state.entries).toEqual([
      { viewId: 'architecture.diagram', selectedId: 'JingangNative10' },
      { viewId: 'core.source', selectedId: 'JingangNative10' },
    ]);
    expect(canMoveNavigation(state, 1)).toBe(false);
  });

  it('keeps the diagram context when only the selected element changes', () => {
    let state = createNavigationState({
      viewId: 'architecture.diagram',
      selectedId: 'JingangNative10',
      viewContextId: 'JingangNative10',
    });
    state = selectInCurrentNavigation(state, 'leftLeg');

    expect(currentNavigationEntry(state)).toEqual({
      viewId: 'architecture.diagram',
      selectedId: 'leftLeg',
      viewContextId: 'JingangNative10',
    });
  });

  it('keeps a 3D camera and display snapshot on the history entry', () => {
    let state = createNavigationState({ viewId: 'architecture.diagram', selectedId: 'robot', viewContextId: 'robot' });
    state = updateCurrentNavigationViewState(state, { projection: 'orthographic', camera: [1, 2, 3], hidden: ['left-leg'] });
    state = navigateNavigationState(state, { viewId: 'core.source', selectedId: 'robot', viewContextId: 'robot' });
    state = moveNavigationState(state, -1);
    expect(currentNavigationEntry(state)?.viewState).toEqual({ projection: 'orthographic', camera: [1, 2, 3], hidden: ['left-leg'] });
  });
});

