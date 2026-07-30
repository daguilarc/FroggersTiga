> **Reconciled (omni 1.2):** CSS + Playwright complete. Label authority superseded by omni §3 `hostDisplay.generated.ts` (handwritten `paramDisplayNames.ts` removed). Task 4.2 optional iPhone remains open.

## 1. CSS cascade fix

- [x] 1.1 Move base `.knobs { display: grid; grid-template-columns: repeat(4, …) }` block **before** mobile `@media (max-width: 720px)` overrides
- [x] 1.2 Mobile `.knobs` uses `grid-template-columns: repeat(3, minmax(0, 1fr))` at ≤720px (3 columns per row)
- [x] 1.3 Leave `@media (min-width: 721px) .knobs { grid-column: 1 }` unchanged — desktop only
- [x] 1.4 Replace mobile `.knob-col .knob-label-main` `-webkit-box` / `-webkit-line-clamp` stack with wrap-friendly rules
- [x] 1.5 `overflow: clip` on `.knob-col` — not needed; label height checks pass

## 2. Static label authority

- [x] 2.1 Static label authority via `web/src/hostDisplay.generated.ts` (generated from `sim/ParamDisplayNames.hpp`; supersedes handwritten `paramDisplayNames.ts` per omni §3)
- [x] 2.2 Apply static labels on init and every UI page change via `applyKnobLabelsFromRows([], [])`
- [x] 2.3 WASM screen sync overrides when row arrays match expected lengths; use `\|\|` fallback for empty WASM names
- [x] 2.4 Fix stale-row bug: UI page navigation must not pass `lastScreenRows` from previous page

## 3. Playwright regression

- [x] 3.1 Label constants in `simSelectors.ts` derived from `hostDisplay.generated.ts` (`coreKnobLabel`, `pairArKnobLabel`)
- [x] 3.2 Labels + 3-column grid visible on load (no audio)
- [x] 3.3 Page pill navigation updates labels (no audio)
- [x] 3.4 Stale WASM row regression when engine happens to be running

## 4. Verification

- [x] 4.1 Run `npm run test:e2e` — all tests pass
- [ ] 4.2 Optional manual check on real iPhone Safari
