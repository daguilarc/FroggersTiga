# Empty-encoder-slot research, ROUND 2 — Drive bank (slots 10-13) and Delay bank (slots 10,12,13)

Read in full before this round: round-1 doc (path in the task brief) and, fresh for this round,
`app/dsp/Drive.hpp`, `app/dsp/Delay.hpp`, `app/dsp/DspMath.hpp`, and `app/dsp/Vco.hpp:102-159` (the
accepted PM-LFO precedent the brief points at, to calibrate what "structurally unreachable by
routing" looks like in this exact codebase).

## Operator corrections applied
- **Feedback Drive is DECIDED at Delay slot 9.** Not re-litigated below; excluded from ranking.
- **Bias is not rejected**, but its headroom flag is now *partially* resolved: the operator supplied
  the exact remedy (`Mangle(x+bias) - Mangle(bias)`, the same subtract-after-shift construction
  `DigitalReorganizer::Process` already uses, Drive.hpp:284-299/336-339) generalized to
  `Mangle(x)` read as "whatever nonlinearity Bias precedes." Applied to `PolynomialDrive`, this
  guarantees `Process(0) == 0` exactly at every Bias setting (silence stays silence) — the acute
  f(0)!=0 danger flagged in round 1 is closed. What it does **not** close: `PolynomialDrive::Process`
  is an unbounded 5th-order polynomial, not a bounded lookup like `Mangle`, so `F(x+bias) - F(bias)`
  cancels the *constant* term exactly but does not by itself bound *peak swing* across the whole Bias
  sweep — that residual is real and stays flagged, just narrower than round 1's version. Bias is
  ranked below this round's two new candidates, both of which have *no* headroom flag at all.

## New search this round: hunting the Vco PM-LFO shape
`Vco::StepPmLfo`/`PmDepthScale` share one knob (`pmKnob01`) between two multiplied roles — LFO rate
and LFO depth — that no mod-matrix route can separate, because routing modulates the knob's single
exposed *value*, not the two internal roles that value plays. Grepped `app/dsp/Drive.hpp` and
`app/dsp/Delay.hpp` for the same shape (one existing knob/value feeding two different fixed-ratio
roles inside one formula) rather than re-proposing round 1's structurally weaker "new stage" ideas.
Found two:

1. **Drive.hpp `PolynomialDrive::SetCoefs`** (:95-105): `computedGain` (i.e. the Drive knob's
   resolved gain) is added into **two** of Shape's five coefficients at a hardcoded weight of
   `0.25f` — `coefs[1] = 10*Sine01(coefsKnob*1.618 + 0.25*(computedGain-1))`, same for `coefs[3]`.
   Drive already leaks into Shape's asymmetry at a fixed 0.25 ratio; nothing lets you dial that
   leakage up or down independent of Drive's own gain value or Shape's own base coefficients.
2. **Delay.hpp `StereoDelay::Process`** (:316,330): the Width knob (`p.dwid`) feeds **two** different
   stereo mechanisms at two different hardcoded weights — `widthSpread = dwid*baseSeconds*0.35`
   (time-offset between L/R taps) and `cross = dwid*0.5` (cross-feed blend of `fbL`/`fbR`). One knob,
   two roles, fixed 0.35:0.5 ratio between them, exactly the PM-LFO shape.

Both are this round's #1-ranked candidates for their bank — see below.

---

## DRIVE bank — 8 candidates, ranked

### 1. Link — `Link` — NEW
- **0.0** Shape's coefficients are computed from the Shape knob alone — `computedGain` no longer
  leaks in at all (today's fixed 0.25 term is zeroed). Drive and Shape become fully independent axes.
  **0.5** today's shipped behavior (0.25 leak), unchanged from current default. **1.0** doubled
  leak — pushing Drive up visibly skews Shape's asymmetry harder than today, a stronger "drive drags
  the fold sideways" character.
- **DSP reused:** `dsp::PolynomialDrive::SetCoefs` (Drive.hpp:95-105) — this candidate multiplies the
  existing `0.25f * (computedGain - 1.0f)` term by a new knob-controlled scalar (0..2, mapped from
  Link knob 0..1) in place of the hardcoded `0.25f`. No new struct, no new state.
- **Cost:** reuses-existing — cheapest possible: one literal becomes one knob-fed multiply inside a
  function that already runs every block.
- **Headroom / f(0):** none, by construction. `PolynomialDrive::Process(0)` is `gain*(0*coefs[0] + ...)`
  — every term carries a positive power of `input`, so it is exactly 0 regardless of what the
  coefficients are, at every Link setting. And `coefs[1]`/`coefs[3]` are `10*Sine01(...)`, which is
  bounded to `[-10,10]` for *any* argument — scaling the coupling weight cannot push those
  coefficients outside the range they already reach today at extreme Drive+Shape settings. Zero
  headroom risk across the whole sweep.
- **Precedent:** the general idiom of exposing "how much does A's value leak into B, independent of A
  and B's own controls" as its own knob is a standard, widely shipped synth pattern — most concretely
  as filter **Key Tracking Amount**: "a dedicated modulation control that determines to what extent
  the cutoff frequency will track the keyboard... even with full tracking on, there is a need for an
  additional control to bias the cutoff" (Spectrasonics Omnisphere 2 manual,
  [support.spectrasonics.net](https://support.spectrasonics.net/manual/Omnisphere2/25/en/topic/layer-page-filters-page05)).
  Honest caveat: no shipped *wavefolder* names this exact coupling a knob — the precedent is for the
  *shape* of the control (decoupling an existing internal leak), not this specific circuit.
- **Why it beats the alternatives:** unlike Bias (residual peak-swing flag) or Cascade (explicit
  headroom flag), Link is provably headroom-neutral at every setting, costs one multiply, and doubles
  as a "cancel Bias's own side-effect on Shape" utility if Bias ships too.
- **Why the mod matrix can't already do this:** the matrix can route any of the 15 sources onto
  Drive or onto Shape's own parameter value — but both routes modulate the *whole* knob, moving
  gain (hence tone, hence loudness) or the base coefficient set together. There is no source/target
  pair that isolates *just* the 0.25 cross-term inside `SetCoefs`, because that term is not a
  parameter — it's a fixed weight inside the formula that computes another parameter's internal
  state. Only a new knob that literally is that weight can reach it.

### 2. Fold — `Fold` — NEW
- **0.0** today's fixed pre-fold scale (`out/4.0`) — unchanged default. **0.5** tighter scale
  (`out/2.0`-ish) — the sine-fold engages more readily at lower Drive settings, denser harmonic
  content before the fuzz blend. **1.0** very tight scale (`out/1.0`-ish) — the fold wraps almost
  continuously, a dense, near-triangle/aliased character distinct from what raising Drive alone
  produces (Drive changes `out`'s magnitude AND the polynomial's own coefficient mix; Fold only
  changes how hard that fixed `out` gets pushed into the sine-fold).
- **DSP reused:** `dsp::FrogBlock::Process`'s inline lambda (Drive.hpp:376-380) — the `sinIn = out /
  4.0f` divisor becomes knob-mapped (e.g. `ExpMapCompute` from 4.0 down to ~0.5) instead of a literal.
  No new struct; `Sine01` (DspMath.hpp:28-33) and `PadeSaturator::Saturate` are unchanged.
- **Cost:** reuses-existing — one literal becomes one knob-fed divisor inside code that already runs.
- **Headroom / f(0):** none. `Sine01(0/anything) == sin(0) == 0`, so `Process(0)` stays 0 at every
  Fold setting. `Sine01`'s output is bounded to `[-1,1]` for *any* argument by definition (it's a
  sine), so no matter how small the divisor gets, the folded branch of the fuzz-blend can never
  exceed the range it already occupies today. `PadeSaturator::Saturate` is bounded by its own clamp.
  Genuinely headroom-free across the whole sweep — the strongest possible verdict on this list.
- **Precedent:** TipTop Audio's **Fold** eurorack wavefolder — the `Fold` control is literally a
  pre-gain into the folding stage: "turning up the Fold control adding folds one-by-one" from "a
  nearly undistorted sound at the counter-clockwise position"
  ([signalsounds.com](https://www.signalsounds.com/tiptop-audio-fold-eurorack-wavefolder-module-black)).
  Joranalogue **Fold 6**'s own `Fold` knob is the same idea (already cited round 1 for Bias's
  precedent set, distinct control on that same module —
  [joranalogue.com](https://joranalogue.com/products/fold-6)).
- **Why it beats the alternatives:** the only Drive candidate on either round's list with a
  *provably* zero headroom flag AND a named, literal shipped-product precedent for exactly this
  control (pre-gain into a sine-fold) — Tone and SRR Spread are also headroom-free but have no
  fold-specific product precedent this strong.
- **Why the mod matrix can't already do this:** routing a source onto Drive changes `out`'s
  magnitude by changing `gain` (and, via `SetCoefs`, the coefficient mix too) — a different, coupled
  effect. There is no existing parameter whose value *is* the sine-fold's pre-scale divisor; it is a
  literal inside a lambda, invisible to the matrix until promoted to its own knob.

### 3. Bias — `Bias` — carried from round 1, headroom re-verdicted
- **0.0/0.5/1.0** unchanged from round 1: full negative bias (even-harmonic-skewed fold) / symmetric
  (today) / full positive bias (mirrored skew).
- **DSP reused:** `dsp::PolynomialDrive` — apply `bias` before `Process()`, subtract
  `PolynomialDrive::Process(bias)` after (the operator's `Mangle(x+bias)-Mangle(bias)` construction,
  generalized from `dsp::DigitalReorganizer::Process`, Drive.hpp:336-339).
- **Cost:** composes-existing.
- **Headroom / f(0):** **f(0) now closed by construction** — `Process(0+bias) - Process(bias) == 0`
  exactly, at every Bias setting, so silence-in still means silence-out (this was round 1's acute
  flag; it's resolved). **Residual, softer flag stands:** unlike `Mangle` (bounded to a fixed
  integer-derived range), `PolynomialDrive::Process` is an unbounded 5th-order polynomial, so
  subtract-after cancels the *constant* term but does not bound *peak* swing through `coefs[1..4]`
  identically at every Bias setting — still needs the sweep measurement round 1 called for, just for
  a narrower reason now.
- **Precedent:** unchanged from round 1 — Surge XT `Bias`, Joranalogue Fold 6 `Symmetry`, Old Blood
  Noise Alpha Haunt `Bias` (see round-1 doc for links).
- **Why it's ranked below Link/Fold:** those two are headroom-flag-free by construction; Bias still
  carries the narrower peak-swing residual even after the operator's fix, so it costs one more
  verification step before shipping.
- **Why the mod matrix can't already do this:** unchanged from round 1 — DC offset is not expressible
  as routing an existing source onto an existing multiplicative parameter; it's an additive shift
  that must land *before* the nonlinearity, which no existing knob position sits at.

### 4. Tone — `Tone` — carried from round 1, unchanged
- Post-chain one-pole lowpass; **no headroom flag** (a lowpass only removes energy); **f(0)=0**
  trivially (zero state, zero input, zero output). DSP reused: `dsp::OnePoleLowPass` (DspMath.hpp:56-76).
  Cost: composes-existing. Precedent: ProCo RAT `Filter`, EHX Big Muff `Tone`, Klevgrand Degrader
  (see round-1 doc for links). Why the matrix can't do it: no existing parameter's value is "cutoff
  of a filter placed after this whole chain" — every existing filtering lives in a different bank.

### 5. SRR Spread — `SRRb` — carried from round 1, unchanged
- Detunes SRR2's hold-rate relative to SRR1 for beating/interference. **No headroom flag** (SRR is
  sample-and-hold, never raises peak above input). **f(0)=0** (holds/passes zero). DSP reused:
  `dsp::SampleRateReducer` x2 (Drive.hpp:180-227), only the value fed to the second instance's
  `SetFreq` changes. Cost: reuses-existing. Precedent: dual-detuned-bitcrusher DIY/modular idiom
  (ModWiggler thread, see round-1 doc). Why the matrix can't do it: no existing source can make one
  SRR instance's *rate* track the other's rate at a settable ratio; routing lands additively or
  multiplicatively on one instance's own knob, not as a relative offset between two instances.

### 6. Cascade — `Casc` — carried from round 1, unchanged
- Re-drives the signal through the polynomial+fuzz stage a second time, crossfaded in. **Headroom
  FLAG stands** — a second full drive pass compounds gain by construction, needs its own
  limiter-budget re-derivation (same class of work `DriveBlendPhase`'s own limiter required,
  Drive.hpp:473-495). **f(0)=0** is fine now that `FrogBlock(0)==0` (the `DigitalReorganizer` DC fix
  makes the whole chain's silent-input response 0, so re-feeding silence through a second pass is
  still silence) — only the *peak* flag is live, not an f(0) one. DSP reused:
  `dsp::PolynomialDrive` + the existing fuzz-blend line (Drive.hpp:379). Cost: composes-existing.
  Precedent: pedalboard "gain stacking" (Wampler, MusicRadar — see round-1 doc). Why the matrix can't
  do it: no source/target pair re-runs the *whole already-processed signal* back through the same
  nonlinear chain a second time; that requires a second call to `FrogBlock`'s own math, not a knob nudge.

### 7. Edge — `Edge` — carried from round 1, unchanged
- Pre-emphasis (cheap 1-pole highpass) ahead of the fold. Mild headroom flag stands (boosting highs
  ahead of a 5th-order polynomial pushes higher-order terms harder at a given Drive). f(0)=0 (highpass
  of zero is zero). DSP reused: `dsp::OnePoleLowPass` used as `input - LP(input)`. Cost:
  composes-existing. Precedent: general wavefolder pre-emphasis literature, no single named product
  (weakest-precedent item, see round-1 doc). Why the matrix can't do it: no existing parameter is "a
  filter placed strictly before the fold, inside FrogBlock" — filtering elsewhere in the signal path
  is a different bank and a different position in the chain.

### 8. Grind — `Grnd` — carried from round 1, unchanged, ranked last
- Bypasses the digital back end (XOR/hash/SRR1/SRR2) in favor of pure fold+fuzz, continuously
  crossfaded. No inherent headroom flag (crossfade of two already-bounded taps) but real cost-tier
  honesty: this is structurally closer to genuinely-new (a parallel tap, not a drop-in unit) than any
  other Drive candidate here, on both rounds. f(0)=0 (both taps are 0 at silence). DSP reused:
  `dsp::DigitalReorganizer`, `dsp::SampleRateReducer` x2. Precedent: general parallel-saturation mix
  idiom (see round-1 doc). Why the matrix can't do it: it's a dedicated new signal path (a pre-crush
  tap), not a route between an existing source and an existing parameter.

---

## DELAY bank — 6 candidates, ranked

### 1. Width Balance — `WBal` — NEW
- **0.0** Width's stereo-image effect comes entirely from cross-feed (`cross` grows with the knob,
  `widthSpread` pinned near 0) — a phase-coherent, mono-compatible widening, no L/R time-offset.
  **0.5** today's shipped balance (both mechanisms present at their current fixed 0.35:0.5 ratio,
  unchanged default). **1.0** Width's effect comes entirely from time-offset spread (`widthSpread`
  grows, `cross` pinned near 0) — a Haas-style, more aggressively "smeared" stereo character with no
  cross-feed blending.
- **DSP reused:** `dsp::StereoDelay::Process` (Delay.hpp:316,330) — the two hardcoded weights
  (`0.35f` for `widthSpread`, `0.5f` for `cross`) become a pair of complementary knob-derived weights
  instead of fixed literals, both still driven by the existing `p.dwid` value. No new struct, no new
  buffers.
- **Cost:** reuses-existing — cheapest possible: two literals become two knob-fed weights inside code
  that already runs every sample.
- **Headroom / f(0):** none. `fbL = dL*(1-cross) + dR*cross` is a convex combination whenever
  `cross` stays in `[0,1]` — bounded by `max(|dL|,|dR|)` regardless of how the balance knob splits it,
  same as today. `widthSpread` only offsets a `ReadAt` tap position (Delay.hpp:405-421), never adds
  gain — an interpolated read of two buffer samples is bounded by those samples regardless of which
  time it reads at. Both mechanisms individually respect the existing per-sample bound
  (`|inSignal|+fbk`, the same bound round 1's Feedback Drive analysis already established) at every
  Width Balance setting.
- **Precedent:** Bitwig's own stereo Delay device ships `Width` and `Cross Feedback` as two
  **independent, separately-exposed** controls rather than one fixed-ratio knob
  ([bitwig.com Delay reference](https://www.bitwig.com/userguide/latest/delay/)) — real, shipped proof
  that cross-feed and stereo time-spread are treated as separable dimensions worth independent
  control, which is exactly what this candidate exposes as a single balance axis rather than two full
  extra knobs.
- **Why it beats the alternatives:** the only Delay candidate across both rounds with a *provably*
  zero headroom flag, the cheapest cost tier, and a real product that ships the two underlying
  mechanisms as independently-controllable dimensions rather than a fixed internal ratio.
- **Why the mod matrix can't already do this:** the matrix can route a source onto Width itself,
  changing both `cross` and `widthSpread` together at their current fixed 0.35:0.5 ratio — it cannot
  change *the ratio between them*, because that ratio is two hardcoded literals inside one function,
  not a parameter with its own value. Only a new knob that literally is that ratio can reach it.

### 2. Feedback Tone — `FbTn` — carried from round 1, unchanged
- Damps the feedback tap before it's written back (aging-tape darkening over generations). **No
  headroom flag** (a lowpass in the loop only removes recirculating energy, improving the loop's
  stability margin). DSP reused: `dsp::OnePoleLowPass`, inserted into the feedback tap ahead of
  `WriteSample` (Delay.hpp:350-351), alongside the existing `PadeSaturator::Saturate`. Cost:
  composes-existing. Precedent: Strymon El Capistan `Tape Age` (see round-1 doc). Why the matrix
  can't do it: no existing parameter is "a filter placed specifically inside the feedback tap, before
  the write" — the matrix has no target there to route onto.

### 3. Crush — `Crsh` — carried from round 1, one correction
- Bit-crushes the feedback tap for lo-fi repeats. DSP reused: `dsp::SampleRateReducer` and/or
  `dsp::DigitalReorganizer`. **Correction from round 1:** must reuse `DigitalReorganizer::Process` as
  it stands TODAY (already DC-blocked via `Mangle(input,...) - Mangle(0,...)`, Drive.hpp:336-339) —
  round 1 flagged this as a risk to avoid; it's now simply "use the struct as-is," since the fix
  already ships in `Drive.hpp`. With that struct, f(0)=0 and no headroom risk (SRR/hash outputs never
  exceed their input's magnitude). Cost: composes-existing. Precedent: Strymon Timeline Lo-Fi
  `Filter`/`Grit`, Caroline Kilobyte (see round-1 doc). Why the matrix can't do it: it's a dedicated
  new stage in the feedback tap, not a route between an existing source and an existing parameter.

### 4. Diffusion — `Diff` — carried from round 1, unchanged
- Allpass-smears repeats toward reverb-adjacent texture. Unity-gain by construction provided the
  coefficient stays inside the same `0.98` margin `DriveBlendPhase`/`dsp::Comb` already use. DSP
  reused: the allpass recurrence already implemented in-tree at `dsp::DriveBlendPhase::Process`
  (Drive.hpp:577-593, `phased = -a*wet + allpassX1 + a*allpassY1`). Cost: composes-existing, closer to
  the composes/genuinely-new boundary (new plumbing, not a drop-in). Precedent: Valhalla Delay's
  Diffusion section, Chase Bliss Mood `Modify` (see round-1 doc). Why the matrix can't do it: no
  existing parameter is "a short allpass chain applied to the wet tap" — that's new signal-path
  plumbing, not a route.

### 5. Ducking — `Duck` — carried from round 1, unchanged, weakest fit flagged again
- Sidechain-style attenuation of repeats under new input. No headroom flag (only ever attenuates).
  Real, continuous, not a mod-matrix duplicate — but still the most "corrective" item on either list,
  working against the brief's "characterful over corrective" bias, same honest caveat round 1 gave.
  DSP reused: the envelope-follower idiom in `dsp::VcoEnvelopeFollowers` (EnvelopeFollowers.hpp), not
  the struct itself. Cost: composes-existing at the idiom level, closer to genuinely-new as an object.
  Precedent: TC Electronic delay ducking (see round-1 doc). Why the matrix can't do it: dedicated
  dynamics stage wired into the DSP chain, not a mod-matrix route.

### 6. Reverse Blend — `Rev` — carried from round 1, unchanged, costliest
- Continuous forward/backward tap crossfade. Genuinely-new cost tier (second read index, click-free
  crossfade, no existing parameter reuse beyond the buffers themselves). Honest precedent caveat
  unchanged from round 1: reverse delay is real and shipped, but always as a discrete mode
  (Valhalla Delay), never as a continuous knob — that continuity is this document's own
  extrapolation. DSP reused: `dsp::StereoDelay`'s own `lineL`/`lineR` buffers and `ReadAt`. Why the
  matrix can't do it: needs a second, independently-incrementing read pointer into the existing
  buffers — no existing parameter or source models "direction of buffer traversal."

---

## Cross-bank notes
- Both banks' new #1 picks (Link, Width Balance) are strictly cheaper (reuses-existing, zero new
  state) AND strictly safer (provably zero headroom flag) than every carried-over round-1 candidate,
  because both exploit the exact "one value, two hardcoded internal roles" shape the operator's Vco
  precedent pointed at, rather than adding a new stage.
- Bias's f(0) flag is resolved by the operator's own remedy; its narrower peak-swing residual is
  called out precisely so it isn't silently dropped, not as a reason to re-reject it.
- Crush's round-1 caveat (about accidentally reintroducing the f(0)!=0 bug via a hand-rolled copy) is
  now moot: `DigitalReorganizer` ships the fix already, so reusing the struct as-is is simply correct,
  not merely "safer if done carefully."
