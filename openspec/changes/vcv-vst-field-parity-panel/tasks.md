## 0. Prerequisite

- [x] 0.1 Apply `sim-pm3-knob-parity` (docs + label authority) before VCV Audio column (Phase B task 3.4)

## 1. Shared panel model + host IO parity (Phase A)

- [x] 1.1 Audit `DesktopChromeLayout.hpp`, `PanelBackend.hpp`, `ModRackPanel` for layout literals to extract
- [x] 1.2 Add `sim/HostPanelLayout.hpp` with page count, row count, mod indices, desktop pixel constants; reference `ParamDisplayNames`
- [x] 1.3 Refactor desktop panels to consume `HostPanelLayout` (visual parity check at 1440×720)
- [x] 1.4 Extend `PagedHostIO` with page-indexed APIs: `SetPageKnob`, `GetPageParam`, `SetPageModSource`, `SetPageModDepth`, `GetPageModSource`, `GetPageModDepth` (mirror `DesktopHostIO` semantics)
- [x] 1.5 Change `PagedHostIO::GetRowName` to return `ParamDisplayNames::forHostPageRow(currentPage, row)` on sim hosts (not firmware `OLVL` etc.)
- [x] 1.6 Wire `DelayState` + `SetSimFxInsert` on VCV shared engine instance across expander modules
- [x] 1.7 Replace VCV per-sample `ProcessBlock(n=1)` with rack block accumulation
- [x] 1.8 Document mod index → jack mapping in design.md appendix (verify table matches implementation)

## 2. JUCE VST plugin (Phase A)

- [x] 2.1 Add `juce_add_plugin` target in `desktop/CMakeLists.txt` (VST3 + AU; `BUILD_VST` option default ON)
- [x] 2.2 Wire `MainComponent` + `AudioEngine` as plugin editor/processor; enable resizable editor; full mod rack (scope + LEDs) matching standalone
- [x] 2.3 Implement transport/bypass policy (document in `SIM_MANUAL.md` plugin section)
- [x] 2.4 Verify standalone `FroggersTigaDesktop` still builds and runs
- [ ] 2.5 Manual: load VST3 in one DAW on macOS; confirm six panels + audio out

## 3. VCV widget scaffold (Phase B)

- [x] 3.1 Fix primary module `box.size` to 72 HP × `RACK_GRID_HEIGHT`; add Expander module(s) for row 2 (or rows 2–3 if 3+3 split)
- [x] 3.2 Remove page knob param; delete `PAGE_PARAM` sync loop from process
- [x] 3.3 Create `vcv/src/widgets/FieldParityWidget` with six column regions + mod rack strip
- [x] 3.4 Implement Audio column fully: 8 knobs, 8 mod input jacks, labels from `ParamDisplayNames` (row 7 = Phase mod 3); mod rack LEDs only (no scopes)
- [x] 3.5 Connect knobs and mod jacks via `PagedHostIO::SetPageKnob(0, row, …)` and `SetPageModSource(0, row, …)` — not current-page-only APIs

## 4. VCV remaining submodules (Phase C)

- [x] 4.1 Replicate column pattern for Random, Reverb, Filter, Drive (pages 1–4)
- [x] 4.2 Implement Delay column (page 5) via `DelayState` sidecar APIs, matching desktop `DelayHostBackend`
- [x] 4.3 Add mod rack output jacks (MIDI, VCO Envelope, Random 1/2) with correct voltages
- [x] 4.4 Reposition master audio/CV/MIDI/gate jacks on bottom or side per final layout
- [x] 4.5 Update `vcv/plugin.json` description and version; update local `vcv/README.md`
- [ ] 4.6 Manual Rack pass: patch Random 1 → Audio VCO1; verify same timbre change as desktop

## 5. Verification

- [ ] 5.1 Desktop vs VCV routing matrix: 12 spot-check patches (2 per submodule); include Delay column
- [ ] 5.2 VST vs desktop A/B at same preset: audio diff within expected float tolerance
- [x] 5.3 OMNI review: nesting ≤4 in new VCV layout code; no duplicate mod routing tables; single label authority
- [x] 5.4 License check: `vcv/` GPL only; plugin artifact contains no Rack SDK symbols
- [ ] 5.5 PM3 spot-check: VCV Audio row 7 + cross-coupler CW matches desktop PM3 behavior (after `sim-pm3-knob-parity`)
