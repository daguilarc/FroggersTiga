## 1. Change hygiene + absorption

- [x] 1.1 Point `desktop-v2-unified-parameter-layout` at this change (cancel/archive note in its proposal) — Random S&H section not absorbed
- [x] 1.2 Add Sheaf adoption inventory stub under this change (source path → Froggers path → notes) — `sheaf-adoption-inventory.md`
- [x] 1.3 Record preset migration default: silent drop of deleted Random page host axes on load — locked in `design.md`

## 2. Vendor Runtime + portable visualizers

- [x] 2.1 Vendor Engine, Runtime, Shell/MainPane, portable UI, ScopeWriter, ScopeVisualizer, GangedRandomLfoVisualizer (and required headers) into FroggersTiga in-tree — **pin Sheaf ≥ `c1810393`** (includes Phase1/2 param processing, ADSR DSP, ButtonGrid/RuntimeUIState, JUCE value-action prefix fix)
- [x] 2.2 Wire CMake so desktop-v2 builds vendored sources without FetchContent of Sheaf; compile through new `GridManager` / `RuntimeUIState` even if FroggersApp leaves grids unused
- [x] 2.3 Complete adoption inventory for every vendored file; gate: `rg` shows no network Sheaf fetch in product build; note browser-catalog tree is **not** vendored

## 3. FroggersAppCore under Engine

- [x] 3.1 Implement `FroggersApp` / `FroggersAppCore` satisfying `SynthApplication` (`Config`, `Init`, `ProcessBlock`, `PortableSurface`)
- [x] 3.2 Delegate audio to existing AudioEngine + control core / facade path
- [x] 3.3 Headless Engine/rig (or equivalent) test: Init → ProcessBlock → finite stereo; extend or replace facade parity coverage

## 4. Delete Random S&H module page + bag params

- [x] 4.1 Remove Random S&H page from manifest, `V2ParamDisplayNames`, `HostParameterInventoryV2`, control-core `rowsForPage` / page indices
- [x] 4.2 Remove engine/host bindings for Step chance, Deja vu, Bag size, Slew, Random expansion tails, Random page Crispy
- [x] 4.3 Keep Random S&H 1/2 in `kPermanentModulationSources`; ensure DSP sources still run without page knobs (Sheaf-style defaults)
- [x] 4.4 Preset/host-state load drops obsolete Random page axes; re-baseline projection validators and host-param counts (preset silent-drop landed in fix wave 27f4a10 via AudioEngine::notifyStateRestored → Marbles::ResetPageToDefaults, desktop-v2-scoped; shared SimPresetSnapshot + v1 untouched)
- [x] 4.5 Update `SIM_MANUAL` / `QUICK_DICT`: no Random module page; lanes + visualizer only

## 5. Dual scopes + encoder visualizers

- [x] 5.1 Application surface: two ScopeVisualizer panels (3 VCO layers + 3 LFO EF layers), color-coded overlap
- [x] 5.2 Retire sole-viz role of EF-only `kOscilloscopeTaps` global scope
- [x] 5.3 Attach `GangedRandomLfoVisualizer` (and other Sheaf underlays required by published modulator metadata) on Random S&H mod-depth cells
- [x] 5.4 Tests: portable visualizer presence; dual-panel smoke

## 6. Mod drill-in depth gate (2 layers)

- [x] 6.1 Control core: reject nested ModDrillIn from depth cells; Target(Back) exits to layer 0 — today `onModDrillIn` (`FroggersV2ControlCore.cpp:711`) does not gate on `m_modView.open`
- [x] 6.2 UI: 16-cell detail grid remains sole mod editor; no recursive mod page
- [x] 6.3 Tests: second drill-in rejected; Target Back closes (minor: reject is a silent no-op, no UI deny-cue — deferred)

## 7. Absorb unified layout + Audio/Envelope content

