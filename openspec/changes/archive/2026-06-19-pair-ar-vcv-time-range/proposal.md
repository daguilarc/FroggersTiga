## Why

Pair-sum A/R Attack and Release knobs map knob 0–1 to an exponential time span. **Today** `PairArEnvelope.hpp` hard-codes **1 ms – 2 s** (`minSec = 0.001f`, `maxSec = 2.0f`) — a stale snippet from early `audio-pair-ad-controls` planning. VCV Fundamental ADSR Attack/Decay/Release use **1 ms – 10 s** (`MIN_TIME = 1e-3`, `MAX_TIME = 10`, ratio 10 000). Operators expect the wider Rack span while keeping the current **level-follower** behavior (envelope tracks pair-sum magnitude, not a gate).

**OMNI audit (explore):**

| Aspect | FroggersTiga today | VCV Fundamental ADSR | Typical Rack ADSR modules |
|--------|-------------------|----------------------|---------------------------|
| Knob min τ | 1 ms | 1 ms | Often 1 ms – 10 ms |
| Knob max τ | **2 s** | **10 s** | Often 1 s – 10 s (some wider) |
| Knob law | `min × (max/min)^k` | Same display law via `LAMBDA_BASE` | Usually exponential |
| Driver | Pair-sum level follower | Gate ADSR stage machine | Gate ADSR |
| Integrator | One-pole `coeff = 1/(τ·sr)` | `λ = pow(base,-k)/MIN_TIME`, `Δenv = (tgt-env)·λ·dt` | Varies |

The **knob→time-constant mapping** should match VCV’s displayed range; the **envelope driver** stays a follower (not gate ADSR). Magic numbers must live in one named constant block; the pow formula must reuse `PhaseUtils::ExpParam::Compute` (already used across the engine) — not a second inline copy.

**Artifact defect fixed:** prior spec claimed knob 0.5 equals `sqrt(10)` **seconds** (~3.16 s). Correct geometric midpoint is **0.1 s** (`1 ms × 10 s` at knob 0.5 in log space).

## What Changes

- Replace inline `minSec` / `maxSec` in `KnobToOnePoleCoeff` with **`kMinTimeSec` / `kMaxTimeSec`** on `PairArEnvelope` (VCV-aligned: 1 ms, 10 s)
- Map knob → seconds via **`PhaseUtils::ExpParam::Compute(kMinTimeSec, kMaxTimeSec, knob)`** — same formula as `FroggersEngine::ExpMap`; no duplicate pow block
- **Max time 2 s → 10 s**; min stays 1 ms; follower semantics unchanged
- Extend `PairArEnvelope_test.cpp`: assert constants and min/max mapped τ; existing rise/fall ordering tests unchanged
- Document range in `SIM_MANUAL.md` and synced copies — pair-AR table: **1 ms – 10 s exponential**; note follower vs gate; distinguish from Delay row (~0–2 s)
- Correct stale **2 s** claim in `openspec/changes/audio-pair-ad-controls/design.md` (historical artifact; also fixes incorrect “via ExpMap” note — pair-AR never wired through `ExpMap`, only duplicated the law inline)

**Soft preset impact:** saved knob values unchanged; presets with Attack/Release near maximum become audibly slower (intended). No snapshot version bump.

**Non-goals:** Gate-triggered ADSR, matching VCV’s λ·dt integrator bit-for-bit, UI knob reskin, VCV panel, randomize wiring (`pair-ar-randomize-parity`), web hover hints (Delay-only pattern today).

## Capabilities

### New Capabilities

- `pair-ar-vcv-time-range`: Pair-AR time constant authority (1 ms – 10 s), VCV ADSR-aligned knob mapping, manual documentation, unit tests

### Modified Capabilities

- (none — baseline specs not archived on main)

## Impact

- `src/core/PairArEnvelope.hpp` — named constants; `ExpParam::Compute`; remove stale inline snippet
- `sim/PairArEnvelope_test.cpp` — range regression tests on constants + mapped endpoints
- `SIM_MANUAL.md`, `docs/sim-manual.md`, `web/public/sim-manual.md`
- `openspec/changes/audio-pair-ad-controls/design.md` — fix outdated 2 s line and ExpMap wording
- No host IO, WASM rebuild required for logic change only (same struct layout); rebuild WASM before web deploy
