import { describe, expect, it } from 'vitest';
import { parseArchitecturePath } from './MaxGraphArchitecture';

describe('maxGraph architecture path adapter', () => {
  it('preserves orthogonal segments and quadratic jump geometry', () => {
    expect(parseArchitecturePath('M10,20 L30,20 Q36,6 42,20 L60,20')).toEqual([
      { kind: 'M', x: 10, y: 20 },
      { kind: 'L', x: 30, y: 20 },
      { kind: 'Q', cx: 36, cy: 6, x: 42, y: 20 },
      { kind: 'L', x: 60, y: 20 },
    ]);
  });
});
