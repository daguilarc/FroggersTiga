## Why

Froggers v2 still owns a bespoke JUCE shell (`Main.cpp` / `MainComponent`) while Sheaf’s product shape is **Runtime (JUCE wrapper) + `SynthApplication` (JUCE-free app)**. Operator intent is to boot desktop Froggers through that Runtime, harmonize visualization/modulation UX with Sheaf’s portable visualizer and encoder skeleton, absorb the deferred unified-parameter layout, and delete the Froggers-only Random S&H **module page** (Sheaf random sources are mod lanes with ganged visualizers — no bag/deja-vu page params).

## What Changes

- Desktop boots via vendored Sheaf **Runtime** / **Engine**; Froggers becomes a `SynthApplication` (`Config` / `Init` / `ProcessBlock` / `PortableSurface`). **BREAKING** for any product assumption that the `MainComponent` shell (via owned `AudioEngine`) is the primary audio-device / File/Audio/Controllers host.
- Application surface uses Sheaf **portable visualizer** skeleton: **two** multi-layer scopes (3 VCOs overlapping; 3 LFO EFs overlapping) and encoder underlays (including `GangedRandomLfoVisualizer` on Random S&H depth cells).
- Modulation drill-in is **exactly two layers** (module param → 16-cell mod page); no recursive mod-on-mod pages.
- **BREAKING:** Remove Random S&H **module page** and its parameters (Step chance, Deja vu, Bag size, Slew, expansion tails, page Crispy) from manifest, host inventory, engine, and presets. Keep **Random S&H 1/2** as modulation **lanes** only.
- **Absorb** `desktop-v2-unified-parameter-layout`: retire carousel; unified parameter surface; relocate transport/global chrome beside scopes; **remove** the cross-coupler entirely (no bipolar, no explicit couplers — redundant with the drilldown matrix, D11); VCO morphs labeled "Shape"; ASR **Envelope** (Attack/Sustain/Release, retire Pair-AR naming). Cancel/archive the standalone unified-layout change when this change is accepted.
- First-layer modules use a **≤16** encoder bank (4×4 grid; sparse cells OK) with stable Global **Crunchy** + local **Crispy** placement.
- Randomization UX: **toggle = global**; **held = next-click local** (deliberate Froggers divergence from Sheaf hold-while-press).
- **AMENDMENT (D16, 2026-07-24 — reverses D2's param deferral):** the param + randomization *model* migrates onto Sheaf-native `ParameterManager`/`ParameterGroup`/`Bank`/`StandardModulators` + `Parameter::RandomizeVisibleValue`/`Bank::RandomizeModulationDepths`. Crunchy/Crispy become ordinary Sheaf parameters; Random S&H sources become `StandardModulators` (which feed their ganged visualizers). The bespoke `FroggersV2ControlCore` randomization authority is **retired** — wherever this proposal or the specs say "existing randomization authority" / name `FroggersV2ControlCore` as the model, read it as the Sheaf `Bank`/`ParameterManager` model. DSP unchanged (migration stops at the `FroggersV2HostBridge` boundary). See design **D16** + tasks **§13**. Specs (`desktop-v2-control-core`, `desktop-v2-rand-arm-gesture`) reconciled to D16 (2026-07-25); `desktop-v2-mod-source-grid`/`desktop-v2-global-controls` were already behavioral/D16-compatible.
- **AMENDMENT (2026-07-25):** randomization scope is **per-page + global** (values and mod-depths), expressed natively in Sheaf. **BREAKING — sequencer + gesture-weight are DELETED** from the desktop-v2 product (legacy old-Sheaf cruft; an earlier D16 draft wrongly planned to keep them bespoke). **Scenes are kept.** Shared `sim/SequencerState.hpp` + `src/core/DesktopHostIO.hpp` sequencer/gate infra stays for v1 desktop + sim — only desktop-v2 consumption is removed. Locked decisions: RNG = Sheaf mt19937; modulation resolution = Sheaf per-sample slew; mod-depth rand = all connected lanes.
- **Out of scope:** MIDI mapping / profile automation; VST/AU Runtime host (explore later).

## Capabilities

### New Capabilities
- `desktop-v2-portable-visualizers`: Sheaf-style portable `Visualizer` / `ScopeVisualizer` / `GangedRandomLfoVisualizer` integration on Froggers Application surface and mod-depth encoders.
- `desktop-v2-rand-arm-gesture`: Froggers rand toggle (global) vs held next-click-local one-shot arm.

### Modified Capabilities
- `froggers-v2-sheaf-runtime`: desktop SHALL boot through Runtime; app SHALL satisfy `SynthApplication`; local vendor inventory; Controllers shell without mapping redesign.
- `desktop-v2-scope-visualization`: dual multi-layer scopes (VCO trio + LFO EF trio); retire single EF-only global tap set as sole viz.
- `desktop-v2-mod-source-grid`: 2-deep max; 16-cell detail; Random S&H lanes keep visualizers; no Random module page.
- `desktop-v2-control-core`: message/path support for 2-deep gate, rand arm, page inventory after Random deletion, 16-slot bank.
- `desktop-v2-page-carousel`: retire single-active-page carousel in favor of unified Application surface.
- `desktop-v2-grid-layout` / `desktop-v2-module-column-layout`: unified module-section geometry; chrome beside scopes.
- `froggers-v2-app-manifest` / `froggers-v2-product-contract`: page list without Random; VCO 1/3; Envelope ASR; host-param count re-baseline.
- `desktop-v2-global-controls`: Crunchy/Crispy bank stability; rand chrome semantics per `desktop-v2-rand-arm-gesture`.
- `desktop-v2-module-expansion`: remove Random page expansion rows; Envelope/Audio content updates from absorbed unified layout.

## Impact

- **Code:** `desktop-v2/` shell (`Main.cpp`, `MainComponent`), control core, manifest/inventory, scope UI, carousel/layout, AudioEngine bindings for deleted Random page params; vendor tree for Sheaf Runtime/Engine/portable UI/visualizers.
- **Presets / host state:** **BREAKING** — page indices and host-parameter IDs shift when Random page is removed; migration or drop of obsolete page-1 axes required.
- **OpenSpec:** absorbs and retires live change `desktop-v2-unified-parameter-layout`.
- **Docs:** `SIM_MANUAL` / `QUICK_DICT` Random page sections removed; Random S&H described as mod lanes + visualizer only.
- **Non-impact this change:** MIDI Controllers mapping UX; VST editor host path; web parameter subset.
