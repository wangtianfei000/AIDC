import { describe, expect, it } from 'vitest';
import { constrainPanelWidth } from './panelLayout';

describe('resizable side panels', () => {
  it('applies minimum and maximum widths', () => {
    const widths = { left: 320, right: 300 };
    expect(constrainPanelWidth('left', 80, widths, 1920)).toBe(220);
    expect(constrainPanelWidth('right', 900, widths, 1920)).toBe(520);
  });

  it('preserves the minimum central workspace', () => {
    const widths = { left: 320, right: 300 };
    expect(constrainPanelWidth('left', 520, widths, 1200)).toBe(280);
    expect(constrainPanelWidth('right', 520, widths, 1200)).toBe(260);
  });
});
