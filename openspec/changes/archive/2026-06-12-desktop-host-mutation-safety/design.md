## Context

Desktop sim runs JUCE message thread (UI) and a real-time audio callback (`AudioEngine::audioDeviceIOCallbackWithContext`). `DesktopHostIO::tickControls()` runs at block start on the audio thread, then `FroggersEngine::ProcessBlock`.

**Current split-brain:**

| Mutation | Thread today | Safe? |
|----------|--------------|-------|
| VCO morph nudge / Randomize waves | Queued → audio | Yes |
| Randomize all / mod | UI → `PageManager` direct | **No** |
| Patch cable assign | UI → `SetPageModSource` | **No** (uint8 + float writes) |
| Knob drag | UI → `KnobUpdateOnPage` | Risk accepted v1 (continuous); randomize burst is worse |
| Delay randomize mod | UI → `DelayState` direct | **No** |

NaN path: torn `m_knobValue` / `m_modAmount` or poisoned reverb lines → non-finite block → silence while Play still shows running. `SoftResetFxState()` today runs only in `stopAudio()` when `m_lastBlockNonFinite`.

Patch overlay bidirectional drag is **already implemented** in `PatchCableOverlay.cpp`; this change closes randomize + queue gaps and recovery.

## Goals / Non-Goals

**Goals:**

- All **burst** UI mutations (randomize, mod assign, morph) applied on audio thread via one queue drained in `tickControls()`.
- One sim randomize distribution: **P(none)=0.5**, else uniform on `{0, 4, 5, 6}`.
- Delay `randomizeMod()` updates **sources + depths** with same picker.
- Init-time sanitize clears legacy `{1, 2, 3}` assignments.
- Non-finite output triggers FX soft-reset **during playback**, not only on Stop.
- Confirm patch cable manual matrix (including empty gray input → mod output).

**Non-Goals:**

- Queue every knob drag (high frequency; separate proposal if needed).
- Web patch cables (dropdown remains v2.1 web model — see `web-sim-page-ux`).
- Firmware randomize behavior change.
- Drag cable body (VCV advanced).

## Decisions

### 1. Unified mutation queue (extend morph queue)

Replace morph-only queue with typed commands:

| `HostMutationType` | Payload | Applied on audio thread |
|--------------------|---------|-------------------------|
| `NudgeMorph` | index, delta | `NudgeVcoMorph` |
| `RandomizeMorphs` | — | `RandomizeVcoMorphs` |
| `RandomizePage` | page | `RandomizePage` |
| `RandomizePageMod` | page | `RandomizePageModSim` |
| `RandomizeAllPages` | — | `RandomizeAllPagesIndependent` |
| `RandomizeAllMod` | — | `RandomizeAllPagesModSim` + `DelayState::randomizeMod` |
| `SetPageModSource` | page, row, modIndex | reject invalid; set |
| `SetPageModDepth` | page, row, depth | set |
| `DelaySetModSource` | row, modIndex | `DelayState::setModSource` |
| `DelayRandomizeMod` | — | `DelayState::randomizeMod` |

Keep ring buffer + atomic read/write (same pattern as `m_morphQueue`). On overflow, drop oldest or coalesce duplicate randomize-all — **coalesce**: if queue already has pending `RandomizeAllMod`, replace rather than stack.

UI entry points (`GlobalStrip`, `PanelBackend`, `PatchCableOverlay`) call `enqueueMutation()` only.

**Alternative rejected:** `std::mutex` on engine — audio thread blocking.

### 2. Sim randomize picker (single definition)

```cpp
// SimModSource.hpp
inline uint8_t PickSimRandomModIndex(RGen& rgen) {
  if (rgen.UniGen() < 0.5f) return 255;
  static constexpr uint8_t kPool[] = {0, 4, 5, 6};
  return kPool[rgen.RangeGen(4)];
}
```

`Parameter::RandomizeModSim(float knobPos)` — same depth randomization as today + `PickSimRandomModIndex`.

`PageManager::RandomizePageModSim` / `RandomizeAllPagesModSim` loop rows calling `RandomizeModSim`.

Firmware path unchanged: `RandomizeMod` keeps `RangeGen(x_numMods)`.

### 3. Sanitize at Init only

`DesktopHostIO::Init()` after `SetAllParamsTracking()`:

- Loop all core pages/rows: if `m_modIndex ∈ {1,2,3}` → set `255`, zero depth.
- `DelayState::sanitizeModSources()` same rule.

Reject at `SetPageModSource` / `setModSource` remains (`IsValidSimModAssignment`). No post-randomize sanitize (randomize path cannot emit 1–3).

### 4. NaN recovery (three triggers)

```text
audio callback:
  ProcessBlock → if any output sample !isfinite:
    SoftResetFxState() + delay.softResetFx()  (same thread, once per block max)

stopAudio / audioDeviceStopped / audioDeviceError:
  if m_lastBlockNonFinite or device stopped unexpectedly:
    SoftResetFxState() + delay.softResetFx()
```

Add `m_resetFxThisBlock` guard to avoid repeated full reset every sample in a runaway NaN loop.

### 5. Patch overlay — verify, don't rewrite

State machine in `PatchCableOverlay::finishDrag` matches VCV table. Tasks focus on manual §5 matrix. No code rewrite unless manual fails.

### 6. WASM / web

AudioWorklet runs mutations on audio thread already. Wire `froggers_randomize_all_mod` → `RandomizeAllPagesModSim` + delay sim randomize. No desktop queue on web.

### 7. Superseded changes

- `desktop-patchbay-vcv` — do not implement
- `desktop-host-corrections` tasks 2.x morph queue — absorbed; remaining NaN tasks finish here

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| One-block latency on randomize | Acceptable at 512 buffer (~12 ms) |
| Queue overflow on spam click | Coalesce randomize commands |
| Knob drag still cross-thread | Document; burst randomize was primary NaN source |
| Dual Delay + core randomize in one command | Single `RandomizeAllMod` mutation applies both atomically on audio thread |

## Migration Plan

1. Land mutation queue + sim randomize (core + Delay + WASM bindings).
2. Land NaN in-play recovery in `AudioEngine`.
3. Wire UI enqueue paths; remove direct `RandomizeAllMod` from `GlobalStrip`.
4. Init sanitize.
5. Manual patch + randomize matrix; update `stereo-delay-page` §E at archive.
6. Archive supersede `desktop-patchbay-vcv` and close redundant `desktop-host-corrections` audio tasks.

## Open Questions

- None blocking. Web UX polish tracked in `web-sim-page-ux`.
