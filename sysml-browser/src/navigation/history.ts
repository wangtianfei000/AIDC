export interface NavigationEntry {
  viewId: string;
  selectedId?: string;
  viewContextId?: string;
  viewState?: Record<string, unknown>;
}

export interface NavigationState {
  entries: NavigationEntry[];
  index: number;
}

export function createNavigationState(entry?: NavigationEntry): NavigationState {
  return entry ? { entries: [entry], index: 0 } : { entries: [], index: -1 };
}

export function currentNavigationEntry(state: NavigationState): NavigationEntry | undefined {
  return state.entries[state.index];
}

export function resetNavigationState(entry: NavigationEntry): NavigationState {
  return createNavigationState(entry);
}

export function navigateNavigationState(state: NavigationState, entry: NavigationEntry): NavigationState {
  const current = currentNavigationEntry(state);
  if (current?.viewId === entry.viewId && current.selectedId === entry.selectedId && current.viewContextId === entry.viewContextId) return state;
  const entries = [...state.entries.slice(0, state.index + 1), entry];
  return { entries, index: entries.length - 1 };
}

export function updateCurrentNavigationViewState(state: NavigationState, viewState: Record<string, unknown>): NavigationState {
  if (state.index < 0) return state;
  const entries = [...state.entries];
  entries[state.index] = { ...entries[state.index], viewState };
  return { entries, index: state.index };
}

export function selectInCurrentNavigation(state: NavigationState, selectedId: string): NavigationState {
  if (state.index < 0) return state;
  const entries = [...state.entries];
  entries[state.index] = { ...entries[state.index], selectedId };
  return { entries, index: state.index };
}

export function moveNavigationState(state: NavigationState, offset: -1 | 1): NavigationState {
  const index = state.index + offset;
  return index < 0 || index >= state.entries.length ? state : { entries: state.entries, index };
}

export function canMoveNavigation(state: NavigationState, offset: -1 | 1): boolean {
  const index = state.index + offset;
  return index >= 0 && index < state.entries.length;
}