- [x] 7.1 Fit spike at 1280×920 for modules **without** Random section; recorded in design "Layout addendum" — **Candidate A** (tabbed modules over one shared 4×4 grid, scopes+chrome in a top band)
- [ ] 7.2 Retire carousel *arrow-paging*: **reuse** `PageCarouselComponent`'s one-module page-swap machinery (per the addendum — A ≈ tabbed carousel), replace the arrow header with a 6-tab module selector, mount on the unified Application surface. Do NOT rewrite the page-swap model or add parallel page-state.
- [ ] 7.3 Relocate transport + global-command chrome beside dual scopes
- [x] 7.4 **Remove the coupler from the V2 product — no coupler of any kind on V2 (not bipolar, not three, none)** (D11, reconciled with D14): remove the bipolar `Cross-coupler` host/UI param row + the `crossCouplers` manifest emit, and on the V2 DSP path (`m_simIndependentPm` branch, `FroggersEngine.hpp:735-744`) keep **zero** cross-VCO terms. **Daisy/v1 guard:** the engine `XCPL` slot (`:611`, index 3) and the `c12`/`c23` legacy `else` branch (`:745-755`) are RETAINED byte-for-byte and flag-gated OFF for V2 — do NOT delete them. PM is self-contained per VCO on V2 (value = phase-mod frequency). **ALL VCO→VCO coupling is drilldown-matrix only, never hardcoded.** Audio page → 10 params. Flag-gate to V2 hosts (D14, Daisy/v1 untouched). Self-contained PM (D14 resolved): with no external mod assigned, each VCO's phase is modulated by a **separate sine LFO oscillator** at frequency = the PM knob value (no self-feedback, no cross-VCO); when a source is drilled in, the PM knob attenuates it (D12). Coupler UI removal + PM rework land together in the V2 path (removing the UI alone makes PM inert).
- [x] 7.5 Envelope ASR: Attack/**Sustain**/Release per VCO; retire Pair-AR naming; sustain-level semantics — DONE: Pair-AR→Envelope rename + full-word Attack/Release, module ported to portable UI (039f37f). DONE (D15): true per-VCO ASR added to `VcoAdsrState` (Hold holds at a per-voice sustain level instead of hardcoded 1.0; Attack-time knob normalized so attack duration is level-invariant), extended in place (no `synth::DspAdsr`, so Daisy/sim stay unaffected). Includes the prerequisite ADSR page-pointer fix (`m_pages[6]` -> `m_pages[5]`) the envelope was silently inert without. Triplet row order (Attack, Sustain, Release) x3 + Crispy; host param count 120 -> 126.
- [ ] 7.6 Per-module Randomize via existing `RandPage` authority (no parallel mutator)
- [ ] 7.7 `LayoutBounds_test` + validators re-baseline
- [x] 7.8 Label the three VCO waveform-morph controls **"Shape"** (D13d, continuous shape morph)

## 8. Sixteen-slot bank + Crunchy/Crispy stability

- [ ] 8.1 Single physical slot map (≤16); sparse disconnected cells OK; one authority only
- [ ] 8.2 Stable Global Crunchy + local Crispy slot identities across section changes; write exact indices into design addendum before closing this packet
- [ ] 8.3 Tests: section change does not renumber Crunchy/Crispy identities

## 9. Rand toggle / held next-click local

- [ ] 9.1 Implement toggle = global; held = one-shot local arm per `desktop-v2-rand-arm-gesture`
- [ ] 9.2 Wire global chrome affordances; no Shift-based held model
- [ ] 9.3 Tests: toggle global path; hold+click local once then clear; hold without click no-ops

## 10. Runtime shell cutover

- [ ] 10.1 Desktop entry uses Runtime main (`SYNTH_RUNTIME_MAIN` or equivalent)
- [ ] 10.2 Retire MainComponent ownership of AudioEngine-as-primary-host and File/Audio/Controllers as primary host
- [ ] 10.3 Runtime File/Audio/Controllers shell pages present; **no** MIDI mapping redesign
- [ ] 10.4 Desktop smoke: launch, scopes visible, modules editable, mod 2-deep, Random page absent

## 11. Verification + docs closure

- [ ] 11.1 Full desktop-v2 test suite + projection validators green at new inventory counts — run via **Task subagent** (OMNI §16.1); parent receives pass/fail counts + failure tail only
- [ ] 11.2 Manual QA checklist at 1280×920: dual scopes, unified surface, ASR Envelope, no Random page, rand arm
- [ ] 11.3 Explicit deferred notes in docs: MIDI mapping later; VST Runtime later

## 12. LFO EF = slow VCO EF; VCO EF / LFO EF labels (D13, V2-host-scoped)

- [x] 12.1 Add a slow-timescale envelope-follower pass populating the dead permanent-rack taps 8–10 from the VCOs (V2-scoped via `V2EnvelopeFollowerBank`; Daisy does not use it, D14) — slow attack/release coefficients so the "LFO EF" moves at LFO rate
- [x] 12.2 Relabel the `lfo_1/2/3` mod sources **`LFO EF 1/2/3`** (keep `VCO 1/2/3 EF`) in the mod-source catalog / display names so mod pages show `VCO EF` and `LFO EF` distinctly
- [x] 12.3 Repoint packet-5's LFO-EF `ScopeVisualizer` panel (`FroggersScopePanels`) off the dead raw-LFO taps onto the slow-EF (`LFO EF`) source; update `FroggersScopePanels_test` stableId assertions
- [x] 12.4 Tests: taps 8–10 non-zero and slower than taps 3–5 after signal; label assertions; scope panel bound to the slow-EF source

