## Context

Mobile layout for the web sim was added in commit `5226f75` with intent for a 2-column knob grid and wrapped labels below page nav. The stylesheet currently declares mobile overrides inside `@media (max-width: 720px)` at lines 447–492, then declares the base `.knobs` grid at lines 494–500. Equal-specificity cascade means the base **4-column** rule wins on all viewports.

Runtime evidence (Playwright Chromium, 390×844, preview on port 4173):

```
gridTemplateColumns: "80.7px 80.7px 80.7px 80.7px"  // 4 columns, not 2
label width: ~68px per column
```

User report on real iOS: all parameter labels appear blank. Chromium still shows text at 68px width, so Safari-specific `-webkit-box` / `-webkit-line-clamp` behavior plus extreme column narrowness is the likely compound failure.

Existing e2e infra: `web/e2e/helpers.ts` (`MOBILE_USE`, `startSimAudio`), `web/playwright.config.ts` (Chromium desktop project only — mobile tests use `test.use(MOILE_USE)` per file).

## Goals / Non-Goals

**Goals:**

- Restore intended 2-column mobile knob grid
- Ensure parameter labels remain visible on narrow viewports after WASM populates them
- Lock behavior with Playwright assertions on computed grid columns and label bounding boxes

**Non-Goals:**

- Redesign desktop 4-column layout
- Change label text authority (`ParamDisplayNames.hpp` / WASM screen updates)
- Add WebKit browser project to Playwright config (Chromium mobile emulation is sufficient for grid + DOM regression; real Safari verification remains manual or Appium backlog)
- Fix mod-bay or switch labels (only `.knob-label-main` in scope unless same root cause found)

## Decisions

### 1. Fix cascade by ordering, not `!important`

Move the mobile `.knobs { grid-template-columns: repeat(2, …) }` block to **after** the base `.knobs` rule (either relocate the entire `@media (max-width: 720px)` knob section or add a trailing mobile-only override).

**Rationale:** Same specificity, source order is the correct CSS fix. `!important` would fight future layout changes.

**Alternative rejected:** Inline styles from JS — violates separation of layout concerns and duplicates column logic already in CSS.

### 2. Replace `-webkit-box` label stack with wrap-friendly rules

Remove:

```css
display: -webkit-box;
-webkit-box-orient: vertical;
-webkit-line-clamp: 2;
overflow: hidden;
```

Replace with:

```css
white-space: normal;
overflow-wrap: break-word;
line-height: 1.15;
min-height: 2.3em;
text-align: center;
```

Optionally add standard `line-clamp: 2` with `display: -webkit-box` only if truncation is still needed at 2-col width (~175px) — at 2 columns, full names like `Cross-coupler` fit without clamp.

**Rationale:** `-webkit-box-orient: vertical` is a known footgun (stripped by some optimizers; inconsistent on Safari). At ~175px column width, clamp is unnecessary.

### 3. Relocate mobile knob grid + label rules after base `.knobs`

Move the mobile `@media (max-width: 720px)` knob block (currently lines 453–491: `grid-template-columns`, label typography) to **after** the base `.knobs` grid definition (line 494–500).

**Do not touch** the `@media (min-width: 721px) { .knobs { grid-column: 1 } }` block at line 657 — that is desktop wide-layout only and does not set column count.

Leave the mod-bay `@media (max-width: 720px)` block at line 308 separate (different concern).

### 4. Playwright spec: `web/e2e/mobile-knob-labels.spec.ts`

- `test.use(MOBILE_USE)` (reuse constant from helpers)
- `startSimAudio(page)` to populate labels
- Assert `gridTemplateColumns.split(/\s+/).length === 2`
- Assert `#status` playing + label text/visibility for known params
- Navigate to audio page for pair-AR label spot check

Shared label strings: import from `sim/ParamDisplayNames.hpp` is not available in TS — duplicate minimal constants in `test-shared/simSelectors.ts` only for asserted sample names (same pattern as button labels).

### 6. Static labels without Play

Add `web/src/paramDisplayNames.ts` mirroring `ParamDisplayNames.hpp`. On init and UI page change, call `applyKnobLabelsFromRows([], [])` so static names show immediately. WASM `onScreenUpdate` passes full row arrays to override when engine is synced.

**Page navigation bug (fixed):** `setHostPage` / `changeHostPage` must not pass `lastScreenRows` — stale Audio rows would label the Drive page `VCO1` while audio plays.

**Fallback:** use `wasmName || coreKnobLabel(...)` not `??` so empty WASM strings fall back to static names.

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| 2-col grid increases vertical scroll on long pages | Acceptable; matches original mobile design intent |
| Playwright passes but Safari still clips | Manual iOS verification task; simplify label CSS specifically for Safari |
| `.knob-col { overflow: clip }` clips multi-line labels after wrap fix | Post-fix visual check; add mobile-only `overflow: visible` on `.knob-col` if needed |
| Duplicate `@media (max-width: 720px)` blocks remain (mod-bay line 308, field line 447) | Relocate knob rules only; do not merge mod-bay into this diff |
| Playwright label-height assertion passes at 4-col in Chromium | Assert `gridTemplateColumns` track count === 2 **before** label visibility checks |

## Migration Plan

1. Apply CSS fix in `web/src/style.css`
2. Add Playwright spec; run `npm run test:e2e` locally
3. Deploy via existing GitHub Pages pipeline on merge to `main`
4. Rollback: revert CSS ordering change (single-file revert)

## Open Questions

- None blocking apply. Optional: add WebKit project to Playwright after `npx playwright install webkit` if CI budget allows.
