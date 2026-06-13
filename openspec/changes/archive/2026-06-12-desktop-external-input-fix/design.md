## Context

**Current desktop path (broken UX):**

```text
Ext. In. OFF (default) ──▶ m_inBlock = 0 ──▶ engine never sees input
Ext. In. ON  ──▶ copy input ──▶ limiter ──▶ envelope ──▶ Schmidt ──▶ mix

Meter: getEnvelopeLevel() gated by Ext. In. flag AND only updates in callback
       fill width = level → 0 looks like empty black box beside checkbox
```

**`desktop-host-corrections` §1 (never landed):** route device input whenever configured; Schmidt decides mix. Host checkbox was kept as a second gate — contradicts Field + corrections design.

**Clipping context:** Sim adds pre-reverb stereo delay + `applyStereoBus`; no master limiter on desktop output; `SoftResetFxState` clears reverb lines but not comb feedback state.

## Goals / Non-Goals

**Goals:**

- **Ext. In.** checkbox clearly enables line routing; default OFF at launch.
- Meter shows real input activity (peak) when routing active — not a dead black rectangle.
- One gate for mix topology: engine Schmidt only.
- Reduce runaway clipping: output soft limit + comb clear on FX reset.

**Non-Goals:**

- Remove Ext. In. checkbox entirely (web keeps separate External mic toggle).
- Firmware or Daisy limiter port.
- Resampler when device ≠ 44100.
- Changing Schmidt thresholds (0.01 / 0.005) in v1.

## Decisions

### 1. Ext. In. semantics

| Ext. In. | Play | Device in ch | `m_inBlock` | Mix |
|----------|------|--------------|-------------|-----|
| OFF | * | * | 0 | OLVL × VCO |
| ON | OFF | * | 0 (no callback) | OLVL × VCO |
| ON | ON | 0 | 0 | OLVL × VCO |
| ON | ON | ≥1 | copy ch0 | Schmidt → ring or OLVL |

Remove `getEnvelopeLevel()` returning 0 when Ext. In. off for **meter** purposes — split APIs:
- `getInputPeakLevel()` — peak |sample| from last block’s copied input (0 if Ext. In. off or not running)
- `GetEnvelopeLevel()` — engine envelope (unchanged, for MIDI out)

### 2. Default OFF

`m_externalInputEnabled` stays **false** at launch. `MainComponent` toggle unchecked in ctor. User opts in when they want line/mic routing (parity with web **External: Off** default).

### 3. Input level meter (`InputEnvelopeIndicator`)

**Data:** `AudioEngine` tracks `m_inputPeak` = max `fabs(m_inBlock[i])` per callback when Ext. In. on.

**Paint states:**

| State | Appearance |
|-------|------------|
| Ext. In. off or !Play | Grey track + 2px centre tick (not empty void) |
| On + Play, peak &lt; 0.02 | Dim blue fill at peak width |
| On + Play, peak ≥ 0.02 | Full alpha blue fill |

Optional: 15 Hz ballistics on peak for readability (single-pole toward target in `setLevel`).

**Tooltip:** `Input peak (Ext. In. on + Play). Ring mod opens above Schmidt threshold.`

### 4. Label **Ext. In.**

`ToggleButton` text **Ext. In.** Width in `resized()` — use `getBestWidthForHeight` or fixed ~72px (was 88 for "External").

### 5. Output headroom (OMNI: accumulate peak in callback, apply limit once)

In `audioDeviceIOCallbackWithContext`, after `applyStereoBus`:

```text
for each sample in block:
  outL[i] = softLimit(outL[i])
  outR[i] = softLimit(outR[i])   // if stereo
```

Use existing `TanhSaturator<false>::Saturate` or `juce::jlimit(-1,1,tanh(x))` — no new dependency. Drive ~0.95 pre-clip if needed.

**Alternative rejected:** Daisy `Limiter` in desktop — extra link surface; tanh sufficient for sim.

### 6. Extend `SoftResetFxState`

Add to `FroggersEngine::SoftResetFxState()`:

```text
m_comFilter: zero delay line, reset index, feedback state
```

Call path unchanged (NaN block, stop, device error). Delay buffers already cleared via `delay.softResetFx()`.

### 7. Stereo delay → reverb energy

No topology change in v1. Headroom limiter + comb reset address runaway; document in MANUAL_VERIFY that hot **Rand All** + Delay send can clip until user pulls **DSND**/**DFBK**/**RVMX**.

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| User forgets to enable Ext. In. | Meter idle chrome + tooltip; check when using line in |
| Tanh limiter colors transients | Sim-only; gain staging fix not firmware change |
| Meter shows peak but mix still VCO-only below Schmidt | Tooltip explains threshold |

## Migration Plan

1. AudioEngine peak tap + routing clarity + output limiter.
2. InputEnvelopeIndicator paint states.
3. MainComponent label + default toggle sync.
4. Engine comb reset.
5. QUICK_DICT + manual verify steps.

## Open Questions

- None blocking. Schmidt threshold tweak deferred unless user testing demands it.