## 13. REVERSE D2 — migrate params + rand onto Sheaf ParameterGroup/Bank/StandardModulators (design D16)

Supersedes the bespoke `FroggersV2ControlCore`/`HostParameterInventoryV2` param+rand layer with Sheaf-native facilities, preserving behavior. Bridge boundary = `FroggersV2HostBridge`; the DSP (`src/core/`) is untouched. Build stays green at each step.

- [ ] 13.0 Delete dead code first (decision-free): `FroggersV2ControlCore::isAdsrPage` (`page==6`, `kNumHostPages==6`) has **zero callers** — delete the function, not re-point it. Remove the twin dead `page==6` branch in `crispyRowForPage` (`:94-96`) in the same step.
- [ ] 13.0.1 **Remove sequencer + gesture-weight from desktop-v2** (operator 2026-07-25; DELETE, not migrate). **Keep scenes** (`SceneState`/`SceneBlend`, live-`m_params` based — verified NOT to need `SequencerSlotPayload`). **DO NOT TOUCH shared infra:** `sim/SequencerState.hpp` (+`_test`), `src/core/DesktopHostIO.hpp` (`m_sequencer` + gate `:97,720-746`) — serve v1 desktop (`desktop/`) + sim. Gate-safety: note-onset fires on `m_gateHigh` rising edge (`m_gateTrigger`, `DesktopHostIO:704`), independent of the sequencer — triggering survives. Executed as 4 build-green sub-steps (leaves→inward):
  - [ ] 13.0.1a **UI leaves.** Delete `ui/SequencerPanelComponent.{hpp,cpp}`; SURGICALLY strip gesture members/logic from `ui/PerformanceBandV2.{hpp,cpp}` (KEEP scene + marbles; fix `resized()` width formulas after `m_gesture*`/`m_gestureWeight*` go); strip step-scope (`m_scopeAllSteps/m_scopeCurrentStep/m_allStepsScope/resolveRandSeqScope`) from `ui/GlobalStripV2` (KEEP scene-scope `m_scopeAllScenes/m_scopeCurrentScene`); remove mounts/wiring in `MainComponent.{h,cpp}` + `HostedMainComponentV2.{h,cpp}`; drop `kSequencerH` + `kPerfGesture*` from `ui/DesktopV2ChromeLayout.hpp`; drop the 4 gesture-bounds getters + `LayoutBounds_test` entries.
  - [ ] 13.0.1b **Control-core + bridge.** Remove `MessageIn::Type::{ResetSequencerStep,RandSequencerStep,RandSequencerMods,GestureSelect,GestureWeight}` + their `applyMessage`/`messageMutatesPatchContent` cases; delete `m_sequencer`/`setSequencerState` + every `SequencerSlotPayload`-taking method (`applySequencerSlotPayload`,`capture*SlotPayload`,`randomizeFullSlotPayload`,`randomizeModIntoSnapshot`,`captureModRoutesIntoSnapshot`,`zeroStepGestures`,`randomizeSceneSlotsInto(SequencerSlotPayload&)`) + gesture state (`m_gestureWeights`,`ParamState::gestureDepth`,`m_activeGestureLane`,`kNumGestures`); SURGICALLY trim `onRandMods` (KEEP `randomizeLiveModDepths()`, drop step-capture tail + `stepScope` arg) and `onRandAll` (KEEP scene/mod rand, drop the two gesture-zeroing loops). **FOOTGUN:** KEEP `randomizeSceneSlotsInto(uint8_t page[,sceneScope])` — only the `SequencerSlotPayload&` overload goes. Delete `FroggersV2HostBridge::{onSequencerStepAdvance,captureLiveToSequencerStep,recallSequencerStep}` + ctor/dtor hook wiring; drop `FroggersV2AppCoreFacade::initialize()`'s `setSequencerState` call; strip `MidiCvAssignmentTable` sequencer-advance hook/`onMidiClockTick`/`externalClock` (KEEP scene targets) + `AudioEngine.cpp:147-169` MIDI Start/Stop/Continue `m_playing` toggles + `MidiCvSettingsComponent` "Sequencer sync" row.
  - [ ] 13.0.1c **Inventory axes + count.** Remove `Axis::{Sequencer,GestureWeight}` + `k{Sequencer,GestureWeight}Count`/`kGestureLaneCount` from `HostParameterInventoryV2.hpp`; drop their branches in `HostParameterRoutingV2.hpp` (read/applyValue) + `FroggersV2AppManifest.hpp` (stableId/displayName/descriptor builder + sequencer metadata: `kSequencerSlots`,`SequencerSlot`,`"step"` scope control, related `ValidationSummary` fields). Closes the host-automation `m_playing` vector.
  - [ ] 13.0.1d **Test re-baseline.** `FROGGERS_EXPECT_HOST_PARAM_COUNT_V2` 126→**119**; delete `checkSequencerAuthority()` + `SequencerPanelComponent` conjunct in `checkHostedProjectionOverlay()`; go function-by-function through `ControlCoreBridge_test.cpp` (mixes scene+sequencer — PRESERVE scene assertions), `FroggersV2AppCoreFacade_test`, `GlobalControlParity_test`, `LayoutBounds_test`, `MidiCvAssignment_test`, `FroggersV2Manifest_test`.
