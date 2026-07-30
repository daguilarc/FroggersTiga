## Why

FroggersTiga v1 desktop and VST surfaces expose all six FX submodule columns at once, route modulation through patch cables, and limit VST MIDI to DAW automation of 107 discrete host parameters with no raw MIDI ingest. The Sheaf synth miniapp demonstrates a tighter control model—encoder rings, page carousel, scene/gesture/shift semantics, lit modulation-source cells, and multi-VCO scope visualization—that maps better to hardware-style performance. A v2 fork preserves the proven `src/core/` + `sim/` engine while replacing desktop/VST chrome, moving envelope shaping to a dedicated ADSR module page, and adding a **global Crunchy** master that works **with** per-page **Crispy** (not instead of it).

## What Changes

- **BREAKING (v2 surfaces only):** Add `desktop-v2/` standalone app and `FroggersTigaPluginV2` (VST3/AU) as parallel products; v1 desktop and VST remain buildable unchanged.
- Replace six simultaneous submodule columns with a **module carousel** (left/right arrows) showing one host module at a time: Audio, Random, Drive, Filter, Reverb, Delay, **ADSR** (7 modules).
- **Module vs scene vocabulary:** carousel navigates **modules** (FX blocks); global strip **Scene** buttons morph stored preset values (Sheaf scene semantics).
- Replace patch-cable mod assignment with **lit modulation-source boxes** per knob row; assign via dropdown or randomization.
- **Fuegoization:** **Global Crunchy** = global fuego on **all** rows on **all** pages, **including every Crispy instance**. Per-page **Crispy** (row 7 on Audio; row 9 on expanded modules 1–5 and ADSR) stacks a second fuego pass on that page's musical rows. Neither replaces the other.
- Expand internal mod sources to six envelope followers (indices 7–12) plus two Marbles (indices 13–14) — **8 total**; scope grid (6 EF traces) + Marbles LEDs; catalog defined in `sim/V2ModSourceCatalog.hpp`.
- **Encoder rings** (Sheaf-style): scene L/R concentric rings, blended indicator dot, mod/gesture badges, **min/max reachability arcs in v2.0** with unified Smart Grid modulation math on the audio path.
- **Scenes:** three scene ordinals (S1/S2/S3) with left/right endpoints and blend slider; scene storage is global across all modules.
- **Gestures:** **two** gesture lanes at launch (G1/G2) with independent selection and value controls; gesture badges on encoder rings.
- **Shift:** keyboard + MIDI-assignable; documented interaction matrix (see design.md).
- **Sequencing:** **full step sequencer in v2.0** — transport BPM, pattern length 4–64, per-step scene capture, gate for ADSR, optional MIDI clock sync.
- **Audio I/O:** desktop v2 defaults to **stereo output** (inherits v1 `initialiseWithDefaultDevices(0, 2)`); mono output supported when device has one channel (`applyStereoBus` downmixes).
- **VST v2:** stereo output bus default, mono input for external audio; `acceptsMidi() true`; every parameter DAW-mappable.
- Adopt Sheaf control principles via `FroggersV2ControlCore`; reimplement JUCE visuals in FroggersTiga tree (no Sheaf source vendoring).
- **Web verification:** extend existing Playwright e2e (`web/e2e/`, `web/test-shared/simSelectors.ts`) for the web v2 parameter subset — ten-row pages 1–5, Crunchy in global strip, new Filter labels; v1 e2e specs remain green.

## Capabilities

### New Capabilities

- `desktop-v2-page-carousel`: Seven-module carousel; ten rows on expanded pages (see `desktop-v2-module-expansion`).
- `desktop-v2-module-expansion`: +2 rows on Random/Reverb/Filter/Drive/Delay; parallel Filter comb/peak; Scoop notch.
- `desktop-v2-adsr-page`: ADSR module with nine per-VCO gated ADSR params + page-local Crispy; replaces pair-AR.
- `desktop-v2-encoder-rings`: Scene L/R ring model, badges, interaction matrix, bank slots for hardware encoders.
- `desktop-v2-mod-source-grid`: Eight internal mod sources (six envelope followers, two Marbles), lit cells, dropdown assignment.
- `desktop-v2-scope-visualization`: Color-coded EF scope grid (indices 7–12); Marbles LEDs (indices 13–14).
- `desktop-v2-global-controls`: Extended strip including **Crunchy** (global fuego), Shift/Gesture 1/Gesture 2/LFO/VCO, scenes, sequencer transport, v1 randomization buttons.
- `desktop-v2-sequencing`: Full step sequencer with BPM, pattern length, per-step scene capture, ADSR gate, VST host params.
- `desktop-v2-midi-cv-input`: Single MIDI input with assignable pitch/gate/CC targets.
- `desktop-v2-control-core`: Message bus, scenes, gestures, shift, banks, modulation drill-down, UIState snapshots.
- `desktop-v2-audio-io`: Stereo-default desktop output; documented mono fallback and VST bus layout.
- `vst-v2-midi-modulation`: Full v2 parameter surface with DAW MIDI routability.
- `web-v2-parameter-subset`: Web gets +2 module rows, Filter parallel/Scoop, global Crunchy; keeps v1 web UX; **Playwright e2e** coverage for expanded rows and Crunchy.

