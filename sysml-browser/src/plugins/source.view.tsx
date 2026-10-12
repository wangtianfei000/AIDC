import { Braces } from 'lucide-react';
import { SourceView } from '../components/Views';
import type { BrowserViewDefinition } from './viewRegistry';

export const browserViews: BrowserViewDefinition[] = [{
  id: 'core.source',
  label: 'SysML 源码',
  hint: '只读定位',
  group: '通用浏览',
  icon: Braces,
  component: ({ sourceText, selected }) => <SourceView text={sourceText} selected={selected} />,
}];
