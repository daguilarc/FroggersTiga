> **Reconciled (omni 1.2):** Code-backed; task 4.2 optional Playwright remains open.

## 1. Core state — single blend path

- [x] 1.1 Refactor `AudioPairArState::blendKnob` to accept `const float* mods` argument; keep `beginBlock` setting `m_mods` for audio-thread `tickSmoothers`
- [x] 1.2 Add `getEffectiveKnob(index, const float* mods)` overload; delegate existing `getEffectiveKnob(index)` to `blendKnob(..., m_mods)`
- [x] 1.3 Update `DesktopHostIO::GetAudioPairArEffective` and `PagedHostIO::GetAudioPairArEffective` to pass `m_pageManager.m_modMgr.m_mods` at read time

## 2. Desktop audio wiring

- [x] 2.1 In `AudioEngine.cpp` process callback, call `m_host.m_pairAr.beginBlock(m_host.m_pageManager.m_modMgr.m_mods)` immediately after `m_delay.beginBlock` (same mod array pointer)
- [x] 2.2 Manual verify: Audio page, assign moving mod to Attack 1+2, Play — knob rotates while not dragging (compare to row 1 knob)

## 3. Web verification

- [x] 3.1 Rebuild WASM if `AudioPairArState` API changed; confirm `froggers_get_audio_pair_ar_effective` still exported
- [x] 3.2 Manual verify: web Audio page third row — mod assigned + Play — knob position animates (note finding if already OK pre-fix)

## 4. Tests

- [x] 4.1 Add unit test in `sim/PairArEnvelope_test.cpp` (or adjacent test file): mod depth 1.0, base 0.5, sweep `mods[0]` 0→1, assert effective follows blend
- [ ] 4.2 Optional: Playwright spec sampling two consecutive `screen` messages with pair-AR mod + audio running; assert `pairArRows[0].value` changes

## 5. Docs / OpenSpec

- [x] 5.1 Mark tasks complete in this file after verification
- [x] 5.2 Archive or link from `audio-pair-ad-controls` tasks if any implied “mod display parity” item exists