- [ ] 13.1 Spike: one `ParameterManager`+`ParameterGroup`+`Bank` for the Audio page, glue writing into `DesktopHostIO::SetPageKnob`; headless test proves a Sheaf `Parameter` change reaches `Page::GetParam()`.
- [ ] 13.2 Rand parity spike: exercise `Bank::ApplyModifierToTopLevel(Random)` + `Modifier::RandomMod`. DECISIONS locked: mod-depth = **all connected lanes** (not Sheaf's subset example); RNG = **Sheaf mt19937** (no xorshift injection); mod resolution = **Sheaf per-sample slew** (`Parameter::ProcessSample`).
- [ ] 13.3 Port Crunchy (global bank cell) + Crispy (per-page bank cell) as ordinary Sheaf `Parameter`s; existing Crunchy/Crispy tests are the regression gate.
- [ ] 13.4 Port all 6 module pages' knobs + mod-depth lanes to `ParameterGroup`/`Bank`, one page at a time, behind the validated bridge shape.
- [ ] 13.5 Repoint `FroggersAppSurface::BuildModuleGrid/BuildModDetailGrid` at Sheaf `Bank`/`Parameter` reads (from `visibleRowForSlot`/`effectiveRow`), and **collapse the dual-control-core split** (surface `m_audioCore` vs facade) onto ONE shared `ParameterManager` — fixes the "surface edits never reach DSP" bug (D16 flag 1).
- [ ] 13.6 Re-express the 2-deep mod-drill-in gate (D4/packet 6) against `Bank`'s modulation-view API; re-express the packet-10 mod-view-reset-on-tab-switch against `Bank::Deselect`.
- [ ] 13.7 Wire `SceneState` end-to-end (scene select + blend). **Scenes only** — sequencer-step capture/recall and gesture-weight rand are DELETED in §13.0.1, NOT re-attached (DECISION 3 resolved: delete, not migrate).
- [ ] 13.8 Add the small "randomize all banks" glue (global rand) Sheaf lacks; wire `StandardModulators<N>` for Random S&H sources — feeds the ganged visualizers for free (OPEN DECISION 5; corrects the false "already fed" assumption).
- [ ] 13.9 Re-derive `HostParameterInventoryV2`'s VST-flat descriptor table from the `ParameterGroup`/`Bank` graph; re-baseline `kCount` + projection validators.
- [ ] 13.10 Retire `FroggersV2ControlCore` once grep-clean; full desktop-v2 suite + validators green (closing gate).

**Reworked/superseded by §13 (delta from previous plan):** packet 6 (mod gate) + portable-surface increments 1-4 (`FroggersAppSurface` reads) + packet-10 step-1 = REWORKED; the bespoke param/inventory/rand/Crunchy/Crispy layer = SUPERSEDED; sequencer + gesture-weight = DELETED (§13.0.1). Packets 2,3,4,5,7.4,7.5,12 + the packet-10 boot mechanism = SURVIVE (DSP/host-boundary, below the bridge). DECISIONS 1-7 (design D16) are RESOLVED (2026-07-25) — see §13.2/§13.7 and D16.

## Packet order / parallelization

Sections **1→11** are sequential for code changes. Do not parallelize implementer subagents across sections unless the lead publishes a dependency map and the user accepts it (OMNI §4). Explore-only subagents may run in parallel.

## Out of scope (do not implement in this change)

- MIDI Controllers mapping / profile automation / per-row encoder MIDI targets
- VST/AU PluginRuntime host
- Web chrome parity
- Full ADSR decay knee
