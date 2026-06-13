## Context

```
Current DOM (all viewports):

  .knob-col (flex column, align-items: center)
    ├── .knob-label-main
    ├── .knob-hint
    ├── .knob-row (grid: 1fr | auto)     ← problem container
    │     ├── .rotary-knob  (44×44)
    │     └── .vco-morph-btn (28×28, z-index: 1)
    ├── .mod-source-label
    └── .mod-select

Mobile width budget (375px iPhone, portrait):

  #app content ≈ 327px
  .field-layout: [52px nav | 1fr knobs | 52px nav] + 2×24px gap
  .knobs usable ≈ 175px → 2 columns ≈ 84px/cell (incl. padding)
  Side-by-side need: 44 + 4 + 28 = 76px content → tight + overflow paints neighbors
```

Desktop at 960px `#app`: 4 columns ≈ 210px/cell — side-by-side fits, which is why desktop looked fine while mobile broke.

Mobile browsers do not clip grid/flex overflow by default; positioned/`z-index` children stack above adjacent cells. Touch targets then overlap the neighbor knob — the waveform "covers the knob to the right."

## Goals / Non-Goals

**Goals:**

- One column template for all eight knobs, all six pages, all viewports — no mobile-only DOM branches.
- VCO morph visible and tappable at 375px without overlapping neighbor columns.
- Knob visually centered in its bordered cell; label, morph slot, mod controls share the same horizontal center line.
- Preserve desktop: 4-column grid, mod bay alignment, field-layout ≥721px rules from `web-field-horizontal-align`.
- Preserve mobile: 2-column `.knobs`, ◀ ▶ nav, centered field — from `web-vco-morph-mobile` / `web-field-horizontal-align`.

**Non-Goals:**

- Changing knob size (44px), morph SVG size (28px), or page grid column count.
- Desktop-only side-by-side morph (rejected — two layouts = ad hoc).
- JS viewport branching for layout.
- Delay page or mod-bay changes.

## Decisions

### D1: Vertical stack — morph between label and knob

**Choice:** DOM order:

```
label → morph (VCO 0–2 only) → hint → knob → mod-label → mod-select
```

**Why:** Uses vertical space inside the cell (plenty on mobile ~84px wide × tall column). Eliminates horizontal overflow and cross-cell paint. Matches user request and noriegas-style "parameter owns its waveform."

**Alternative rejected:** Keep side-by-side on desktop, stack on mobile — two templates, divergent CSS/JS, still ad hoc.

### D2: `.knob-col` as CSS grid with named rows

**Choice:**

```css
.knob-col {
  display: grid;
  grid-auto-rows: auto;
  justify-items: center;
  align-content: start;
  gap: 0.25rem;
  width: 100%;
  min-width: 0;
}
```

Optional morph slot on cols 3–7: append empty `.knob-morph-slot` span (28×28, `aria-hidden="true"`) so all eight columns share the same row geometry on every page.

**Why:** Grid `justify-items: center` centers every row on one axis without per-child hacks. OMNI: one structure, same DOM shape for all eight columns, no nested asymmetric grid.

### D3: Simplify `.knob-row` to knob-only centering

**Choice:** `.knob-row { display: flex; justify-content: center; width: 100%; }` — holds only the rotary knob.

**Why:** Removes `grid-template-columns: 1fr auto` that caused off-center knob appearance.

### D4: Remove morph `z-index` and side-by-side rules

**Choice:** Delete `.vco-morph-btn { position: relative; z-index: 1 }` and `.knob-row .rotary-knob { justify-self: center }`.

**Why:** No longer fighting overflow; z-index was a band-aid that made overlap worse visually.

### D5: `main.ts` DOM build — move morph append target

**Choice:** After creating `mainLabel`, append `morphBtn` (when `i < 3`), then hint, then knob-only row.

**Why:** Single build path; `renderVcoMorphButtons` unchanged except button parent is column not row.

### D6: Breakpoint behavior unchanged

**Choice:** Keep `@media (max-width: 720px) { .knobs { grid-template-columns: repeat(2, 1fr); } }` and `@media (min-width: 721px) { .page-nav { display: none; } }`.

**Why:** Column count is orthogonal to intra-column stack; desktop layout not weakened.

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| VCO columns slightly taller (morph row) | Acceptable; 2-col mobile already scrolls vertically |
| Spec says morph "to the right of knob" | Update `web-vco-morph-inline` delta to "between label and knob" |
| Columns 0–2 taller than 3–7 | `.knob-morph-slot` placeholder (28×28) on cols 3–7 preserves uniform row heights |
| Touch target 28px morph | Meets minimum if entire button is tappable; column padding adds margin |

## Migration Plan

1. Update `main.ts` DOM order (morph parent move).
2. Replace `.knob-col` / `.knob-row` / morph CSS.
3. Verify 375px and 960px in DevTools; confirm no horizontal bleed.
4. Archive; merge spec deltas.

## Open Questions

- (none)