### Modified Capabilities

- `froggers-host-master`: `SimHostKind::DesktopV2` / `VstV2`; 7 host pages; v2 mod catalog; global Crunchy; ADSR page; **Web** gets module expansion + global Crunchy only.
- `juce-vst-cc-mod-gating`: VST v2 accepts DAW MIDI→parameter routing.
- `midi-cc-to-mod-cv`: v2 unified MIDI CV input.
- `mod-rack-dual-midi-jacks`: v2 removes dual CC scope cells.
- `desktop-host-panel-column-order`: v2 uses carousel (v1 unchanged).
- `audio-pair-ar-engine` / `audio-pair-ar-desktop-ui`: superseded on v2 hosts by ADSR page (v1 unchanged).

## Impact

- **New:** `desktop-v2/`, `sim/V2ModSourceCatalog.hpp`, `sim/V2ParamDisplayNames.hpp` (or extend `ParamDisplayNames` for v2), `src/core/VcoAdsrState.hpp`, `sim/V2EnvelopeFollowerBank.hpp` (six EF taps at audio rate).
- **Removed on v2 only:** Audio pair-AR band UI.
- **Host parameters:** VST v2 inventory replaces pair-AR with ADSR×9 + global Crunchy + depths; count fixed in `HostParameterInventoryV2.hpp`.
- **Stereo:** v1 desktop already calls `initialiseWithDefaultDevices(0, 2)`; v2 codifies stereo default in spec; engine remains mono-internally with stereo spread via `applyStereoBus`.
- **Web/WASM:** expanded module parameters (pages 1–5), parallel Filter DSP, global **Crunchy** knob — **not** encoder rings, scenes, ADSR page, or v2 mod grid (see `web-v2-parameter-subset`).
- **Web touchpoints:** `wasm/bindings.cpp`, `WasmSimHost`, `web/src/main.ts`, `hostDisplay.generated.ts`, `web/e2e/` (Playwright), `web/test-shared/simSelectors.ts`.
- **Branch policy:** All implementation on a feature branch until desktop v2 + web subset pass local QA; **no merge to `main`** until sign-off (see `design.md` Migration Plan).
- **Out of scope:** Daisy firmware, VCV.

## Resolved in artifacts

| Topic | Decision |
|-------|----------|
| Encoder ring voices | Scene L outer, Scene R inner, blended dot |
| Range arcs | **v2.0** with unified Smart Grid math (not deferred) |
| Crunchy / Crispy | Global Crunchy fuego on **all** rows incl. Crispy instances; per-page Crispy stacks on musical rows |
| Scenes | 3 ordinals (S1/S2/S3), global storage |
| Gestures | **2 lanes** at launch (G1/G2) |
| ADSR DSP | **Full gated ADSR per VCO** (1B) |
| Sequencing | **Full sequencer in v2.0** |
| VST params | **Both** flat stable IDs + grouped display names |
| Rand All | All modules + ADSR + global Crunchy; skip scene metadata |
| VST buses | Mono in, stereo out (same as v1 plugin) |
| Internal mod sources | 6 envelope followers + 2 Marbles (indices 7–14, 8 total) |
| Crispy row index | Audio row 7; expanded modules 1–5 and ADSR row 9 |
| Module rows | +2 on Random/Reverb/Filter/Drive/Delay; Filter parallel comb/peak + Scoop; Audio unchanged |
| Web scope | Params + global Crunchy only; no v2 chrome |
| Web verification | Playwright e2e: expanded pages 1–5, Crunchy strip, new labels; v1 e2e green |
| Crispy row authority | `CrispyRowForPage(hostPage)` table in `V2ParamDisplayNames` / v2 layout |
| Branch / merge | Feature branch until local desktop + web QA; v2 builds OFF in CI |
