## 0. Merge + postmortem

- [ ] 0.1 Archive `vcv-panel-silkscreen-fix` after this change absorbs its scope (do not duplicate apply)
- [ ] 0.2 Record predecessor false-completion list in design.md (done in proposal/design)

## 1. Silkscreen + layout geometry (blocking — regress 1.4–1.5 until fixed)

- [x] 1.1 Path-based SVG via `fontTools`; parse `ParamDisplayNames.hpp` + `VcvPanelLayout.hpp`
- [x] 1.2 Header strip art: frog logo + “FroggersTiga” paths on main + FX
- [x] 1.3 `sim/check_vcv_panel_svg.sh`; stale Voicing SVGs removed
- [ ] 1.4 **D15:** Fix header geometry — recompute `rowStepY` from usable band; bottom I/O anchored from panel bottom (remove header term from `primaryIoY`)
- [ ] 1.5 **D16:** Move label offsets to `VcvPanelLayout.hpp`; delete magic `11.0` mm / `±0.55` grid literals from `generate_panels.py`
- [ ] 1.6 **D17:** Extend `check_vcv_panel_bounds.sh` — widget bbox + bottom I/O clearance (not center-only)
- [ ] 1.7 Regenerate SVGs; rebuild v2.5.x `.vcvplugin`
- [ ] 1.8 `./build.sh --install` on target machine
- [ ] 1.9 Manual Rack 100% zoom: labels **aligned** to knobs/ports (not on jacks); bottom row **not clipped**; primary band not empty junk text; PM3/Crispy correct

## 2. Layout constants for new controls

- [ ] 2.1 Add `kGlobalStripGridY`, `kColumnActionGridY`, `kModRackCellCount`, `kWaveMorphGridOffsetX` to `VcvPanelLayout.hpp`
- [ ] 2.2 Extend `check_vcv_panel_bounds.sh` for global strip, column actions, 5-cell mod rack, wave morph tails
- [ ] 2.3 Mockup fit at 72 HP; bump to 84 HP in `kMainHp` + `plugin.json` if mod rack + buttons overflow

## 3. Randomize wiring (`plugin.cpp`)

- [ ] 3.1 Remove miswire: mod-rack `RANDOM_PARAM` shall **not** call `RandomizeAllPages()`
- [ ] 3.2 Global strip: Rand All → `RandomizeAllPages()`; Rand Mods → `RandomizeAllMod()`; Rand Resample → `ButtonCallback(0)`; Rand waveforms → `RandomizeVcoMorphs()` (labels from `ParamDisplayNames::forGlobalStrip`)
- [ ] 3.3 Per voicing column: Randomize + Randmod → `RandomizePage` / `RandomizePageMod` for pages 0, 1, 3, 4
- [ ] 3.4 FX Reverb column: Randomize + Randmod → page 2 APIs
- [ ] 3.5 FX Delay column: Randomize + Randmod → `DelayState` randomize APIs with `midiBridge` gating
- [ ] 3.6 Consolidated rising-edge momentary handler (one helper, all randomize params)

## 4. Randomize widgets + silkscreen

- [ ] 4.1 `FieldParityWidget`: helpers for global strip + column action row
- [ ] 4.2 Main module: global strip + four column action pairs
- [ ] 4.3 FX module: Reverb + Delay action pairs
- [ ] 4.4 Regenerate SVGs: path labels for all new buttons
- [ ] 4.5 Remove duplicate mod-rack Random toggle; **Random** lives in global strip only

## 5. VCO morph widgets (Audio rows 0–2)

- [ ] 5.1 Add `WaveMorphWidget.hpp`: nanovg wave stroke from `VcoWaveEval` / shared morph eval
- [ ] 5.2 `FieldParityWidget`: place wave widget left of knob on Audio page rows 0–2 only
- [ ] 5.3 Wire click → `host.CycleVcoMorph(index)`; refresh morph from `host.GetVcoMorph(index)` each frame
- [ ] 5.4 Manual: click VCO1 morph cycles sine → saw → square audibly matching desktop
- [ ] 5.5 Manual: **Rand waveforms** updates all three wave widgets

## 6. Mod rack five-cell parity (LED-only — no scopes)

