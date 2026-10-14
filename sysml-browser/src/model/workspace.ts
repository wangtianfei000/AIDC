import type { AnalysisSnapshot, ModelSnapshot, ModelSource } from './types';

export interface OpenModelDocument {
  id: string;
  source: ModelSource;
  snapshot: ModelSnapshot;
  analysis?: AnalysisSnapshot;
  analysisError?: string;
  staleSnapshot: boolean;
}

export function upsertOpenDocument(current: OpenModelDocument[], document: OpenModelDocument): OpenModelDocument[] {
  const index = current.findIndex((item) => item.id === document.id);
  if (index < 0) return [...current, document];
  const next = [...current];
  next[index] = document;
  return next;
}

export function closeOpenDocument(current: OpenModelDocument[], documentId: string) {
  const index = current.findIndex((item) => item.id === documentId);
  if (index < 0) return { documents: current, nextDocumentId: undefined };
  const documents = current.filter((item) => item.id !== documentId);
  return { documents, nextDocumentId: documents[Math.min(index, documents.length - 1)]?.id };
}
