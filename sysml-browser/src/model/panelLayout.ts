export type PanelSide = 'left' | 'right';
export interface PanelWidths { left: number; right: number }

export const PANEL_LIMITS = {
  leftMin: 220,
  rightMin: 220,
  max: 520,
  workspaceMin: 620,
} as const;

export function constrainPanelWidth(side: PanelSide, value: number, widths: PanelWidths, viewportWidth: number): number {
  const minimum = side === 'left' ? PANEL_LIMITS.leftMin : PANEL_LIMITS.rightMin;
  const other = side === 'left' ? widths.right : widths.left;
  const available = viewportWidth - other - PANEL_LIMITS.workspaceMin;
  const maximum = Math.max(minimum, Math.min(PANEL_LIMITS.max, available));
  return Math.round(Math.max(minimum, Math.min(maximum, value)));
}
