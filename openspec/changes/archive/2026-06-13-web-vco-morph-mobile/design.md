## Context

```
Mobile layout (≤720px):

  | ◀ | [V1+k?][V2+k?][V3+k?][Crunch] | ▶ |   ← 4-col: morph under neighbor knob
  | ◀ | [V1 + wave] [V2 + wave]      | ▶ |   ← 2-col: morph visible

External audio today:

  getUserMedia(echoCancellation: false) → worklet
       → raw input[i] copied to WASM heap
       → FroggersEngine m_extInputLimiter (drive 3.0) → envelope → Schmidt gate → ring mod

Phone problem: hot mic + speaker bleed → envelope stays high → ring mod howl / clip
```

Engine already has `TanhSaturator<true> m_extInputLimiter` at drive 3.0 and Schmidt gate on envelope (`m_extGate` 0.01 / 0.005). Web worklet adds **no** pre-conditioning today.

## Data Flow

| Stage | Input | Transform | Output |
|-------|-------|-----------|--------|
| Capture | `getUserMedia` | mobile: `echoCancellation: true`; AGC off | `MediaStream` |
| Layout | viewport ≤720px | CSS 2-col `.knobs`, grid `.knob-row` | morph visible |
| External block | `input[i]` | `pad * softLimit(drive * sample)` in one loop | limited samples → WASM heap + `inputPeak` |
| Engine | limited ext samples | existing limiter @ drive 5.0 → env → Schmidt | ring mod mix |
| Morph UI | `lastMorphs[i]` | existing `renderVcoMorphButtons` | unchanged |

One loop in `process()` mutates samples once before `heap.set`. Peak meter reads **limited** peak (what engine sees).

## Goals / Non-Goals

**Goals:**

- VCO morph visible and tappable on Audio page at 375px.
- Phone mic external input: reduced feedback runaway and clipping with External on + Play.
- Preserve mobile ◀ ▶ nav and centered `#app` column.
- OMNI: one worklet loop for pad+limit+peak; one `getUserMedia` constraint branch; no AnalyserNode side chain.

**Non-Goals:**

- Device picker or mic gain UI.
- Desktop getUserMedia constraint changes (mobile-only echo cancellation).
- Firmware / Daisy changes.
- Output master limiter (separate from ext-in path).

## Decisions

### D1: Mobile knob grid = 2 columns

**Choice:** `@media (max-width: 720px) { .knobs { grid-template-columns: repeat(2, minmax(0, 1fr)); } }`

**Why:** 4 columns ≈ 43px/cell at 375px — narrower than the 44px knob. 2 columns ≈ 87px/cell fits knob + morph.

### D2: knob-row = grid, morph in auto column

**Choice:** `.knob-row { display: grid; grid-template-columns: 1fr auto; }` with centered knob and end-aligned morph.

**Why:** Flex centering lets morph spill into adjacent cells.

### D3: Morph z-index fallback

**Choice:** `.vco-morph-btn { position: relative; z-index: 1; }`

### D4: Breakpoint = 720px

**Choice:** Same boundary as `web-field-center-align` for layout and mobile mic detection.

### D5: Worklet pre-limiter (single loop)

**Choice:** Module-level constants in `froggers-processor.ts`:

```typescript
const EXT_IN_PAD = 0.4;
const EXT_IN_DRIVE = 2.5;

function softLimit(x: number): number {
  const x2 = x * x;
  const y = x * (27 + x2) / (27 + 9 * x2);
  return Math.max(-1, Math.min(1, y));
}
```

In the external-enabled branch, per sample: `const s = softLimit(EXT_IN_DRIVE * input[i] * EXT_IN_PAD)` → write `s` to heap; accumulate `|s|` for peak.

**Why:** Matches core `TanhSaturator::Saturate` math; OMNI accumulate-then-apply in one pass. Phone mics often arrive near full scale; pad + limit before WASM prevents envelope pegging Schmidt gate.

**Alternative rejected:** Main-thread `GainNode` — extra node; limiter still needed for peaks.

### D6: Mobile echo cancellation

**Choice:** When `window.matchMedia("(max-width: 720px)").matches`, request:

```javascript
audio: { echoCancellation: true, noiseSuppression: true, autoGainControl: false }
```

Desktop/wide: keep `echoCancellation: false` (line-in / interface use case).

**Why:** Browser AEC reduces speaker→mic feedback on phones; AGC stays off so level is predictable with D5.

### D7: Raise engine ext-input limiter drive

**Choice:** `FroggersEngine.hpp`: `x_extInputLimiterDrive` **3.0 → 5.0**.

**Why:** Second limiting stage inside engine after worklet pad; stronger tanh compression before envelope/Schmidt. Applies to desktop ext-in too at slightly lower hot-input level — acceptable trade for consistency.

**Rebuild:** `wasm/` cmake build + copy `froggers.wasm` to `web/public/`.

### D8: Schmidt gate unchanged

**Choice:** Keep `m_extGate(0.01f, 0.005f)` — D5+D7 lower envelope feeding the gate instead of raising thresholds.

**Why:** Avoid silent desktop line-in when thresholds rise; limiter stack is the right layer for phone hot levels.

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| 2-col grid adds vertical scroll | Same 8 knobs; acceptable on mobile |
| Echo cancellation colors ext-in on mobile | Intended for phone mic; desktop path unchanged |
| Quieter ext-in on desktop after drive 5.0 | Still audible; user can turn up source |
| WASM rebuild required | Task includes wasm build + copy |

## Migration Plan

1. CSS layout (D1–D3).
2. Worklet limiter (D5) + main.ts capture (D6).
3. Core drive bump + WASM rebuild (D7).
4. Verify: 375px morph tappable; phone External on + Play — no runaway clip; meter shows activity without pegging.

## Open Questions

- (none)
