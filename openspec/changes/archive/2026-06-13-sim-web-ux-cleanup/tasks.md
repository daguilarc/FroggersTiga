## 1. Delay page layout parity

- [x] 1.1 Remove `delay-page` class toggle from `renderPageChrome`; remove `delay-pill` class from page pill buttons; delete `.page-chrome.delay-page`, `.page-pill.delay-pill.active`, and unused `--delay-accent` CSS
- [x] 1.2 Delete `applyDelayKnobHints` mod-gating; keep hint text static for Delay row 0 only
- [x] 1.3 Add `.knob-hint { min-height: 1.2em; }` so empty hint slots preserve column height
- [x] 1.4 Remove `applyDelayKnobHints` call from `syncKnobUi` and delete the function
- [x] 1.5 In `applyStaticKnobLabels`, always set hint slot `display: block` on all pages; empty text on non-Delay rows

## 2. VCO morph fixes

- [x] 2.1 `renderVcoMorphButtons`: show when `hostPage === 0` only (drop `wasmPage` gate)
- [x] 2.2 Morph button click: guard with `requireEngineForAction()`; optimistic local morph cycle + SVG update
- [x] 2.3 Rename `#rand-morphs` button to **Rand waveforms** in `index.html`; guard click with `requireEngineForAction()`

## 3. VCO Envelope naming

- [x] 3.1 `ParamDisplayNames.hpp` Audio row 6: **VCO Envelope**
- [x] 3.2 `main.ts`: `INTERNAL_MOD_LABELS`, mod dropdown option, `HOST_PAGE_LABELS` Audio row 6, mod scope label

## 4. Marbles LED + S&H labels — web

- [x] 4.0 Refactor `renderModBay`: one loop over mixed indicator list (scope vs LED); no duplicate push/draw blocks
- [x] 4.1 Add `ModLedIndicator.ts` — green on when level > 0.55, dim off otherwise
- [x] 4.2 Replace Marbles 1/2 `CvScopeCanvas` entries in mod bay with LED; keep VCO Envelope as scope
- [x] 4.3 `renderModBay`: drive LEDs from `modLevels[5]` and `modLevels[6]`
- [x] 4.4 `INTERNAL_MOD_LABELS` and mod `<select>` options: **Marbles 1 S&H**, **Marbles 2 S&H**

## 5. Marbles LED + S&H labels — desktop

- [x] 5.1 `ModModuleBox`: for mod indices 5–6, paint LED circle instead of refreshing `CvScopeDisplay`
- [x] 5.2 Hide or skip scope bounds for Marbles modules; keep jack + label layout
- [x] 5.3 `ModRackPanel`: module titles **Marbles 1 S&H**, **Marbles 2 S&H**; update tooltips to mention S&H

## 6. Docs sync

- [x] 6.0 Run `npm run sync:docs` once after all label renames (VCO Envelope, Marbles S&H, Rand waveforms)

## 7. Verify

- [x] 7.1 Web: Delay page — Randomize mod → uniform column heights, no orange border
- [x] 7.2 Web: Audio page — morph click cycles SVG; Rand waveforms updates all three
- [x] 7.3 Web + desktop: Marbles LED toggles green on step while playing; labels show **Marbles 1 S&H** / **Marbles 2 S&H**
- [x] 7.4 Web mod dropdown shows **VCO Envelope** not **VCO level**
- [x] 7.5 `cd web && npm run build`; desktop build

## 8. Archive

- [x] 8.1 Archive change; merge specs into `openspec/specs/`
