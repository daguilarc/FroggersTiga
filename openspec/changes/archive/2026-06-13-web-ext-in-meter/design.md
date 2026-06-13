## Context

Web external audio path today:

```
External click → getUserMedia → micSource.connect(workletNode)
                                      │
                                      ▼
                         processor process(): inputs[0] when externalEnabled
                                      │
                                      ▼
                         WASM ring-mod (Schmidt gate) — no UI peak tap
```

Desktop parity (`InputEnvelopeIndicator`): peak beside toggle, idle grey track with centre tick, blue fill when active.

## Goals / Non-Goals

**Goals:**

- Peak meter visible beside **External** on all viewports
- OMNI: one peak accumulator in worklet; one DOM update per `screen` message; no per-frame main-thread audio scan
- Idle/active states match desktop semantics
- Short status hint when External + Play but sustained silence (~1 s)

**Non-Goals:**

- Device picker (browser default capture device only)
- Separate oscilloscope / waveform trace for external input
- Desktop or WASM engine changes beyond posting peak in `screen`

## Decisions

### D1: Peak in worklet, ship via existing `screen` message

**Choice:** In `froggers-processor.ts` `process()`, when `externalEnabled && input`, compute block peak with `Math.max` over `n` samples; decay-smooth into `this.inputPeak` (same 0.35 blend as desktop). Include `inputPeak` in the periodic `screen` post (every 20 frames).

**Why:** Samples already on the audio thread; avoids extra `AnalyserNode` + `requestAnimationFrame` loop. OMNI accumulate-then-apply: one write in processor, one read in `onScreenUpdate`.

**Alternative rejected:** `AnalyserNode` on main thread — duplicate graph tap and second polling loop.

### D2: `renderInputMeter(peak, active)` — single DOM update

**Choice:** `#external-meter` is a div with inner fill span. `renderInputMeter` sets width % and `data-active` class in one call from `onScreenUpdate` and External/Play state changes.

**Why:** One function, one loop-free update; no CSS reads in a rAF loop.

### D3: Route hint via status line append

**Choice:** Track `silentSampleFrames` on main thread from `screen.inputPeak`. When External on + Play + peak < 1e-4 for ~1 s at 44.1 kHz frame cadence, set status suffix: " — input silent (check mic permission / level)". Clear when peak rises or External off.

**Why:** Reuses `#status` instead of a new label; desktop uses `m_routeHint` but web transport is already status-heavy.

### D4: Meter idle when External off or !audioRunning

**Choice:** `active = externalEnabled && audioRunning`. Peak forced to 0 when inactive.

**Why:** Matches desktop `getInputPeakLevel()` gate and avoids misleading fill when stream is connected but Play is stopped.

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| `screen` posts every 20 frames — meter not sample-accurate | Acceptable for peak meter UX; same cadence as mod scopes |
| Status line crowded with route hint | Append only when silent > 1 s; remove on signal |
| Mobile narrow transport row | `flex-wrap` on `.controls-top` already present; meter min-width 64px |

## Migration Plan

1. Processor peak + `screen` field
2. HTML + CSS meter
3. `main.ts` render + route hint
4. Manual verify: External on + Play + speak → bar moves

## Open Questions

None blocking.
