import type { ComponentType } from 'react';
import type { LucideIcon } from 'lucide-react';
import type { AnalysisSnapshot, ModelElement, ModelSnapshot } from '../model/types';
import type { ModelQuery } from '../model/query';

export interface BrowserViewContext {
  snapshot: ModelSnapshot;
  query: ModelQuery;
  sourceText: string;
  analysis?: AnalysisSnapshot;
  analysisError?: string;
  selected?: ModelElement;
  selectedId?: string;
  viewContextId?: string;
  viewState?: Record<string, unknown>;
  onSelect: (id: string) => void;
  selectInView: (id: string) => void;
  openView: (viewId: string, elementId?: string) => void;
  saveViewState: (state: Record<string, unknown>) => void;
}

export interface BrowserViewDefinition {
  id: string;
  label: string;
  hint: string;
  group: '需求浏览' | '架构浏览' | '参数分析' | '通用浏览';
  icon: LucideIcon;
  component: ComponentType<BrowserViewContext>;
  badge?: (snapshot: ModelSnapshot) => number | undefined;
}

type ViewModule = { browserViews?: BrowserViewDefinition[] };

const modules = import.meta.glob<ViewModule>('./*.view.tsx', { eager: true });

export const browserViews = Object.values(modules)
  .flatMap((module) => module.browserViews ?? [])
  .sort((a, b) => {
    const groupOrder = { '需求浏览': 0, '架构浏览': 1, '参数分析': 2, '通用浏览': 3 };
    return groupOrder[a.group] - groupOrder[b.group] || a.id.localeCompare(b.id);
  });

export const defaultViewId = 'requirements.table';
