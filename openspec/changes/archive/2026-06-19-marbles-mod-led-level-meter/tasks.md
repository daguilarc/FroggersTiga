## 1. Shared curve authority

- [x] 1.1 Add `sim/ModLedBrightness.hpp` with `kModLedFullBrightnessCv` (0.55) and clamped `ModLedDisplayBrightness(cv01, active)` using `normalized * normalized`
- [x] 1.2 Add `sim/ModLedBrightness_test.cpp` clamp/golden cases: negative active CV → 0, `(0,true)→0`, `(0.275,true)→0.25`, `(0.55,true)→1`, `(0.8,false)→0`, `(1,true)→1`, above-range active CV → 1; wire into sim CMake and CTest
- [x] 1.3 Extend `scripts/generate-host-display.mjs` to emit the threshold and `modLedDisplayBrightness` into the existing `web/src/hostDisplay.generated.ts`; retain the existing `--check` path

## 2. Host UI wiring (all four sim hosts)

- [x] 2.1 `desktop/Source/ModModuleBox.cpp` — remove local threshold, draw the dark LED base, then overlay green using `ModLedDisplayBrightness(m_lastLevel, m_audioRunning)` alpha
- [x] 2.2 `web/src/ModLedIndicator.ts` — import `modLedDisplayBrightness` from `hostDisplay.generated.ts`; initialize/update `data-brightness` and `--mod-led-brightness`; remove `data-on` / `LED_ON_THRESHOLD`
- [x] 2.3 `web/src/style.css` — preserve a dark LED base and drive a green overlay's opacity from `--mod-led-brightness`; remove `[data-on="true"]`
- [x] 2.4 `vcv/src/plugin.cpp` — `LIGHT_MOD_RANDOM1/2` use `ModLedDisplayBrightness(GetCvOut(5/6), true)`; include `ModLedBrightness.hpp`

## 3. Playwright e2e

- [x] 3.1 Add `web/e2e/marbles-mod-led-level.spec.ts`: assert idle zero; after Play + Rand Resample assert numeric/live brightness, no `data-on`, and CSS-property parity
- [x] 3.2 In the same spec, import `modLedDisplayBrightness` from `hostDisplay.generated.ts` and assert clamp, inactive, and golden curve points
- [x] 3.3 Rebuild WASM, then run `cd web && npm run test:e2e` locally; confirm the new spec passes

## 4. Docs and tooltips

- [x] 4.1 Update `desktop/Source/ModRackPanel.cpp` tooltips — level-proportional green, full at ~55% CV
- [x] 4.2 Update `SIM_MANUAL.md` mod-rack Random LED section and add the fix beneath the current version-history heading; do not bump the CMake/package version; run `scripts/sync-help-docs.sh`
- [x] 4.3 Update `vcv/DEVELOPMENT.md` LED threshold note to reference shared proportional curve

## 5. Manual smoke (all hosts)

- [x] 5.1 Desktop: start audio, Rand Resample, confirm Random LEDs dim/brighten with held CV; idle = dark
- [x] 5.2 VST/AU: same check in hosted editor mod rack
- [x] 5.3 VCV: patch primary module; confirm Random output LEDs track CV level continuously
- [x] 5.4 Run `scripts/verify_clean_rebuild.sh` (includes sim build/CTest, web checks, and desktop/VST build)
