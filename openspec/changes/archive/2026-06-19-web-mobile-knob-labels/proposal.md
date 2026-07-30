## Why

On mobile web (≤720px viewport), users report parameter knob labels truncate to nothing or stay on the `…` placeholder. Two root causes:

1. **CSS cascade** — base `.knobs { repeat(4) }` was declared after the mobile `@media` override, forcing four ~68px columns on phones.
2. **Label data flow** — labels only came from WASM screen updates after Play; init and page navigation used `…` or stale WASM rows when `lastScreenRows` was empty or from the previous page.

## What Changes

- **Fix CSS cascade** so mobile `.knobs` uses a **3-column** grid (not 4)
- **Static label authority** — labels from `paramDisplayNames.ts` on init and every page change; WASM overrides when synced
- **Playwright regression** — 3-column grid and label visibility on load and page navigation (no audio required)

## Capabilities

### New Capabilities

- `web-mobile-knob-labels`: Mobile grid layout, always-visible knob labels, Playwright coverage

### Modified Capabilities

- (none in `openspec/specs/`)

## Impact

- `web/src/style.css` — grid cascade + mobile label typography
- `web/src/paramDisplayNames.ts` — static label table (parity with `sim/ParamDisplayNames.hpp`)
- `web/src/main.ts` — label apply on init/page change; WASM override in screen sync only
- `web/e2e/mobile-knob-labels.spec.ts` — static + post-Play + page-switch-while-playing tests
- `web/test-shared/simSelectors.ts` — label constants derived from `paramDisplayNames.ts`

## OMNI Audit Summary (post-apply)

| Check | Finding | Resolution |
|-------|---------|------------|
| Data flow | Labels need static source on init/page change; WASM is overlay when rows match page | `paramDisplayNames.ts` + `applyKnobLabelsFromRows([], [])` on UI page change |
| Stale WASM rows | `setHostPage` used `lastScreenRows` from previous page → wrong names while playing | Pass `[]` on UI navigation; WASM resync via `onScreenUpdate` |
| Empty WASM name | `??` kept empty string from WASM | Use `\|\|` fallback to static name |
| Repetition | Label strings in tests duplicated `ParamDisplayNames.hpp` | `simSelectors` imports `coreKnobLabel` / `pairArKnobLabel` |
| Repetition | `paramDisplayNames.ts` vs `.hpp` table | Accepted: client-side mirror until codegen; tables verified identical |
| Cascade | Base `.knobs` before `@media`; mobile override wins at ≤720px | Applied in `style.css` |
| Verification | Tests must cover before-Play and page-switch-while-playing | 5 mobile label tests added |
