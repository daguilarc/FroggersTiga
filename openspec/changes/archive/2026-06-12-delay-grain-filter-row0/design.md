## Context

**Today — Tone (misnamed):**

```cpp
m_toneL += toneMix * (dL - m_toneL);
dL = m_toneL * (1 - toneMix*0.5f) + dL * (1 - toneMix*0.5f);
```

`toneMix = dton`. No pitch change. `m_toneAlpha` (2 kHz) is never used. Tone runs on **both** L and R.

**Today — DMOD:** LFO multiplies delay time ±8% — chorus/flutter, not static detune.

**Filter row 0:** `pureDelay → comb → bump` unchanged. Short tap for comb color; not stereo Delay.

**Today — Reverb stereo:** `ProcessReverb` returns mono `0.5*(aOut+bOut)`. Sim stereo output comes only from `applyStereoBus` delay wet L/R delta.

## Goals / Non-Goals

**Goals:**

- Row 4 audibly **detunes** wet L/R vs each other (and vs dry) as knob rises.
- Label **Detune** on desktop, web, Quick Dict.
- Filter row 0 **Comb line** — honest, no collision with **Delay time**.
- DTIM max **2.0 s** from one constexpr site.
- Drive rows 4–5 **XOR**, **Bit depth** (display only; `DigitalReorganizer` unchanged).
- Reverb rows 5–6 **Stereo width**, **Diffusion** replace LFO depth/rate in core engine + sim UI.
- OMNI: detune math in `StereoDelay::process` only; reverb width/diffusion in one `ProcessReverb` path; stereo wet merged in **one** `applyStereoBus` loop; no duplicate max-delay literals.

**Non-Goals:**

- Removing `PureDelay` from firmware chain.
- Granular/grain engine.
- Pitch-shifting dry signal.
- Renaming firmware OLED codes (`RMOD`, `RRAT`, `DIGR`, `HASH`).

## Decisions

### 1. Detune DSP — stereo cents on read time

After `baseSeconds`, `modSeconds`, `widthSpread` compute nominal `timeL`, `timeR`:

```text
maxCents = 50   // full knob
cents    = detune * maxCents
ratioL   = 2^(+cents/1200)
ratioR   = 2^(-cents/1200)
readL    = readAt(timeL / ratioL, lineL)
readR    = readAt(timeR / ratioR, lineR)
```

Shorter effective read time → pitch up on that channel. Opposite signs L/R → stereo spread on repeats. Knob 0 → ratio 1 (bypass).

**Rejected:** Resampling buffer — too heavy for sim insert; delay-time scaling matches existing `readAt` interpolation.

**Rejected:** Keep tone LP — user wants detune, not darkening.

### 2. vs Mod depth

| Param | Effect |
|-------|--------|
| **Mod depth** | Periodic delay-time LFO (chorus) |
| **Detune** | Static L/R pitch offset on echo |

### 3. Filter row 0 — Comb line

Display only. `PureDelay` DSP unchanged.

### 3b. Filter rows 1–3 — Peak EQ (not Bump)

`ResonantBump` is a standard peaking biquad (`SetFreq`, `SetHeight`, `SetWidth` where width = Q).

| Row | Firmware | Sim label |
|-----|----------|-----------|
| 1 | BUPF | **Peak freq** |
| 2 | BUPR | **Peak gain** |
| 3 | BUPW | **Peak Q** |

### 3c. Drive rows 4–5 — XOR + Bit depth

| Row | Firmware | DSP | Sim label |
|-----|----------|-----|-----------|
| 4 | DIGR | `SetFlip` — XOR mask on 8-bit sample | **XOR** |
| 5 | HASH | `SetHash` — low-bit scramble depth | **Bit depth** |

### 4. Reverb rows 5–6 — Stereo width + Diffusion (not LFO)

**Remove:** `m_rvLfoPhase`, LFO multiply on delay lengths, `m_rvModRate` usage.

**Rename runtime slots:** `m_rvModDepth` → `m_rvWidth`, `m_rvModRate` → `m_rvDiffusion` (same `RuntimeParam` smoothing).

**Param curves in `UpdateParams`:**

```text
m_rvWidth.SetTarget(m_reverbParams->GetParam(5))           // 0..1 linear
m_rvDiffusion.SetTarget(m_reverbParams->GetParam(6))       // 0..1 linear
```

**Single feedback matrix** (replaces separate LFO + fixed cross):

```text
readA, readB = delay-line reads at fixed baseA, baseB (no LFO)
aOut = damp(lineA[readA])
bOut = damp(lineB[readB])
cross = diffusion * 0.5f
aFb = lerp(lineB[readB], lineA[readA], cross)
bFb = lerp(lineA[readA], lineB[readB], cross)
aIn = preOut + aFb * fb
bIn = preOut + bFb * fb
```

`diffusion = 0` → each line feeds the opposite read (today's topology). `diffusion = 1` → each line feeds a 50/50 blend (denser smear).

**Width pan law** (after damp outputs):

```text
mid  = 0.5 * (aOut + bOut)
wetL = mid + width * (aOut - mid)
wetR = mid + width * (bOut - mid)
```

Store `m_reverbWetL`, `m_reverbWetR` on the engine each sample. `ApplyOutputFx` mixes dry mono `output` with wet L/R using `rvMix`; dry stays centered on both channels.

### 4b. Stereo bus merge (sim + desktop)

```text
coreMono[i]     = dry + rvMix * monoWet   // ProcessSample path; monoWet = 0.5*(wetL+wetR) for firmware
delayDeltaL/R   = from DelayState last wet (existing)
reverbDeltaL/R  = wetL/wetR - monoWet    // from engine getters after ProcessReverb

outL[i] = coreMono[i] + dmix*delayDeltaL + rvMix*reverbDeltaL
outR[i] = coreMono[i] + dmix*delayDeltaR + rvMix*reverbDeltaR
```

One loop in `applyStereoBus`; read delay + reverb deltas once per block from last-sample state (same pattern as delay today).

**Mono hosts:** `(outL + outR) * 0.5` into single buffer.

### 5. Stereo cap 2 s

`kMaxDelaySeconds = 2.0f`, `kMaxDelaySamples = 96000`.

### 6. Param wire

`DelayParams::dton` → `ddet` in struct; update `DelayState` blend path.

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| Detune + width + mod stack oddly | Manual verify moderate settings |
| Extreme detune aliases | Cap at ±50 cents v1 |
| Reverb stereo subtle at low rvMix | Verify with high RVMX |

## Migration Plan

1. `StereoDelay.hpp` detune + 2 s cap; delete tone state.
2. `FroggersEngine.hpp` reverb width/diffusion + wet L/R getters.
3. `applyStereoBus` + `AudioEngine.cpp` reverb delta merge.
4. `ParamDisplayNames` + Quick Dict + manual + stale sibling chrome hints.
5. Rebuild WASM; verify Detune, Comb line, Reverb width/diffusion.
