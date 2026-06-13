## Why

The `web-vco-morph-mobile` fix (2-column grid + knob/morph side-by-side) reduced clipping but did not fix the root layout conflict: at ≤720px each knob cell is ~84px wide while knob (44px) + morph (28px) + gap need ~76px inside a row that also shares horizontal space with a neighbor column. Morph buttons overflow their cells, paint over adjacent knobs (`z-index: 1`), and knobs look off-center because the knob-row grid is asymmetric. Desktop works because 4-column cells are ~150px+ wide — the side-by-side pattern only fails on mobile.

## What Changes

- **Unified knob column template** — every `.knob-col` uses the same vertical stack: label → optional hint → optional VCO morph → centered knob → mod label → mod select. All children center-aligned on one axis; no horizontal knob+morph row.
- **VCO morph placement** — move waveform button from `.knob-row` sibling to **between parameter label and knob** (columns 0–2 on Audio page only). Same blue 28×28 SVG; same click/`cycleVcoMorph` behavior.
- **Remove mobile ad hoc layout hacks** — drop `.knob-row` two-column grid, morph `z-index`, and side-by-side sizing that caused cross-cell bleed. Keep existing 2-column `.knobs` grid and 4-column desktop grid unchanged.
- **Column box alignment** — knob centers on the bordered cell; every column reserves a 28×28 morph row (active waveform on VCO1–3, invisible `.knob-morph-slot` placeholder on columns 4–8) so bordered cell heights stay uniform on the Audio page grid.

## Capabilities

### New Capabilities

- `web-knob-column-stack`: Unified vertical stack template and center alignment for all eight knob columns on every viewport.

### Modified Capabilities

- `web-vco-morph-inline`: Morph control location changes from beside knob to between label and knob (all viewports).
- `web-vco-morph-mobile`: Mobile layout requirement updated — morph in label–knob gap, not side-by-side row.

## Impact

- `web/src/main.ts` — DOM order: insert morph after label, remove from `knob-row`
- `web/src/style.css` — column stack grid/flex, simplify `.knob-row`, remove morph z-index side-by-side rules
- `openspec/specs/web-vco-morph-inline/spec.md` — morph placement requirement (via delta)
- No WASM, desktop, or worklet changes
