## 1. Core engine and PageManager

- [x] 1.1 Add `m_sampleRate` and `SetSampleRate(float)` to `FroggersEngine`; replace all `48000` literals in `FroggersEngine.hpp`, `Marbles.hpp`, `RuntimeParam.hpp`, `EQ.hpp`, and reverb pre-delay scaling (zero bare `48000` left in those files); recompute RuntimeParam alphas on `SetSampleRate`
- [x] 1.2 Add `KnobUpdateOnPage(uint8_t page, uint8_t position, float value)` to `PageManager`
- [x] 1.2b Add `RandomizePage(uint8_t page)` and `RandomizeAllPagesIndependent()` to `PageManager` — read each page's `m_knobValue`, not shared `m_knobPositions`; desktop B1/B2 use these; paged hosts keep `RandomizeCurrentPage` / `RandomizeAllPages`
- [x] 1.3 Wire firmware shim to call `SetSampleRate(48000.f)` on init; verify `make` in `src/FroggersTiga`
- [x] 1.4 Add `SetSimWaveMorph(bool)` (default false), sim-only `VcoWaveMorph[3]`, `EvalWaveMorph()`, exp knob mapping, `RandomizeVcoMorphs()`; sim `StepOscillators` uses morph path when flag true; firmware keeps discrete A8/B8 + sine VCO3; do not modify `DaisyIO.hpp`
- [x] 1.5 Verify `grep -r daisy src/core` is empty; firmware build still passes with VCO3 = sine on device
- [x] 1.6 Register morph targets with `ModMgr` in sim host `Init()` only (`SetSimWaveMorph(true)` there); wire CV attenuation like `Parameter` (per-sample morph read via `VcoWaveMorph::GetMorph` + `SetVcoMorphMod`)
- [x] 1.7 Add `GetEnvelopeLevel()` to `FroggersEngine` (last sample in block); implement `CvMidiBridge` (`drainMidiIn` in `tickControls`, `tickMidiOut` after block reads `GetEnvelopeLevel()`)

## 2. Host adapter split

- [x] 2.1 Rename/refactor `HostIO.hpp` → `PagedHostIO.hpp` (SW1/SW2, shared knobs, OLED queries, CV/gate)
- [x] 2.2 Implement `DesktopHostIO.hpp` with `Init()` (force all params Tracking), `SetPageKnob`, per-panel Randomize → `RandomizePage`, shared strip Randomize all → `RandomizeAllPagesIndependent`, Randomize mod (all) → `RandomizeAllPagesModIndependent`, per-panel Randomize mod → `RandomizePageMod`
- [x] 2.3 Extract `applyCvPresence` into `src/core/CvPresence.hpp`; use from PagedHostIO and DesktopHostIO
- [x] 2.4 Update `wasm/bindings.cpp` to use `PagedHostIO`

## 3. Web simulator (paged UI)

- [x] 3.1 Fix web audio to use `audioContext.sampleRate` and engine `SetSampleRate` (default 44100)
- [x] 3.2 Complete Vite UI: 8 knobs, SW1/SW2, OLED mock, **Mic** toggle (default off); Audio rows 0–2 get inline wave icon + morph control; optional A′/B′ strip; no extra wave row
- [x] 3.2b Wire **Mic** off → zero external input in worklet; **Mic** on → `getUserMedia` → external bus (no permission prompt until user enables mic)
- [x] 3.3 Finish AudioWorklet + standalone WASM load in worklet
- [x] 3.4 Add `.github/workflows/pages.yml` (emsdk, cmake wasm, npm build, commit `docs/`)
- [x] 3.5 Add `web/public/` wasm copy step and `.gitignore` rules for local `dist/`

## 4. Desktop simulator (multi-panel UI)

- [x] 4.1 Scaffold `desktop/` JUCE CMake project linking `froggers_core` + `DesktopHostIO`
- [x] 4.2 Implement `SubModulePanel` scaffold (Randomize + 7 labeled knobs + FUEG; wave button on Audio VCO rows) — **superseded by §8** (remove mod ComboBox, add Randomize mod + input jacks, no mini-OLED)
- [x] 4.3 Layout panels horizontally; shared strip with human labels (Randomize all, Randomize mod, Marbles, Randomize waves) — no XCPL strip buttons (knob-only); mod rack + per-panel Randomize mod in §8
- [x] 4.4 Audio callback: `tickControls()` (MIDI in drain + CV presence + gate) → `ProcessBlock` → `tickMidiOut()`; external-in bus → `ProcessSample`
- [x] 4.5 Audio settings UI: output device + external (ring-mod) input device/channel selector
- [x] 4.6 MIDI settings UI: in/out device picker; default CC/channel for envelope out
- [x] 4.7 Omit SW1/SW2 and `[` `]` page keys from desktop v1

