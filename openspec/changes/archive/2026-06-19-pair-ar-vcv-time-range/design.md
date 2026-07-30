## Context

```
PairArEnvelope::KnobToOnePoleCoeff (today)
──────────────────────────────────────────
  inline minSec = 0.001f;   ← stale magic
  inline maxSec = 2.0f;     ← 5× shorter max than VCV ADSR
  sec = minSec * pow(max/min, knob)     ← duplicates ExpParam::Compute
  coeff = 1 / (sec * sampleRate)
  level += (target - level) * coeff

PhaseUtils::ExpParam::Compute (engine-wide)
───────────────────────────────────────────
  return min * pow(max / min, value);    ← identical law

VCV Fundamental ADSR (reference — knob display range)
─────────────────────────────────────────────────────
  MIN_TIME = 1e-3f, MAX_TIME = 10.f, LAMBDA_BASE = 10000
  Display: configParam(..., LAMBDA_BASE, MIN_TIME * 1000)
  DSP λ = pow(LAMBDA_BASE, -knob) / MIN_TIME  →  τ_display = 1/λ
  Endpoints: knob 0 → 1 ms, knob 1 → 10 s, knob 0.5 → 0.1 s
```

Envelope **driver** stays unchanged: `FroggersEngine::MixOscVoices` passes `|v1+v2|*0.5` as target — level follower, not gate ADSR. Only the **time mapping** and **constant authority** change.

**Integrator note:** VCV uses `env += (target - env) * lambda * sampleTime`. Pair-AR uses a discrete one-pole with `coeff = 1/(τ·sr)`. Endpoints share the same τ labels; mid-knob slew differs slightly from VCV’s λ integrator. Community reports VCV labeled times already diverge from perceived speed — we align **knob span and mapping law**, not gate ADSR semantics or VCV’s exact λ DSP.

## Goals / Non-Goals

**Goals:**

- Single source of truth for pair-AR min/max time in `PairArEnvelope.hpp`
- Knob mapping matches VCV Fundamental ADSR Attack/Release **display range** (1 ms – 10 s, exponential)
- Reuse `PhaseUtils::ExpParam::Compute` — no second pow implementation
- Remove inline literal snippet from `KnobToOnePoleCoeff`
- Manual documents range, follower semantics, and Delay distinction
- Unit tests lock min/max τ and constant names

**Non-Goals:**

- Gate input or ADSR stage machine
- Changing `PairArEnvelope::Step` follower branch logic
- Bit-identical VCV λ·dt envelope
- Delay time range (still ~0–2 s in `StereoDelay.hpp`)
- Web per-knob hover hints (only Delay has `DELAY_HINTS` today)

## Decisions

### D1 — Named constants on `PairArEnvelope`

```cpp
static constexpr float kMinTimeSec = 1e-3f;  // VCV Fundamental MIN_TIME
static constexpr float kMaxTimeSec = 10.f;   // VCV Fundamental MAX_TIME
```

**Why:** OMNI — one authority for min/max; tests assert these symbols; manual cites the range; no literals in the coefficient path.

### D2 — Reuse `PhaseUtils::ExpParam::Compute` (no `KnobToTimeConstantSec` wrapper)

```cpp
#include "PhaseUtils.hpp"

static float KnobToOnePoleCoeff(float knob, float sampleRate)
{
    const float clampedKnob = std::min(std::max(knob, 0.f), 1.f);
    const float sec = PhaseUtils::ExpParam::Compute(kMinTimeSec, kMaxTimeSec, clampedKnob);
    const float tau = sec * sampleRate;
    return (tau > 1.f) ? (1.f / tau) : 1.f;
}
```

**Why:** OMNI — reuse existing structure; avoids trivial one-line helper (One-Time Helper Extraction Rule). Tests assert `kMinTimeSec` / `kMaxTimeSec` and `ExpParam::Compute` endpoints directly.

**Rejected:** standalone `KnobToTimeConstantSec()` duplicating `ExpParam::Compute`; shared header with VCV plugin (different build).

### D3 — Follower semantics preserved

No change to `Step(target, attackKnob, releaseKnob, sampleRate)` branch logic or `MixOscVoices` target computation.

### D4 — Manual copy (three files)

Extend pair-AR table in `SIM_MANUAL.md` (sync to `docs/` and `web/public/`):

| Control | Range | Notes |
|---------|-------|-------|
| Att./Rel. | 1 ms – 10 s (exponential knob) | Same knob→time mapping as VCV ADSR A/D/R; envelope **follows pair-sum level**, not a gate |

**Why:** Label authority stays `ParamDisplayNames.hpp`; **time range** authority is `PairArEnvelope` constants + manual. Delay row (~0–2 s) stays separate to avoid confusion.

### D5 — Stale artifact cleanup

Update `openspec/changes/audio-pair-ad-controls/design.md` D3: replace `1 ms – 2 s at 48 kHz` and “Times via ExpMap” with `1 ms – 10 s via PairArEnvelope + ExpParam::Compute (see pair-ar-vcv-time-range)`.

### D6 — Center knob test value (spec correction)

Knob 0.5 → **0.1 s**, not `sqrt(10) s` (~3.16 s). Geometric midpoint of 1 ms and 10 s in log space.

## Risks / Trade-offs

- **[Risk] Presets with high A/R knobs sound much slower** → Expected; no schema bump
- **[Risk] Someone re-adds inline literals** → Mitigation: constexpr tests on `kMinTimeSec` / `kMaxTimeSec`
- **[Risk] Confusion with Delay 2 s range** → Manual table distinguishes pair-AR vs Delay row
- **[Risk] “Same as VCV ADSR” over-read** → Manual states follower vs gate; design documents integrator difference

## Migration Plan

1. Constants + `ExpParam::Compute` refactor + tests
2. Sync manuals; fix audio-pair-ad-controls design stale line
3. Rebuild sim tests; rebuild WASM for web
4. Smoke: max Attack 1+2 on Audio page — swell should reach target over **multi-second** timescale, not ~2 s cap

Rollback: revert single commit.

## Open Questions

- None.
