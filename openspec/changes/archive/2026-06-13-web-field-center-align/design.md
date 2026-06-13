## Context

```
Desktop today (misaligned visual axis):

  |←──────────── #app max 960px ────────────→|
  | [ VCO Envelope ] [ Marbles 1 ] [ Marbles 2 ] |  ← mod bay: full width
  | ◀ | [knob][knob][knob][knob] | ▶          |  ← inset ~76px each side
  |        [knob][knob][knob][knob]            |
  |      Audio Marbles … (centered pills)        |
  | Rand All  Rand Mods  Marbles …             |  ← left-aligned strip

Mobile today (symmetric — keep):

  | ◀ | 4-col knob grid (may wrap rows) | ▶ |
  |         centered page pills               |
```

**Root causes (verified in `web/src/style.css`):**

1. `.field-layout { grid-template-columns: auto 1fr auto }` always reserves 52px nav columns + 1.5rem gaps, narrowing the knob grid vs full-width mod bay.
2. `#mod-bay { display: flex }` (line ~558) overrides `.mod-bay { display: grid; grid-template-columns: repeat(3, 1fr) }` — duplicate/conflicting rules.
3. `.page-pills { justify-content: center }` vs `.global-strip` / `.controls-top` default flex-start — mixed alignment makes the page feel shifted.
4. No `scrollbar-gutter: stable` — vertical scrollbar can nudge `#app` a few pixels off true viewport center.

`host-ui-delay-page` requires flanking arrows **only at ≤720px**; desktop arrow hiding does not violate spec.

## Data Flow

| Viewport | Input | Transform | Output |
|----------|-------|-----------|--------|
| >720px | `#app` width | single-column field; mod bay + knobs same width | aligned vertical axis |
| ≤720px | `#app` width | 3-col field + visible nav | centered knob block between arrows |
| All | mod bay children | grid `1fr × 3` only | equal mod cell widths |

## Goals / Non-Goals

**Goals:**

- Mod bay left/right edges align with knob grid left/right edges on desktop.
- Page pills and global strip share the same horizontal center axis on desktop.
- Mobile: prev/next arrows, 44px targets, no knob horizontal scroll unchanged.
- CSS-only fix in one file; no duplicate mobile/desktop markup.

**Non-Goals:**

- Reintroducing knob-group meta-panels or changing column cell internals.
- Desktop layout changes to mod bay content (LED/scopes stay).
- Resizing `#app` max-width or typography.

## Decisions

### D1: Desktop hides flanking page-nav

**Choice:** `@media (min-width: 721px)`: `.page-nav { display: none }`, `.field-layout { grid-template-columns: 1fr }`, `.knobs { grid-column: 1 }`.

**Why:** Spec mandates arrows at ≤720px only. Removing desktop inset lets knobs span the same width as mod bay without JS.

**Alternative rejected:** Keep arrows and shrink mod bay to match inset — duplicates gutter math and wastes desktop space.

### D2: Mobile keeps three-column field layout

**Choice:** `@media (max-width: 720px)`: retain `auto 1fr auto`, visible `.page-nav`, existing gap values. Optional: `justify-items: center` on middle column if knob grid narrower than track.

**Why:** Preserves touch nav and `host-ui-delay-page` mobile scenarios.

### D3: Single mod bay layout rule

**Choice:** Delete `#mod-bay { display: flex; … }` block. Keep `.mod-bay { display: grid; grid-template-columns: repeat(3, minmax(0, 1fr)); }` only.

**Why:** Eliminates dead override; three mod cells stay equal width aligned with knob columns below.

### D4: Center global strip on desktop

**Choice:** `@media (min-width: 721px)`: `.global-strip { justify-content: center }`.

**Why:** Matches centered page pills; completes one visual axis. Mobile keeps wrap from flex-start (narrow width).

### D5: Stable scrollbar gutter

**Choice:** `html { scrollbar-gutter: stable; }`.

**Why:** Prevents sub-pixel shift of `#app` when content grows tall enough for a scrollbar.

### D6: Knob grid row alignment

**Choice:** Change `.knobs { align-items: end }` to `align-items: stretch` (default cell height uniform).

**Why:** `end` bottom-aligns columns of unequal content height (hint slot + morph buttons), which can exaggerate a “leaning” grid; stretch keeps tops aligned without affecting horizontal centering.

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| Desktop users lose visible page arrows | Page pills + `[`/`]` keys remain; tooltip in hint line already documents keys |
| Centered global strip wraps awkwardly at ~721–800px | flex-wrap still applies; pills already wrap similarly |
| `scrollbar-gutter: stable` adds empty gutter when no scroll | Acceptable; only affects pages with overflow |

## Migration Plan

1. Edit `web/src/style.css` in one pass (mod bay, field-layout media queries, global strip, html gutter, knobs align).
2. `cd web && npm run build`.
3. Manual verify at 1280px and 390px widths.

## Open Questions

None.