- [ ] 6.1 Add `MOD_CC1_OUTPUT`, `MOD_CC2_OUTPUT` params; wire `process()` to `GetCvOut(0)` / `GetCvOut(1)`
- [ ] 6.2 Replace 3-cell mod row with 5 cells: CC1 LED | CC2 LED | VCO Env LED | Rand1 LED | Rand2 LED (threshold 0.55)
- [ ] 6.3 Extend `updateModLights()` for mods 0, 1, 4 (same pattern as existing 4, 5, 6)
- [ ] 6.4 Update `generate_panels.py` mod rack labels to 5 cells (`ParamDisplayNames::forModSource`)
- [ ] 6.5 Remove `CV_OUT1`, `CV_OUT2`, MIDI Out button, and `tickMidiOut()`; shrink bottom I/O row in layout + SVG
- [ ] 6.6 Manual: CC1 LED responds to MIDI/CV input; Rand1 LED steps on **Random** press
- [ ] 6.7 Confirm no `ModCvScopeWidget` or CV trace code added to VCV tree

## 7. Option C stereo FX routing (expander linked)

- [ ] 7.1 Remove `setStereoOutputs(sameVoltage)` duplicate on FX L/R
- [ ] 7.2 After `ProcessBlock`, build `StereoFxSpread` via `makeStereoFxSpread(delay, engine.getReverbStereoDeltaL/R(), engine.getLastRvMix())`
- [ ] 7.3 Main `AUDIO_OUTPUT` = `coreMono`; FX L/R = `coreMono + dmix·delayΔ + rvMix·revΔ` (same formula as `applyStereoBus`)
- [ ] 7.4 No expander: main out = full mix only (unchanged)
- [ ] 7.5 Manual: delay width > 0 → FX L ≠ FX R; matches desktop stereo field
- [ ] 7.6 Update `vcv/DEVELOPMENT.md`: Option C patch diagram (FX L/R primary; do not sum main + FX to same bus)

## 8. Build + verification

- [ ] 8.1 `arch -x86_64 make dist && ./build.sh --install`
- [ ] 8.2 `sim/check_vcv_panel_svg.sh` + `check_vcv_panel_bounds.sh` pass
- [ ] 8.3 Manual randomize matrix: 8+ actions spot-check vs desktop
- [ ] 8.4 Manual: patch Random 1 → Audio VCO1; compare timbre to desktop
- [ ] 8.5 Manual: patch VCO Envelope → Audio row; timbre match desktop
- [ ] 8.6 Manual: PM3 knob CW matches desktop (Audio row 7)
- [ ] 8.7 Manual Rack 100% screenshots: silkscreen + randomize + wave morph + five mod LEDs
- [ ] 8.8 Bump `plugin.json` to 2.6.0

## 9. VST reference audit (delegated — see `vst-plugin-host-ux`)

- [x] 9.1 Audit VST architecture (design D10)
- [x] 9.2 Confirm VST passes field-parity visuals vs VCV gaps
- [x] 9.3 Spun off: manual DAW smoke test → `vst-plugin-host-ux` task 6.1
- [x] 9.4 Spun off: VST vs desktop A/B → `vst-plugin-host-ux` task 6.2
- [x] 9.5 Spun off: editor min size → `vst-plugin-host-ux` task 4.x
- [x] 9.6 Spun off: plugin-host UX change → `openspec/changes/vst-plugin-host-ux/`
- [x] 9.7 Sync audit tables both ways (2026-06-14): VCV proposal VST table ↔ `vst-plugin-host-ux` proposal OMNI audit

## 10. Docs

- [ ] 10.1 `vcv/DEVELOPMENT.md`: randomize map, VCO morph, mod rack, Option C stereo patch, I/O simplification
- [ ] 10.2 VCO Envelope mod vs Phase mod 3 distinction
- [ ] 10.3 Cross-reference archived field-parity manual gates 4.6, 5.1, 5.5

## 11. Structural refactor (audit 2026-06-14 — do with sections 3–6, not after)

- [ ] 11.1 Add `kModRackCells[]` table (mods 0,1,4,5,6); widget + `process()` + `updateModLights()` iterate it — no triplicate `GetCvOut` lines
- [ ] 11.2 Add `RandomizeAction[]` + single rising-edge dispatcher; remove mod-rack-row TL1105 when global strip owns **Random**
- [ ] 11.3 Extract shared column sync loop from `syncVoicingColumn` / `syncColumn` (knob + mod jack; delay branch inside loop)
- [ ] 11.4 Remove dead `primaryPanelSize()` from `FieldParityWidget.hpp`
- [ ] 11.5 Remove duplicate `CV_OUT1/2` voltage writes in `process()` when jacks removed (section 6.5)
- [ ] 11.6 `generate_panels.py`: read new anchors from `VcvPanelLayout.hpp` only — delete mirrored coordinate math at lines ~130–154
- [ ] 11.7 Regenerate mod-rack SVG labels: five cells from `ParamDisplayNames::forModSource`; no "Random" in mod row
- [ ] 11.8 `check_vcv_panel_bounds.sh`: assert global strip, column actions, 5-cell mod rack, wave-morph tails fit HP
