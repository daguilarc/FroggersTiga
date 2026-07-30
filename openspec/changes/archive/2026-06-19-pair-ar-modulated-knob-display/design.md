## Context

```
Page-row knobs (desktop + web)
──────────────────────────────
Parameter::Get(ModMgr*)  ──►  always reads live m_mods[]
SubModulePanel.refresh() ──►  getEffectiveKnob() → GetPageParam → Get(modMgr)
Web froggers_row_value   ──►  GetRowValue → GetParam(modMgr)

Pair-AR knobs (added in audio-pair-ad-controls)
───────────────────────────────────────────────
AudioPairArState.blendKnob() ──►  needs float* m_mods from beginBlock()
Desktop UI refresh           ──►  getPairArEffectiveKnob → GetAudioPairArEffective
Web syncKnobUi               ──►  froggers_get_audio_pair_ar_effective when mod ≠ 255

Desktop audio callback (AudioEngine.cpp)
────────────────────────────────────────
tickControls()
delay.beginBlock(m_modMgr.m_mods)     ✓
m_pairAr.beginBlock(...)              ✗ MISSING
engine.ProcessBlock(...)
```

`DesktopHostIO::ProcessBlock` includes `m_pairAr.beginBlock`, but the desktop app bypasses it and calls `m_engine.ProcessBlock` directly from `AudioEngine`.

## Goals / Non-Goals

**Goals:**

- Pair-AR knob rotaries track live modulation on desktop and web, matching page-row knobs
- Single blend function in `AudioPairArState`; no duplicate display math in UI layers
- Unit test proving effective value moves when mod bus moves
- Minimal diff: wiring fix + read-path fix, not new UI components

**Non-Goals:**

- Showing smoothed envelope state on knobs (DSP uses `getEffectiveSmoothed`; display uses instantaneous blend like `Parameter::Get`)
- Refactoring all `beginBlock` state (`DelayState`, etc.) to permanent `ModMgr*` pointers
- VCV Rack panel (defer to `vcv-rack-field-parity`)

## Decisions

### D1 — Fix desktop audio callback (primary bug)

**Choice:** In `AudioEngine.cpp` process callback, after `m_delay.beginBlock(...)`, add:

```cpp
m_host.m_pairAr.beginBlock(m_host.m_pageManager.m_modMgr.m_mods);
```

**Why:** Mirrors existing delay wiring; restores mod pointer for `tickSmoothers()` in `FroggersEngine::MixOscVoices` and for UI timer reads between blocks (pointer remains valid to stable `ModMgr::m_mods` array).

**Alternative rejected:** Call `m_host.ProcessBlock` wrapper instead — would duplicate stereo/delay bus logic already inlined in `AudioEngine`.

### D2 — Live bus at read time in host IO

**Choice:** Change `GetAudioPairArEffective` in `DesktopHostIO` and `PagedHostIO` to blend using `m_pageManager.m_modMgr.m_mods` explicitly at read time (add `getEffectiveKnob(index, const float* mods)` or pass `ModMgr&` into `AudioPairArState`).

**Why:** OMNI parity with `Parameter::Get(modMgr)` — display works even when `beginBlock` has not run (audio stopped, first frame). Avoids relying on stale/null `m_mods` member for UI-only reads.

**Implementation sketch (OMNI — one blend path):**

```cpp
float getEffectiveKnob(uint8_t index, const float* mods) const {
    return blendKnob(index, knobs[index], mods);  // extract blendKnob to accept mods arg
}
```

`beginBlock` continues to set `m_mods` for audio-thread `tickSmoothers()` inside `blendKnob` during `ProcessBlock`.

### D3 — No UI layer changes

**Choice:** Keep `SubModulePanel::updatePairArKnobDisplay` and web `syncKnobUi` unchanged; they already call effective getters when mod ≠ 255.

**Why:** Repetition violation if UI re-implements blend; root cause is host/state, not missing slider updates.

### D4 — Tests

**Choice:**

1. C++ unit test: set knob 0.5, mod depth 1.0, mod source 0, mods[0]=0.0 → effective ≈ 0.0; mods[0]=1.0 → effective ≈ 1.0
2. Optional Playwright: assign LFO mod to pair-AR cell, sample two `screen` messages, assert `pairArRows[0].value` changed

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| Dual mod pointer (`m_mods` member + read-time arg) | Single `blendKnob(index, knob, mods)` helper; both paths call it |
| Web already works — unnecessary churn | D2 is read-path only; verify web before/after; skip if redundant |
| Confusing effective vs smoothed display | Document in spec: display = instantaneous blend; DSP envelope rates use smoothed attack/release params |

## Migration Plan

1. Ship host/audio wiring fix in desktop build
2. Rebuild WASM if `AudioPairArState` signature changes (web bundle)
3. No snapshot version bump (behavior fix only)

## Open Questions

- None blocking — desktop missing `beginBlock` is confirmed in source; web path already calls it via `PagedHostIO::ProcessBlock`.
