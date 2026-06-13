# Desktop sim UX polish — tasks

## 1. Sample rate (44100)

- [x] 1.1 In `AudioEngine` ctor after `initialiseWithDefaultDevices`, set `AudioDeviceSetup.sampleRate = 44100.0` via `setAudioDeviceSetup`
- [x] 1.2 Call `m_host.SetSampleRate(44100.f)` in ctor after `Init()` and in `audioDeviceAboutToStart` (ignore device-reported rate for DSP)
- [x] 1.3 Call `m_delay.setSampleRate(44100.f)` in same places
- [x] 1.4 Log warning if opened device rate ≠ 44100 after setup

## 2. Mod rack CV scopes

- [x] 2.1 `CvScopeDisplay` trace mode per mod type; scope fills box width (~44px tall)
- [x] 2.2 `ModRackPanel`: equal-width boxes, ≥16px gaps
- [ ] 2.3 Manual: scopes read as wide oscilloscope traces, not vertical sliders

## 3. Ring mod meter + mod rack clarity

**SUPERSEDED by `desktop-host-corrections`** — remove ring-mod toggle; gate-only routing; optional passive In meter.

- [x] 2.1 ~~ring mod toggle meter~~ (revert in host-corrections)
- [x] 2.2 ~~dim meter when Off~~ (revert in host-corrections)
- [x] 2.3 Add "Mod out" subtitle on each `ModModuleBox`; confirm jack is visually below meter

## 4. Patch cable fixes

- [x] 3.1 Increase `PatchCableOverlay::kPortHitRadius` to 22.f
- [x] 3.2 Call `syncPatchPorts()` from `MainComponent::timerCallback()` (keep call in `resized()` too)
- [x] 3.3 On successful connect/disconnect in overlay, trigger affected panel `refresh()`
- [x] 3.4 Verify `m_cableOverlay.toFront(false)` after layout; manual test all four mod sources × sample knobs on pages 0 and 5
- [x] 3.5 Per-knob mod-in jacks beside each knob (48 inputs across six panels); overlay draws all sockets + thick random-hue cables

## 5. Effective knob value API

- [x] 4.1 Add read-only accessor on `DesktopHostIO` / `PageManager` for post-mod effective knob value per (page, row) — source: last block's applied parameter or stored page knob after mod mix
- [x] 4.2 Wire `DelayHostBackend` equivalent for delay page rows if mod routing applies

## 6. Rotary knobs + layout

- [x] 5.1 Replace `LinearVertical` with `RotaryHorizontalVerticalDrag` in `SubModulePanel` (7 rows + FUEG); 38–40 px diameter, no text box
- [x] 5.2 Re-layout rows: label column 64 px, wave button 18 px (audio rows 0–2), knob 38 px, mod-in jack 20 px right of knob
- [x] 5.3 Reduce row height divisor or fixed row height (~36 px) so panel fits 720 px window
- [x] 5.4 Update `refresh()`: unpatched → `getKnob`; patched + dragging → mod depth; patched + idle → `GetEffectiveKnob`
- [x] 5.5 Apply same knob component to Delay panel via shared layout in `SubModulePanel`

## 7. Manual verification

- [ ] 6.1 Rebuild desktop: `cd desktop && cmake --build build && open build/FroggersTigaDesktop_artefacts/FroggersTiga.app`
- [ ] 6.2 Play → confirm engine at 44100 (log or delay max time sanity)
- [ ] 6.3 Line in + Play → passive **In** envelope indicator moves (no ring-mod toggle; gate-only per `desktop-host-corrections`); mod rack meters idle unless MIDI/Marbles active
- [ ] 6.4 Patch VCO level → Audio VCO1: cable visible, knob tracks modulation while playing
- [ ] 6.5 VCO1/VCO2/VCO3 labels fully visible at default window size (1680×720 per desktop-compact-layout)
- [ ] 6.6 Delay panel patch + knob row behaves same as core panels
- [ ] 6.7 Each knob row shows mod-in socket; drag mod-out → mod-in connects independently per parameter

## 8. Sign-off

- [x] 7.1 OMNI rule review on touched files (nesting ≤4, no repetition, accumulate-then-apply)
- [x] 7.2 No firmware or WASM page-count changes
