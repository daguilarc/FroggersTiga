## Why

The Audio page controls three VCOs and cross-coupling, but there is no way to shape **pair-sum dynamics** — how the (VCO1+VCO2) and (VCO2+VCO3) contributions rise and fall over time. Operators need attack and release times per pair sum, with mod CV like the existing vertical rows.

Separately, the global-strip **Random** label is ambiguous: **Rand All**, **Rand Mods**, and **Rand waveforms** say what they randomize; **Random** only steps the two marbles S&H bags (manual: *draw* from each bag). That mismatch confuses desktop, web, and future VCV silkscreen.

## What Changes

- **Engine:** four new Audio-only parameters — **Attack 1+2**, **Release 1+2**, **Attack 2+3**, **Release 2+3** — applied as AR envelopes on the respective pair sums in `FroggersEngine` before the main osc mix
- **Param authority:** labels and cell order live in `ParamDisplayNames.hpp` / `AudioPairArLayout.hpp` (single table); not duplicated in desktop, web, or docs
- **Desktop:** additional **horizontal band** at the bottom of the Audio `SubModulePanel` only — four columns, each stacked **jack → knob → label** (inverted vs the vertical rows above)
- **Web:** third row of four knobs on the Audio page (same 4-column grid as rows 1–2); standard label → knob → mod-source column layout — no inverted band
- **Mod routing:** each pair-AR param accepts the same mod sources as page rows; patch overlay collects four new input ports on Audio page
- **Persistence:** include four knob values + mod source/depth in `SimPresetSnapshot` (version bump)
- **Docs:** `SIM_MANUAL.md` + synced copies — pair-AR semantics (Release covers decay+release; not reverb “Decay”)
- **Global strip rename (all sim hosts):** marbles step button **Random** → **Rand Resample** via `ParamDisplayNames::forGlobalStrip()`; align desktop **Rand waves** typo with **Rand waveforms**; update manual, quick-dict, web hint (`m` key)

**Non-goals (this change):** VCV Rack panel implementation (defer label to `vcv-rack-field-parity` apply using same constant), Daisy Field firmware, renaming reverb row “Decay”

## Capabilities

### New Capabilities

- `audio-pair-ar-engine`: Pair-sum AR envelope DSP, host IO, WASM bindings, snapshot fields
- `audio-pair-ar-desktop-ui`: Horizontal four-cell band on Audio `SubModulePanel`
- `audio-pair-ar-web-ui`: Third knob row on Audio page only
- `global-strip-marbles-label`: Rename marbles step button to **Rand Resample** on desktop + web from single label table

### Modified Capabilities

- (none in `openspec/specs/` — baseline specs not archived on public main)

## Impact

- `src/core/FroggersEngine.hpp` — pair-sum AR processing in `StepOscillators` / mix path
- `sim/ParamDisplayNames.hpp`, new `sim/AudioPairArLayout.hpp`
- `sim/SimPresetSnapshot.hpp`, `src/core/DesktopHostIO.hpp`, `sim/WasmSimHost.hpp`, `wasm/bindings.cpp`
- `desktop/Source/SubModulePanel.{h,cpp}`, `PatchCableOverlay` port collection
- `desktop/Source/GlobalStrip.{h,cpp}` — global strip labels from `ParamDisplayNames`
- `web/src/main.ts`, `web/index.html`, `web/src/style.css`, `web/public/froggers-processor.js` (rebuild)
- `SIM_MANUAL.md`, `docs/sim-manual.md`, `web/public/sim-manual.md`, `web/public/quick-dict.md`
