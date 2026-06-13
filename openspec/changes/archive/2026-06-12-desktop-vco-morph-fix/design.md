## Context

**Data flow (intended):**

```text
Wave click / Rand waves
  → DesktopHostIO mutation queue
  → drainMutationQueue() in tickControls()
  → FroggersEngine SetKnob / Cycle / Randomize on m_vcoMorph[]
  → ModulatedMorph → EvalWaveMorph in ProcessBlock
  → WaveMorphButton.setMorph(GetVcoDisplayMorph)
```

**Actual (broken):**

```text
GetMorph: ExpParam::Compute(0, 1, knob) → NaN for knob > 0
ModulatedMorph: NaN → 0.f
EvalWaveMorph(..., 0) → sine always

CycleVcoMorph: direct m_engine.CycleVcoMorph on UI thread (race)
RandomizeVcoMorphs: queued but only drains inside audio callback (needs Play)
```

## Goals / Non-Goals

**Goals:**

- Morph knob 0 / 0.5 / 1 produces morph ~0 / ~0.5 / ~1 in audio and icon.
- Randomize sets continuous morph; audible timbre change with Play.
- All morph mutations on audio thread via existing `HostMutation` queue.
- UI icon updates after morph change even when audio stopped.

**Non-Goals:**

- Web wave control changes (desktop fix first).
- Removing continuous morph (randomize stays granular, not three-way flip).
- New mod bus for morph CV.

## Decisions

### 1. Fix GetMorph — linear, not ExpParam(0,1)

```cpp
float GetMorph(ModMgr* modMgr) const {
    float knob = m_knobValue;
    if (modMgr && m_modIndex != 255)
        knob = modMgr->Modulate(m_knobValue, m_modIndex, m_modAmount);
    return std::isfinite(knob) ? jlimit(0.f, 1.f, knob) : 0.f;
}
```

**Rationale:** Morph domain is already 0–1 blend between wave shapes. `ExpParam::Compute(0,1,x)` is mathematically invalid (`min=0`).

### 2. Add HostMutationType::CycleMorph

```text
struct HostMutation { ... uint8_t morphIndex; };
CycleVcoMorph(i) → enqueue { CycleMorph, morphIndex=i }
applyMutation → m_engine.CycleVcoMorph(i)
```

Remove direct `m_engine.CycleVcoMorph` from `CycleVcoMorph()`.

### 3. Idle queue drain

In `MainComponent::timerCallback`, when `!m_audio.isAudioRunning()`:

```text
m_audio.getHost().DrainPendingMutations();
```

Add public `DesktopHostIO::DrainPendingMutations()` that calls the existing private `drainMutationQueue()`. No MIDI drain in the timer path — `tickControls` during Play handles MIDI. Morph-only mutations apply on the message thread when stopped; engine state is not touched from audio callback while idle.

### 4. Cycle knob targets unchanged

Keep `CycleVcoMorph` engine logic: `<0.25 → 0.5`, `<0.75 → 1`, else `0`. With linear GetMorph, these map to sine / saw / square regions in `EvalWaveMorph`.

### 5. Wave icon uses display morph

Continue `setMorph(getVcoDisplayMorph())` after enqueue + on timer refresh. With fixed GetMorph, icon path from `EvalWaveMorph` reflects real blend.

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| Idle drain applies randomize without Play | Icon updates; tooltip says Play to hear |
| CycleMorph + RandomizeMorphs same block | Queue order preserved; both apply in one drain |

## Migration Plan

1. Fix `VcoWaveMorph::GetMorph`.
2. Add `CycleMorph` to mutation enum + apply path.
3. Route `CycleVcoMorph` through queue; SubModulePanel click calls host API only.
4. Idle drain in timer.
5. Manual: Play → click wave icons → hear timbre change; Rand waves → continuous morph; icons update.

## Open Questions

- None blocking.
