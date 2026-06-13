## Context

Filter row 0 (`DELF`) feeds `PureDelay` before `Comb::Process`. Row 4 (`COMF`) sets comb pitch. Both looked like “delay” knobs; row 0 is a **phase primer** into the comb, not a second pitch control.

**Supersedes:** `delay-grain-filter-row0` §3 (Comb line, display-only). That change landed the label **Comb line** and left DSP on the inverted freq map.

**Current DSP bug:** knob maps through `ExpParam(20/sr … 20000/sr)` → `PureDelay::SetDelaySamples(freq)` stores `1/freq`. At 48 kHz, knob 0 ≈ 50 ms, knob 1 ≈ 2.4 ms — **shorter** at max knob. Users expect higher knob → longer offset / more smear.

```text
knob ──► ExpParam Hz band ──► 1/freq samples ──► PureDelay ──► Comb
         (inverted vs intent)
```

## Goals / Non-Goals

**Goals:**

- Sim label **Comb offset** (distinct from row 4 **Comb delay**).
- Exponential **0.001 s – 0.1 s**; **knob 0 = 1 ms**, **knob 1 = 100 ms** (monotonic).
- One remap in `ReadParamsBlock`; `ParamDisplayNames` + Quick Dict updated.
- Firmware OLED `DELF` unchanged.

**Non-Goals:**

- Moving grain/detune to Filter (stays on Delay **Detune**).
- Changing comb pitch (row 4), peak EQ, feedback, LP.

## Decisions

### 1. Seconds target, not frequency

Replace `m_pureDelayFreq` Hz target with `m_pureDelaySeconds` (or reuse `RuntimeParam` slot with semantic rename):

```cpp
// ReadParamsBlock — filter row 0
m_pureDelaySeconds.SetTarget(
    PhaseUtils::ExpParam::Compute(0.001f, 0.1f, m_filterParams->GetParam(0)));
```

In `ApplyOutputFx`:

```cpp
m_pureDelay.SetDelaySeconds(m_pureDelaySeconds.Process());
```

### 2. PureDelay API fix

`Comb.hpp` `PureDelay`:

```cpp
void SetDelaySeconds(float seconds) {
    m_delaySamples = seconds * sampleRate;  // sampleRate from engine or passed per block
}
```

Remove misnamed `SetDelaySamples(float freq)` that stored `1/freq`. **OMNI:** one setter, one unit (seconds).

At 48 kHz: 0.001 s → 48 samples; 0.1 s → 4800 samples (within `x_size` 8192).

### 3. Display dictionary

`ParamDisplayNames` Filter row 0: **Comb offset** (replaces **Comb line**).

Quick Dict: `Comb offset : Short line before comb — smears strike, not pitch`

### 4. Label distinction (user-facing)

| Row | Label | Role |
|-----|-------|------|
| 0 | **Comb offset** | Phase primer / smear before comb |
| 4 | **Comb delay** | Comb notch pitch / loop length |

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| Firmware at 48 kHz vs sim at 44.1 kHz | Seconds map is rate-independent; sample count uses `m_sampleRate` |
| Existing presets sound different | Intended — fixes inverted knob |
| `delay-grain-filter-row0` archived with stale Comb line spec | Main spec `filter-comb-offset` is canonical; archive with `--skip-specs` |

## Migration Plan

1. `PureDelay` + `FroggersEngine` seconds remap.
2. `ParamDisplayNames`, `QUICK_DICT.md`, `npm run sync:docs`.
3. Rebuild WASM; verify Filter row 0 label and monotonic sweep.
4. Archive `delay-grain-filter-row0` (already implemented except row 0 DSP).
