> **Reconciled (omni 1.2):** Code-backed; tasks 6.2–6.4 remain open as explicit manual smokes.

## 1. Shared param authority

- [x] 1.1 Add `sim/AudioPairArLayout.hpp`: `kAudioPairArCellCount = 4`, desktop band height/column spacing constants
- [x] 1.2 Add `ParamDisplayNames::forAudioPairAr(index)` — Attack 1+2, Release 1+2, Attack 2+3, Release 2+3
- [x] 1.3 Document pair-AR vs reverb “Decay” in `SIM_MANUAL.md` + synced copies

## 2. Engine + host IO

- [x] 2.1 Add `PairArEnvelope` helper in `src/core/PairArEnvelope.hpp` (attack/release step from target level)
- [x] 2.2 Add `sim/PairArEnvelope_test.cpp` — rise on attack min, slow fall on release max
- [x] 2.3 Wire two instances in `FroggersEngine` — envelope `(v1+v2)` and `(v2+v3)` into osc mix
- [x] 2.4 `AudioPairArState` on `DesktopHostIO` / `PagedHostIO`: knob, mod source, mod depth, effective value
- [x] 2.5 Extend `SimPresetSnapshot` with 4× knob + mod fields; bump version; v1 defaults

## 3. WASM bindings

- [x] 3.1 `wasm/bindings.cpp`: set/get pair-AR knob, mod source, mod depth
- [x] 3.2 Extend worklet screen message with pair-AR row data for page 0
- [x] 3.3 Rebuild `web/public/froggers.wasm` + processor bundle

## 4. Desktop UI

- [x] 4.1 `SubModulePanel`: four pair-AR sliders + labels + jack bounds (Audio page only)
- [x] 4.2 `layoutPairArBand()` — single loop over `kAudioPairArCellCount`, jack → knob → label stack
- [x] 4.3 Extend `collectInputPorts` + `PatchCableOverlay` for pair-AR mod indices
- [x] 4.4 Increase Audio panel min height; verify layout at default window size

## 5. Web UI

- [x] 5.1 When `hostPage === 0`, render 12 knob columns (append 4 pair-AR cols)
- [x] 5.2 Wire pair-AR knobs/mod selects to new worklet messages
- [x] 5.3 `layoutKnobCols` / page change: hide pair-AR cols on non-Audio pages
- [x] 5.4 CSS: third row flows in existing 4-column grid (no layout regression on other pages)

## 6. Verification

- [x] 6.1 `PairArEnvelope_test` passes in CI
- [ ] 6.2 Manual desktop: patch mod to Attack 1+2; hear pair-12 swell
- [ ] 6.3 Manual web: Audio page shows third row with four labels; Release 2+3 affects timbre
- [ ] 6.4 Manual: snapshot save/reload restores pair-AR knobs + mod routing

## 7. Global strip: Rand Resample rename

- [x] 7.1 `ParamDisplayNames::forGlobalStrip(GlobalStripAction)` — Rand All, Rand Mods, **Rand Resample**, Rand waveforms
- [x] 7.2 `GlobalStrip.cpp/.h` — button text from table; fix Rand waves → Rand waveforms
- [x] 7.3 Web `index.html` + hint line — **Rand Resample**; optional `main.ts` constant from shared doc sync
- [x] 7.4 Manual + Quick Dict (synced copies): **Rand Resample** — “Resample both random S&H channels (draws from bags)”; cross-refs in Random S&H section use **Rand Resample** not bare **Random**
- [x] 7.5 Cross-note for `vcv-rack-field-parity`: VCV global-strip silkscreen uses same `forGlobalStrip` string (see that change proposal Global strip section)
