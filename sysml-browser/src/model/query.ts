import type { ModelElement, ModelRelationship, ModelSnapshot } from './types';

export class ModelQuery {
  readonly byId = new Map<string, ModelElement>();
  readonly relationshipById = new Map<string, ModelRelationship>();
  readonly childrenByOwner = new Map<string | null, ModelElement[]>();
  readonly outgoing = new Map<string, ModelRelationship[]>();
  readonly incoming = new Map<string, ModelRelationship[]>();

  constructor(readonly snapshot: ModelSnapshot) {
    for (const element of snapshot.elements) {
      this.byId.set(element.id, element);
      const children = this.childrenByOwner.get(element.ownerId) ?? [];
      children.push(element);
      this.childrenByOwner.set(element.ownerId, children);
    }
    for (const relationship of snapshot.relationships) {
      this.relationshipById.set(relationship.id, relationship);
      for (const id of relationship.sourceIds) {
        const values = this.outgoing.get(id) ?? [];
        values.push(relationship);
        this.outgoing.set(id, values);
      }
      for (const id of relationship.targetIds) {
        const values = this.incoming.get(id) ?? [];
        values.push(relationship);
        this.incoming.set(id, values);
      }
    }
  }

  children(ownerId: string | null): ModelElement[] {
    return this.childrenByOwner.get(ownerId) ?? [];
  }

  descendants(id: string): ModelElement[] {
    const result: ModelElement[] = [];
    const visit = (ownerId: string) => {
      for (const child of this.children(ownerId)) {
        result.push(child);
        visit(child.id);
      }
    };
    visit(id);
    return result;
  }

  ancestors(id: string): ModelElement[] {
    const result: ModelElement[] = [];
    let current = this.byId.get(id);
    const visited = new Set<string>();
    while (current?.ownerId && !visited.has(current.ownerId)) {
      visited.add(current.ownerId);
      const owner = this.byId.get(current.ownerId);
      if (!owner) break;
      result.push(owner);
      current = owner;
    }
    return result;
  }

  related(id: string) {
    return {
      outgoing: this.outgoing.get(id) ?? [],
      incoming: this.incoming.get(id) ?? [],
    };
  }
}

