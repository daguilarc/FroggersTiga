## 1. Scaffold and engine taps

- [x] 1.0 Create feature branch `froggerstiga-desktop-v2` from `main`; all v2 work stays off `main` until §10 QA sign-off
- [x] 1.1 Add `desktop-v2/` CMake target `FroggersTigaDesktopV2` sharing `src/core`, `sim`, JUCE 8.0.4 FetchContent
- [x] 1.2 Add `sim/V2ModSourceCatalog.hpp` with indices 7–14 and `SimHostKind::DesktopV2` / `VstV2`
- [x] 1.3 Add `sim/V2ParamDisplayNames.hpp` (10-row labels for pages 1–5 and ADSR; Audio unchanged)
- [x] 1.4 Implement `V2EnvelopeFollowerBank` for six EF taps at audio rate
- [x] 1.5 Remap Marbles to indices 13–14 on v2 hosts only
- [x] 1.6 Add `sim/V2ModSource_test.cpp`

## 2. ADSR engine and fuegoization

- [x] 2.1 Implement `src/core/VcoAdsrState.hpp` (gated ADSR per VCO: attack → sustain hold → release on gate off; 9 A/S/R params + page Crispy path)
- [x] 2.2 Wire `VcoAdsrState` into `DesktopHostIO` for v2 hosts; disable `AudioPairArState` on v2
- [x] 2.3 Implement global `Crunchy` state (fuego pass on **all** rows all pages incl. every Crispy instance)
- [x] 2.4 Retain per-page Crispy on every module; implement `CrispyRowForPage(hostPage)` authority table (Audio row 7; expanded modules 1–5 and ADSR row 9) per `design.md` §9
- [x] 2.6 Implement v2 module expansion rows (pages 1–5) and parallel Filter comb/peak + Scoop per `desktop-v2-module-expansion`
- [x] 2.7 Add `sim/V2ModuleExpansion_test.cpp` and `sim/V2FilterParallel_test.cpp`
- [x] 2.8 Wire module expansion + global Crunchy into `SimHostKind::Web` engine path (shared DSP with DesktopV2)

## 3. Control core bridge

- [x] 3.1 Create `FroggersV2ControlCore` (MessageIn, MessageInBus, UIState with scene L/R voices)
- [x] 3.2 Implement three scene ordinals, blend, **two gesture lanes** (G1/G2), shift-held semantics
- [x] 3.3 Implement interaction matrix (design.md §6) including Crunchy/ADSR Crispy exceptions
- [x] 3.4 Implement bank/slot paging for modules with more rows than physical encoders
- [x] 3.5 Implement modulation-depth drill-down and dropdown assignment coordination
- [x] 3.6 Implement `FroggersV2HostBridge` with unified Smart Grid math (depths + range normalization; arcs match audio)
- [x] 3.7 Rand All scope: all module pages + ADSR rows 0–8 + global Crunchy; skip scene storage; clear gesture first
- [x] 3.8 Add `desktop-v2/tests/ControlCoreBridge_test.cpp`

## 4. Desktop v2 UI

- [x] 4.1 Implement `PageCarouselComponent` (7 modules, **Module:** label)
- [x] 4.2 Implement `SubmodulePagePanel` (10 rows for module pages 1–5; Audio layout unchanged)
- [x] 4.3 Implement `AdsrPagePanel` (10 rows)
- [x] 4.4 Implement `EncoderRingComponent` (scene L/R rings, dot, badges, min/max arcs)
- [x] 4.5 Implement `ModSourceCell` (lit state, dropdown)
- [x] 4.6 Implement `ScopeGridComponent` (6 traces + 2 Marbles LEDs)
- [x] 4.7 Implement global strip: Rand All/Mods/waveforms/Resample + **Crunchy** + Shift/Gesture 1/Gesture 2/LFO/VCO + Scene S1–S3 + blend slider + sequencer transport (BPM, length, play/record)
- [x] 4.8 Implement `SequencerPanelComponent` (step grid, playhead, record arm, pattern length 4–64)
- [x] 4.9 Wire keyboard Shift and MIDI-assignable Shift
- [x] 4.10 Layout in `DesktopV2ChromeLayout.hpp`; hide v1 patch cables and six-column chrome
- [x] 4.11 Stereo-default audio init (document equivalence to v1 `initialiseWithDefaultDevices(0, 2)`)

