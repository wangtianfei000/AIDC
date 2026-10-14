import type { ModelElement, ModelSnapshot } from './types';

const HAN_TEXT = /[\u3400-\u9fff]/;
const SENTENCE_PUNCTUATION = /[。！？；\n\r]/;

export function elementDocs(snapshot: ModelSnapshot, element: ModelElement): string[] {
  return snapshot.elements
    .filter((item) => item.ownerId === element.id && item.kind === 'DocStatement')
    .map((item) => item.properties.content)
    .filter((value): value is string => typeof value === 'string')
    .map((value) => value.trim())
    .filter(Boolean);
}

/** A short Chinese doc comment is treated as the element's human-facing name. */
export function localizedElementName(snapshot: ModelSnapshot, element: ModelElement): string | undefined {
  return elementDocs(snapshot, element).find((value) =>
    value.length <= 24 && HAN_TEXT.test(value) && !SENTENCE_PUNCTUATION.test(value));
}

export function elementDisplayName(snapshot: ModelSnapshot, element: ModelElement): string {
  return localizedElementName(snapshot, element)
    ?? element.name
    ?? element.qualifiedName
    ?? '未命名元素';
}
