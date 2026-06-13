## Why

On mobile (≤720px), the knob grid stays four columns wide between the page-nav arrows. Each cell is narrower than the 44px rotary knob, so VCO morph waveform buttons overflow and are buried under neighbor knobs. Users cannot see or tap waveforms on the Audio page.

Separately, the built-in phone mic path passes **raw hot samples** into WASM (`echoCancellation: false`, no worklet pre-limit). On phones, speaker bleed causes runaway ring-mod feedback and clipping before the engine's internal limiter can tame it.

## What Changes

**UX cleanup:**
- Remove mod-bay hint “CV trace while playing” above `#mod-bay` (redundant with mod source UI).

**Mobile VCO morph layout (CSS):**
- At ≤720px, switch `.knobs` to a **2-column** grid.
- Change `.knob-row` to `grid: 1fr auto` so morph buttons stay in-cell.
- Morph `z-index` fallback for tap targets.

**Phone mic feedback / limiter (worklet + capture):**
- In `froggers-processor.ts`, apply **pad + soft-limit** in the external-input loop before writing WASM heap (same rational saturator as core `TanhSaturator`).
- On mobile (`max-width: 720px`), enable **`echoCancellation: true`** in `getUserMedia` (keep `autoGainControl: false` so our limiter stays authoritative).
- Increase core `x_extInputLimiterDrive` (3.0 → 5.0) — WASM rebuild required; tames ext-in inside engine after worklet stage.

## Capabilities

### New Capabilities

- `web-vco-morph-mobile`: VCO morph waveform buttons visible and tappable on narrow viewports.
- `web-phone-mic-limit`: Stronger external-input limiting and mobile echo cancellation for built-in phone mic.

### Modified Capabilities

- (none)

## Impact

- `web/index.html` — remove mod-bay hint line
- `web/src/style.css` — mobile knob grid / knob-row layout; drop `.mod-bay-hint`
- `web/src/froggers-processor.ts` — external input pad + soft-limit in one process loop
- `web/src/main.ts` — mobile-aware `getUserMedia` constraints
- `src/core/FroggersEngine.hpp` — higher ext-input limiter drive (web WASM + desktop ext-in)
- `wasm/` — rebuild after core change