## 5. Desktop v2 MIDI CV input

- [x] 5.1 Implement `MidiCvAssignmentTable` and settings UI (one input device)
- [x] 5.2 Support pitch, gate, CC→external-mod, shift, scene button bindings
- [x] 5.3 Optional QWERTY virtual MIDI channel
- [x] 5.4 Add `desktop-v2/tests/MidiCvAssignment_test.cpp`

## 6. VST v2 plugin

- [x] 6.1 Add `BUILD_VST_V2` and `FroggersTigaPluginV2` target
- [x] 6.2 Generate `HostParameterInventoryV2.hpp` (dual `stableId` + `displayName`; ADSR; global Crunchy; sequencer; no pair-AR)
- [x] 6.3 Implement registry, routing, pending store v2
- [x] 6.4 `acceptsMidi() true`; mono in / stereo out default; `isBusesLayoutSupported` mono out
- [x] 6.5 Embed `PluginEditorV2` with v2 chrome minus standalone-only clusters
- [x] 6.6 Add `HostParameterProcessorV2_test.cpp` with exact `kCount`

## 7. Sequencer engine and control core

- [x] 7.1 Implement `SequencerState` (BPM, pattern length, playhead, per-step scene/gesture snapshots, step gate)
- [x] 7.2 Wire sequencer clock to control core (`MessageIn::Clock`); recall step snapshots on advance
- [x] 7.3 Wire sequencer gate to `VcoAdsrState` and MIDI gate merge
- [x] 7.4 Optional external MIDI clock sync in `MidiCvAssignmentTable`
- [x] 7.5 Add `sim/SequencerState_test.cpp`

## 8. Web parameter subset

- [x] 8.1 Extend WASM bindings and `WasmSimHost` for module expansion rows and global Crunchy state
- [x] 8.2 Regenerate `web/src/hostDisplay.generated.ts` with ten-row labels for pages 1–5
- [x] 8.3 Add **Crunchy** rotary to web global strip; wire Crunchy/Crispy fuego stack
- [x] 8.4 Update `web/public/sim-manual.md` and quick-dict for new rows + Crunchy
- [x] 8.5 Manual: web pages 1–5 show 10 knobs; Filter Comb/Peak + Scoop audible; Crunchy affects all pages; pair-AR still on Audio
- [x] 8.6 Extend `web/test-shared/simSelectors.ts` with v2 row labels (pages 1–5 rows 7–9) and Crunchy global-strip selector
- [x] 8.7 Add Playwright e2e specs: ten-knob expanded pages 1–5, Crunchy visible in global strip, Filter Comb/Peak + Scoop labels; existing v1 e2e specs remain green

## 9. Spec and doc parity

- [x] 9.1 Update `SIM_MANUAL.md` with v2 section (Module vs Scene, Crunchy, ADSR, encoder rings, stereo default, sequencer, 2 gestures)
- [x] 9.2 Add `desktop-v2/PACKAGING.md` (local-only until release)
- [x] 9.3 Extend `verify_clean_rebuild.sh` optional path for v2 targets
- [x] 9.4 Update web manual/quick-dict (see §8.4)

## 10. Verification

- [ ] 10.1 `make -C sim test` passes including `VcoAdsrState_test`, `V2ModSource_test`, and `SequencerState_test`
- [ ] 10.2 Build `FroggersTigaV2.app`
- [ ] 10.3 Build VST3/AU v2 artefacts
- [ ] 10.4 Manual desktop: module carousel, ADSR gated envelopes, Crunchy global, scene rings, 2 gesture lanes, sequencer record/playback
- [ ] 10.5 DAW: MIDI map to `Global/Crunchy`, ADSR param, and `Sequencer/BPM`
- [ ] 10.6 Manual web: expanded pages + Crunchy (§8.5)
- [ ] 10.7 `cd web && npm run test:e2e` passes including new v2 Playwright specs (§8.7)
- [ ] 10.8 Confirm v1 default build on branch (`BUILD_DESKTOP_V2=OFF`, `BUILD_VST_V2=OFF`) before merge to `main`