## 5. Verification and docs

- [ ] 5.1 Manual A/B: same param vector at 44.1 kHz web vs desktop within float tolerance
- [x] 5.2 Update `.cursor/plans/desktop_sim_vcv_rack.plan.md` or mark superseded by this change
- [x] 5.3 Document GitHub Pages publish steps in README

## 7. v2 UX (transport, mod bay, labels)

- [x] 7.1 Core: `SetPageModSource` / `SetPageModDepth` / getters on `PageManager`; host + WASM exports
- [x] 7.2 Desktop: Play/Stop transport; external Off/L/R; no auto-start audio
- [x] 7.3 Desktop: ModBayPanel (M1–M7 meters); per-knob mod combo; MIDI settings dialog — **superseded by §8** (dropdown approach rejected)
- [x] 7.4 Desktop: human labels; strip dedupe (no A8/B8/A'); VCO wave on row; remove mini-OLED rows
- [x] 7.5 Desktop: refresh skips dragging sliders; ASCII button labels (Audio..., not mangled Unicode)
- [x] 7.6 Web: Play/Stop; External audio toggle; mod bay + per-knob mod dropdowns; global strip
- [ ] 7.7 Rebuild WASM and verify web + desktop manually

## 8. v2.1 UX (sim mod sources, patch bay, layout)

- [x] 8.1 Add `SimModSource` enum + mapping in `DesktopHostIO` / `PagedHostIO`; host `SetPageModSource` rejects indices 1–3; `CvMidiBridge` sim profile: one `m_inChannel` + `m_inCc` → `m_mods[0]` only (remove `m_inCvCc[4]` loop)
- [x] 8.2 Desktop: delete `ModBayPanel`; add four `ModModuleBox` components (MIDI, VCO feat, Marbles 1/2) with live meters + output port registration
- [x] 8.3 Desktop: `PatchCableOverlay` — top-level z-order above panels; port registry from module boxes + `SubModulePanel` rows (incl. FUEG); VCV drag-from-out (≥4px threshold) / drop-on-in / drop-void-cancel; grab plug to move or disconnect; input hover highlight; replace-on-repatch; one out → many in; repaint on resize and after Randomize mod
- [x] 8.4 Desktop: remove per-knob mod `ComboBox`; row layout `[label][wave?][●in jack][slider]` (label truncation fix — no panel widen)
- [x] 8.5 Desktop: per-panel **Randomize mod** button → `RandomizePageMod(page)`; `MidiSettingsComponent`: one in channel + in CC (remove M1–M4 grid)
- [x] 8.6 Web: mod dropdown **below** each knob slider; sources `None | VCO feat | Marbles 1 | Marbles 2`; internal meter strip (three sources, no M1–M4)
- [x] 8.7 WASM: rebuild `froggers.wasm` locally or via CI (`wasm/CMakeLists.txt` exports fixed; `npm run build:wasm`)
- [x] 8.8 Fix FUEG `nan` on Filter panel refresh (`DesktopHostIO::GetPageParam` finite guard)
- [x] 8.9 Desktop: `AudioEngine` reuses member `inBlock` buffer across callbacks (no per-block `std::vector` allocation)

## 6. VCV module (phase 2 — after license decision)

- [x] 6.1 Record GPL + MIT core distribution decision (`vcv/LICENSE`, `vcv/LICENSE_BOUNDARY.md`)
- [x] 6.2 Scaffold Rack 2 plugin with `PagedHostIO` and page-switch params (`vcv/src/plugin.cpp`, `vcv/Makefile`, `vcv/build.sh`)
- [x] 6.3 Map Field-parity jacks (audio in/out, CV1–4, gate, CV out M5/M6) plus MIDI in/out, `SetSampleRate`, VCO morph params, and `CvMidiBridge` tick order (`vcv/src/plugin.cpp` — VCO morph UI knobs deferred)
