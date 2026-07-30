## Why

Random mod rack LEDs (indices 5 and 6) on desktop, web, VST/AU, and VCV currently snap on/off at a fixed 55% CV threshold. That binary behavior hides the magnitude of the held Random CV. Proportional brightness makes weaker held values visibly dimmer and full green only near the reference level, improving mod-source readability without changing DSP or mod routing.

## What Changes

- Introduce one shared, clamped quadratic brightness curve in `sim/` mapping mod CV (0–1) to display brightness (0–1), reaching **1.0 at CV ≥ 0.55** when audio/transport is active.
- Apply the curve on **all four sim hosts** for Random 1 and Random 2 LED cells (mod indices 5 and 6): desktop standalone, web/WASM, VST/AU editor, and VCV primary module.
- Replace `data-on` boolean styling on web with both an inspectable `data-brightness` value and a numeric CSS custom property that drives the rendered accent opacity.
- Add sim unit tests for the shared curve and a focused **Playwright e2e** spec for the web DOM contract.
- Update operator docs (`SIM_MANUAL.md`, VCV `DEVELOPMENT.md`, mod-rack tooltips) to describe level-proportional LEDs instead of a hard on/off threshold.

## Capabilities

### New Capabilities

- `mod-led-level-meter`: Cross-host proportional brightness for Random mod rack LEDs (indices 5 and 6), shared curve authority, host presentation rules, and automated verification (sim tests + Playwright).

### Modified Capabilities

- `froggers-host-master`: Mod-rack LED presentation for indices 5 and 6 SHALL be level-proportional on desktop, web, VST/AU, and VCV (not binary threshold).

## Impact

- `sim/ModLedBrightness.hpp` (new) — shared curve + constants
- `sim/ModLedBrightness_test.cpp` (new) — golden curve points
- `scripts/generate-host-display.mjs`, `web/src/hostDisplay.generated.ts` — extend the existing generated web projection with the curve constant/function
- `desktop/Source/ModModuleBox.cpp` — alpha from shared curve
- `web/src/ModLedIndicator.ts`, `web/src/style.css` — continuous brightness
- `vcv/src/plugin.cpp` — `setBrightness` from shared curve for `LIGHT_MOD_RANDOM1/2`
- `web/e2e/marbles-mod-led-level.spec.ts` (new) — Playwright regression
- `SIM_MANUAL.md`, `vcv/DEVELOPMENT.md`, `desktop/Source/ModRackPanel.cpp` tooltips
- No DSP, preset, parameter ID, or `HostPanelLayout::kModRackCatalog` topology changes
- No desktop package-version bump, new desktop tag, or new GitHub Release; document the fix under the current `SIM_MANUAL.md` version-history heading for the single `froggerstiga-v1` channel
