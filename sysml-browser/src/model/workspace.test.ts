import { describe, expect, it } from 'vitest';
import { createElement } from 'react';
import { renderToStaticMarkup } from 'react-dom/server';
import { ModelNavigator } from '../App';
import type { ModelSnapshot } from './types';
import { closeOpenDocument, type OpenModelDocument, upsertOpenDocument } from './workspace';

const snapshot = (uri: string): ModelSnapshot => ({
  schemaVersion: '0.1',
  document: { uri, contentHash: uri, parser: { name: 'test', baseline: 'test', capabilityVersion: '0.1' } },
  elements: [{ id: `${uri}-root`, kind: 'Package', name: `${uri}Package`, qualifiedName: `${uri}Package`, ownerId: null, source: null, properties: {} }], relationships: [], diagnostics: [], capabilities: {},
});

const document = (id: string): OpenModelDocument => ({
  id,
  source: { name: `${id}.sysml`, uri: id, text: `package ${id};` },
  snapshot: snapshot(id),
  staleSnapshot: false,
});

describe('multi-model workspace', () => {
  it('keeps two independently parsed models open', () => {
    const documents = upsertOpenDocument(upsertOpenDocument([], document('first')), document('second'));
    expect(documents.map((item) => item.id)).toEqual(['first', 'second']);
    expect(documents[0].snapshot).not.toBe(documents[1].snapshot);
  });

  it('refreshes a model in place without replacing the other model', () => {
    const documents = [document('first'), document('second')];
    const refreshed = { ...document('first'), source: { ...document('first').source, text: 'package refreshed;' } };
    const next = upsertOpenDocument(documents, refreshed);
    expect(next.map((item) => item.id)).toEqual(['first', 'second']);
    expect(next[0].source.text).toBe('package refreshed;');
  });

  it('selects the adjacent model when the active model closes', () => {
    const result = closeOpenDocument([document('first'), document('second'), document('third')], 'second');
    expect(result.documents.map((item) => item.id)).toEqual(['first', 'third']);
    expect(result.nextDocumentId).toBe('third');
  });

  it('merges top-level packages without adding filename nodes', () => {
    const markup = renderToStaticMarkup(createElement(ModelNavigator, {
      documents: [document('first'), document('second')],
      activeDocumentId: 'first',
      activeViewId: 'architecture.tree',
      onSelect: () => undefined,
      onOpenView: () => undefined,
      onShowProperties: () => undefined,
    }));
    expect(markup).toContain('2 个模型');
    expect(markup).toContain('firstPackage');
    expect(markup).toContain('secondPackage');
    expect(markup).not.toContain('first.sysml');
    expect(markup).not.toContain('navigator-document-row');
  });
});
