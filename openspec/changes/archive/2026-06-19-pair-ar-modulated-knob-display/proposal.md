## Why

The four Audio pair-AR knobs (Attack/Release 1+2 and 2+3) were added with the same mod-routing model as page rows, but on **desktop** their rotary positions stay frozen at the stored knob value while a mod source is active — unlike every other knob on the panel. That breaks operator feedback and violates OMNI parity: one mod-display pipeline for all assignable knobs.

Root cause (explore): `AudioPairArState` blends against the mod bus via `beginBlock(m_mods)`, but the desktop audio callback wires `delay.beginBlock(...)` and never `m_pairAr.beginBlock(...)`, so `m_mods` stays null and `getEffectiveKnob()` returns the base knob. Page-row knobs use `Parameter::Get(ModMgr*)` with a permanent bus pointer and do not have this gap.

**Web:** the WASM path calls `PagedHostIO::ProcessBlock`, which already invokes `m_pairAr.beginBlock`. Pair-AR knobs should track modulation when audio is running. When audio is stopped, display may still stall (same as any mod-driven motion that depends on the audio loop) — verify and align read path if needed.

## What Changes

- **Desktop audio callback:** call `m_host.m_pairAr.beginBlock(m_host.m_pageManager.m_modMgr.m_mods)` each block alongside `m_delay.beginBlock` — restores modulated DSP *and* UI refresh while audio runs
- **Host IO read path:** `GetAudioPairArEffective` (desktop + WASM) passes the live mod bus at read time so knob display works even before the first audio block (OMNI: same instantaneous blend semantics as `Parameter::Get`, not the smoothed envelope state used in DSP)
- **No UI duplication:** reuse existing `SubModulePanel::updatePairArKnobDisplay` / web `syncKnobUi` — fix is host/state wiring only
- **Regression test:** extend `PairArEnvelope_test` or add a small `AudioPairArState` unit test asserting effective knob moves when mod bus changes
- **Optional Playwright smoke:** assert pair-AR `row.value` from processor differs from base knob when mod assigned and LFO active (web only)

**Non-goals:** changing pair-AR DSP curves, label layout, mod assign UI, or VCV panel work

## Capabilities

### New Capabilities

- `pair-ar-modulated-knob-display`: Pair-AR knobs SHALL display live modulated position (effective value) whenever a mod source is assigned, matching page-row knob behavior on desktop and web

### Modified Capabilities

- (none — baseline `openspec/specs/` has no archived pair-AR capability yet)

## Impact

- `desktop/Source/AudioEngine.cpp` — add `m_pairAr.beginBlock` in process callback
- `src/core/AudioPairArState.hpp` — effective read accepts live mod bus (or permanent `ModMgr` reference)
- `src/core/DesktopHostIO.hpp`, `src/core/PagedHostIO.hpp` — `GetAudioPairArEffective` uses live bus
- `sim/PairArEnvelope_test.cpp` or new test — modulated effective value
- `web/e2e/` (optional) — pair-AR knob motion under mod
