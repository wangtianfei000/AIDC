import { describe, expect, it, vi } from 'vitest';
import * as THREE from 'three';
import { applyDisplayState, componentIdFromObject, disposeObject3D, parseModel3DViewState } from './runtime';

describe('Model3D browser runtime', () => {
  it('restores a safe navigation state and applies isolate/hidden display rules', () => {
    expect(parseModel3DViewState({ projection: 'bad', hiddenComponentIds: ['A', 2], transparent: true, jointValues: { good: .5, bad: 'x' }, ghostEnabled: true })).toMatchObject({ projection: 'perspective', hiddenComponentIds: ['A'], transparent: true, jointValues: { good: .5 }, ghostEnabled: true });
    const scene = new THREE.Group();
    const a = new THREE.Group(); const aMesh = new THREE.Mesh(new THREE.BoxGeometry(), new THREE.MeshStandardMaterial()); a.add(aMesh); scene.add(a);
    const b = new THREE.Group(); const bMesh = new THREE.Mesh(new THREE.BoxGeometry(), new THREE.MeshStandardMaterial()); b.add(bMesh); scene.add(b);
    const components = new Map([['A', a], ['B', b]]);
    applyDisplayState(scene, components, { hiddenComponentIds: [], isolatedComponentId: 'A', transparent: false, wireframe: false });
    expect(aMesh.visible).toBe(true); expect(bMesh.visible).toBe(false);
    expect(componentIdFromObject(aMesh, components)).toBe('A');
    disposeObject3D(scene);
  });

  it('disposes geometry, material and texture resources on repeated closes', () => {
    for (let index = 0; index < 3; index += 1) {
      const geometry = new THREE.BoxGeometry();
      const texture = new THREE.Texture();
      const material = new THREE.MeshStandardMaterial({ map: texture });
      const geometryDispose = vi.spyOn(geometry, 'dispose');
      const materialDispose = vi.spyOn(material, 'dispose');
      const textureDispose = vi.spyOn(texture, 'dispose');
      const root = new THREE.Group();
      root.add(new THREE.Mesh(geometry, material));

      disposeObject3D(root);

      expect(geometryDispose).toHaveBeenCalledOnce();
      expect(materialDispose).toHaveBeenCalledOnce();
      expect(textureDispose).toHaveBeenCalledOnce();
      expect(root.children).toHaveLength(0);
    }
  });
});
