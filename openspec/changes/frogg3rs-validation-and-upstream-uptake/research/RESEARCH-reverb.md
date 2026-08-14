# Reverb bank, slots 11-12: candidate research

Scope: 2 empty encoder slots in the Reverb bank (slots 11-12, between Tank Drive [10] and
Tuned [13]). Read `app/FroggersParameters.hpp` and `app/dsp/Reverb.hpp` (plus `app/dsp/FilterFx.hpp`
and `app/dsp/Drive.hpp` for reusable DSP already in the tree) before writing this.

Tank recap (verified from `Reverb::Process`, `app/dsp/Reverb.hpp`):
`lineA`/`lineB` (4096 samples each), `dA = 180 + sizeNorm*1300`, `dB = 260 + sizeNorm*1800`,
diffusion cross-feed (`cross = diffusion*0.5`), `dampFilter` (one shared `dsp::OnePoleLowPass`),
`preLine` pre-delay, LFO offset on the read taps (Mod depth), `PadeSaturator::Saturate` INSIDE the
feedback path (on `aFb`/`bFb`, before the `fb` multiply), then `wetLimiter`
(`dsp::OutputLimiter`, tuned `kReverbWetLimiterThreshold=0.72f`, attack 2us, release
`kSharedReleaseSeconds`) applied to the fully mixed dry/wet output as the very last step of
`Process()`. `fb = decayFb + (1-decayFb)*min(hold, 0.999)`, ~0.99998 at Hold maxed.

Already decided (not open for debate here): Mod Rate (9), Tank Drive (10, pre-gain into the
existing `PadeSaturator`), Tuned (13, tank delay length driven directly by the knob).

DSP available elsewhere in the tree, confirmed by reading the files, relevant to what follows:
- `dsp::DigitalReorganizer::Process`/`Mangle` (`app/dsp/Drive.hpp`) — XOR + bit-scramble, DC-blocked,
  bounded near [-1,1] by construction. Currently only wired into the Drive bank's audio path (Bit
  depth/XOR), never inside a feedback loop.
- `dsp::ResonantBump` (`app/dsp/FilterFx.hpp`) — RBJ peaking biquad, already used for the Filter
  bank's Peak freq/gain/Q. Ships a shared ceiling constant `dsp::kMaxResonantBumpHeight = 2.0f` and
  a proven, MEASURED companion pattern (`FilterFxChain::peakTrimSmoother` + `peakLimiter`, B1/B5)
  for bounding it when swept — a scalar trim alone was measured insufficient (1.669x worst case)
  because a stateful biquad's energy survives a height drop.
- `dsp::OnePoleLowPass` (`app/dsp/DspMath.hpp`) — already used twice in `Reverb.hpp` (`dampFilter`)
  and reusable as a highpass by subtracting its own lowpass output from the input (identity used
  elsewhere in the codebase, e.g. `Oversampler2x`'s anti-alias filter is the same primitive).
- `dsp::PolynomialDrive` (`app/dsp/Drive.hpp`) — the wavefolder behind the Drive bank's "Shape",
  already proven continuous/musical across its full knob range in this exact codebase.
- `dsp::OutputLimiter` (`app/dsp/Limiter.hpp`) — the same class already backstopping the reverb
  (`wetLimiter`) and the Filter bank's peak branch (`peakLimiter`); the established pattern in this
  codebase for "bound what escapes, don't touch what persists" (per the operator's own framing,
  cited in `Reverb.hpp`'s header comment).

No pitch shifter exists anywhere in the tree (confirmed by the file list above) — every candidate
below that would need one is flagged accordingly.

---

## Ranked candidates

### 1. Grit — bit-scramble tank degradation
**Short label:** `Grit`

- **0.0:** identical to today's tank — true bypass (`flip=0`, `hashBits=0`), matches this
  codebase's own "0 = no-op" convention for every other newly-authored param (Mod depth, Hold, Tank
  Drive all default to a parity no-op at 0).
- **0.5:** a grainy, aliased edge creeping into the tail — the decay stops being smooth and starts
  ticking/crackling as it dies, like a reverb playing back through a failing sample-and-hold.
- **1.0:** the tail is mostly digital hash — pitched aliasing artifacts dominate over the "clean"
  decayed signal, a broken/lo-fi character.
- **Precedent:** Qu-Bit Nautilus's `Chroma`/`Depth` pair applies a selectable effect — including "a
  customizable bitcrusher" — independently within each delay line's own feedback path, i.e. exactly
  this placement (inside the loop, not on the dry/wet bus).
  [Qu-Bit Nautilus](https://www.qubitelectronix.com/shop/p/nautilus)
- **DSP reused:** `dsp::DigitalReorganizer::Process`/`Mangle` (`app/dsp/Drive.hpp`), called on
  `aFb`/`bFb` (or the tank's read taps `valA`/`valB`) before the existing `PadeSaturator::Saturate`
  call already there.
- **Cost tier:** **reuses-existing** — cheapest of the eight. No new struct, just a new call site for
  a function that already exists and is already proven DC-safe.
- **Headroom verdict:** LOW RISK but not zero. `Mangle`'s output is bounded near [-1,1] by its own
  construction (the clamp at `app/dsp/Drive.hpp`'s `DigitalReorganizer::Mangle`), and if inserted
  *before* the existing `PadeSaturator::Saturate` call it stays inside the same bound that already
  guards `aFb`/`bFb`. Still needs the same measured-sweep treatment every other in-loop stage in this
  file got (S2a.1's own convention) before shipping — inserting a new nonlinearity ahead of the
  saturator changes what the saturator actually sees, even if the theoretical bound holds.
- **Why it earns a slot:** cheapest candidate on the list, zero conceptual overlap with any taken
  slot, and it is the single closest match to the instrument's own stated vocabulary
  ("bit-scrambling... not a clean studio plate").

### 2. Tilt — bipolar post-tank tone shave
**Short label:** `Tilt`

- **0.0:** dark tail — bass-heavy, treble rolled off, like the reverb is happening behind a wall.
- **0.5:** flat/neutral — today's tonal balance, unchanged.
- **1.0:** bright, thin tail — treble emphasized, closer to a metallic sheen.
- **Precedent:** two independent hardware citations for exactly this shape of knob (bipolar,
  continuously useful through the center, not a switch):
  - Noise Engineering Desmodus Versio's **Tone** knob — "a bipolar filter in the reverb tank:
    leftward engages lowpass filtering, rightward engages highpass filtering, with the center
    position disabling it entirely." [Desmodus Versio manual](https://manuals.noiseengineering.us/dv/)
  - Make Noise Erbe-Verb's **Tilt** — "shapes the final tone of the reverb... the last operation in
    the algorithm so it has no effect on the energy, feedback, or nature of the reverberations."
    [Erbe-Verb manual (PDF)](https://www.makenoisemusic.com/wp-content/uploads/2024/03/Erbe-VerbManual.pdf)
- **DSP reused:** `dsp::OnePoleLowPass` (`app/dsp/DspMath.hpp`), instantiated twice: one direct
  lowpass tap, one complementary highpass (`input - lowpass(input)`, the same identity already
  implicit in this codebase's other one-pole uses), crossfaded around the knob's 0.5 center.
- **Cost tier:** **composes-existing** — two extra filter instances and a blend, no new primitive.
- **Headroom verdict:** applied to `wet`/`mixedOut` *before* the existing `wetLimiter.Process()`
  call (the very last line of `Process()`), so any treble boost is automatically caught by the
  limiter already there — no new limiter needed structurally. BUT: `kReverbWetLimiterThreshold`
  (0.72) and its attack/release were measured (per this file's own header comment) against the
  *un-tilted* spectral content, with a worst-case transient overshoot of 1.28x found at the smallest
  room size. A maximally bright Tilt setting changes the spectral shape hitting that limiter and
  should be re-swept at Tilt=1.0, per the standing rule that level-adjacent changes need their
  headroom re-derived, not assumed — even when a limiter is already in the signal path.
- **Why it earns a slot:** the strongest, most literal real-world precedent of anything on this
  list (two well-known modules, same mechanism, same rationale for the design), and it is
  structurally the cheapest to make provably safe since it slots in ahead of an already-tuned
  limiter.

### 3. Resonance — sweeping resonant peak in the tank
**Short label:** `Reso`

- **0.0:** peak parked at the low end — a boomy, sub-heavy emphasis on the tail.
- **0.5:** peak mid-spectrum — a vowel-like, almost vocal resonance riding the decay.
- **1.0:** peak at the high end — a glassy, bell-like ring on top of the reverb.
- **Precedent:** Eventide Blackhole's reverb engine includes "filters... and resonance" as part of
  its parameter set (Sound on Sound's review of the plugin, citing the manufacturer's own
  documentation). [Eventide Blackhole review, Sound on Sound](https://www.soundonsound.com/reviews/eventide-blackhole)
  This instrument's own Filter bank already uses the identical DSP primitive for the same purpose
  (Peak freq/gain/Q), so this candidate is also internally consistent with the instrument's existing
  comb/peak vocabulary, not just externally precedented.
- **DSP reused:** `dsp::ResonantBump` (`app/dsp/FilterFx.hpp`), a second instance inserted where
  `dampFilter` currently sits (processing `valA`/`valB` or `aOut`/`bOut`), frequency swept by the
  knob at a fixed, moderate height/Q (reusing `dsp::kMaxResonantBumpHeight` for consistency with the
  Filter bank's own ceiling).
- **Cost tier:** **composes-existing** — new instance of an existing struct, new wiring point.
- **Headroom verdict:** RAISES LEVEL, and this codebase has already measured exactly this failure
  mode once: `FilterFxChain`'s own peak branch needed BOTH a trim (`peakTrimSmoother`, `1/height`)
  AND a dedicated `peakLimiter` (B5) because a same-instant scalar trim could not retroactively
  remove energy already stored in a stateful biquad after a height drop (measured worst case 1.669x
  with the trim alone). A reverb-tank Resonance control needs the same two-part treatment, not just
  a pass through the existing `wetLimiter` — that is real, non-trivial headroom work, already proven
  necessary once in this exact codebase for the exact same primitive.
- **Why it earns a slot:** the best thematic fit with the instrument's existing comb-resonance
  identity of anything on this list, at the cost of being the most engineering-expensive of the
  "composes-existing" tier — a legitimate #3, not a #1, because of that cost.

### 4. Spread — line A/B spacing ratio
**Short label:** `Sprd`

- **0.0:** today's fixed ratio between `dA`/`dB` — no change.
- **0.5:** a moderate widening of the ratio between the two lines' delay lengths — subtle chorus-y
  beating between the two taps.
- **1.0:** maximally divergent ratio — pronounced comb-interference, metallic/detuned character
  without any pitch-shifting.
- **Precedent:** Noise Engineering Desmodus Versio's **Dense** knob — "Controls delay line spacing,
  transitioning from delay-like characteristics (left) to smeared reverb character (right)."
  [Desmodus Versio manual](https://manuals.noiseengineering.us/dv/)
- **DSP reused:** the existing `dA`/`dB` formula in `Reverb::Process` (`app/dsp/Reverb.hpp`) — no new
  primitive, just a second multiplier decoupling `dB`'s scale from `dA`'s instead of both riding
  `sizeNorm` identically.
- **Cost tier:** **composes-existing** (arguably close to reuses-existing — it is a formula
  parameterization, not a new DSP stage).
- **Headroom verdict:** SAFE. Changes only integer delay-line lengths, never amplitude — same risk
  profile as the already-decided Tuned (13) and the existing Room size.
- **Why it earns a slot / caveat:** cheapest and safest candidate after Grit, but flagged for a real
  overlap risk with Room size (both touch `dA`/`dB`'s scale) — defensible because Room size moves
  both lines together and Spread moves them apart, a genuinely different audible effect (absolute
  size vs. detuned interference), but the operator should weigh whether that distinction is worth a
  slot next to Room size already sitting at index 1.

### 5. Fold — wavefolder in the feedback path
**Short label:** `Fold`

- **0.0:** no additional folding — parity no-op.
- **0.5:** the tail starts folding over itself on peaks — a harmonically rich, slightly chaotic
  buzz layered on the decay.
- **1.0:** heavy folding — the tail becomes a dense, aggressive wall of harmonics.
- **Precedent:** Qu-Bit Nautilus's `Chroma` feedback-path effect set includes "a built-in wavefolder"
  as one of the selectable in-loop textures. [Qu-Bit Nautilus](https://www.qubitelectronix.com/shop/p/nautilus)
- **DSP reused:** `dsp::PolynomialDrive` (`app/dsp/Drive.hpp`), the same wavefolder already driving
  the Drive bank's "Shape" — its `SetCoefs`/`Process` reused directly, called on `aFb`/`bFb` ahead of
  the existing `PadeSaturator::Saturate`.
- **Cost tier:** **composes-existing.**
- **Headroom verdict:** RAISES LEVEL more aggressively than Grit does — `PolynomialDrive`'s
  coefficients run up to ~11 (`coefs[0] = 1 + 10*Sine01(...)`, `app/dsp/Drive.hpp`), so even routed
  ahead of the existing `PadeSaturator` (which does bound the final write), the saturator is being
  driven far harder than it is today. Needs the same measured sweep Tank Drive (10) itself needed.
- **Why it's ranked lower, not why it earns a slot:** real precedent and reuses real DSP, but it
  competes directly with both Grit (same "texture in the loop" territory) and the already-decided
  Tank Drive (both are "extra nonlinearity ahead of the saturator") — taking both Grit and Fold would
  spend both open slots on adjacent flavors of the same idea. Included for completeness/contrast,
  not as a top pick.

### 6. Seal — input-injection freeze crossfade
**Short label:** `Seal`

- **0.0:** today's behavior — the tank fully absorbs new input every sample (parity default).
- **0.5:** the tank half-ignores new input — existing energy dominates, new material blends in
  faintly.
- **1.0:** the tank stops absorbing new input entirely — whatever's already circulating loops
  indefinitely, a true freeze.
- **Precedent:** Qu-Bit Nautilus's classic **Freeze** — "locks the delay lines based on the current
  clock rate." [Qu-Bit Nautilus](https://www.qubitelectronix.com/shop/p/nautilus) Most hardware
  "freeze" controls (Nautilus's included) are gated/binary, not continuous — this candidate is
  deliberately redesigned as a continuous crossfade on the `preOut` term feeding `aIn`/`bIn`
  specifically to satisfy the continuous-range hard constraint, not a direct port of the hardware
  behavior.
- **DSP reused:** none new — a crossfade coefficient on the existing `preOut` contribution inside
  `aIn = preOut + fb*PadeSaturator::Saturate(aFb)` / `bIn`'s equivalent line.
- **Cost tier:** **composes-existing** (new mixing logic, no new struct).
- **Headroom verdict:** SAFE — this only ever *removes* the `preOut` contribution, never adds gain.
- **Why it's ranked lower:** two real problems, both worth flagging plainly. (1) Rule-3 risk: this
  is functionally a second route to "the tank never dies," which is exactly what Hold (already
  decided, slot 8) already does by a different mechanism (raising feedback vs. gating injection) —
  the operator's own rule singles out near-synonyms of Hold as slot-wasting. (2) At Seal≈1.0 combined
  with Hold maxed, the tank can reach a state that never returns to silence even absent input, in
  tension with the Stop-transport `Reset()` contract this file already documents as load-bearing
  (`Reverb::Reset()`'s own comment: "without this the reverb keeps ringing after the operator stops
  the transport" — Seal would make an *already-quiet* tank able to relaunch itself indefinitely on
  old energy in a way Reset() would have to explicitly zero, same as it already does for lineA/lineB).

### 7. Scrub — forward/reverse read blend
**Short label:** `Scrb`

- **0.0:** normal forward-only tank read (parity default).
- **0.5:** an even blend of forward and reverse-read taps — a smeared, granular, semi-scrambled
  texture, closer to granular time-smear than a discrete "reverse" flip.
- **1.0:** fully reverse-read — the tail plays back its own recent history backwards, a classic
  swelling reverse-reverb texture.
- **Precedent:** Make Noise Erbe-Verb ships a **Reverse** control ("Manual or voltage controlled
  REVERSE reverb"), and Mutable Instruments Beads/Clouds's granular texture engine crossfades grain
  playback direction/position continuously rather than switching it.
  [Erbe-Verb manual (PDF)](https://www.makenoisemusic.com/wp-content/uploads/2024/03/Erbe-VerbManual.pdf) ·
  [Mutable Instruments Beads manual](https://pichenettes.github.io/mutable-instruments-documentation/modules/beads/manual/)
- **DSP reused:** none — needs a second, independent read index walking backward through
  `lineA`/`lineB`, crossfaded against the existing forward `readA`/`readB` taps.
- **Cost tier:** **genuinely-new** — no existing struct does bidirectional or granular reads
  anywhere in this tree.
- **Headroom verdict:** LOW direct risk (blending two reads of already-bounded signal doesn't add
  gain), but flagged because Erbe-Verb's own "Reverse" is described as a mode/gesture in its source
  hardware, not natively continuous — the continuous version proposed here is a genuine redesign to
  satisfy the hard constraint, not a straight port, so its "musically useful across the whole sweep"
  claim is less battle-tested than candidates 1-4.
- **Why it's ranked lower:** most expensive of the "no new spectral content" candidates, and the
  precedent has to be adapted rather than borrowed directly.

### 8. Shimmer — octave-up pitch shift in the feedback path
**Short label:** `Shim`

- **0.0:** no shimmer — parity default.
- **0.5:** a blended octave-up layer riding underneath the normal decay — the classic "ethereal
  pad" reverb texture.
- **1.0:** the feedback path is dominated by the pitched-up layer — the tail becomes a rising,
  chorus-like wash.
- **Precedent:** the canonical reference for this exact control — Valhalla Shimmer, "9 continuous
  parameters... Shimmer" among them.
  [ValhallaShimmer](https://valhalladsp.com/shop/reverb/valhalla-shimmer/) — plus Eventide's own
  dedicated shimmer product line (ShimmerVerb / Blackhole+MicroPitch), confirming this is a
  well-established, distinct effect from ordinary reverb parameters, not a stretch application of
  one. [ShimmerVerb](https://apps.apple.com/mx/app/shimmerverb/id1463786346)
- **DSP reused:** none. **As stated in the brief, there is no pitch shifter anywhere in this
  codebase** (confirmed against the full `app/dsp/` file list) — this would need a new granular or
  PSOLA-style pitch-shift engine built from scratch and threaded into the feedback path.
- **Cost tier:** **genuinely-new**, the most expensive candidate on this list by a wide margin —
  the only one requiring an entirely new DSP category, not a new instance/composition of something
  already in the tree.
- **Headroom verdict:** RAISES LEVEL, and by more than any other candidate here — a pitch-shifted
  feedback layer introduces new spectral energy at a different octave that stacks with the existing
  ~50,000x steady-state gain the tank already reaches at Hold's ceiling. Per the standing rule, this
  would need its own from-scratch headroom re-derivation (almost certainly its own limiter or trim,
  mirroring `wetLimiter`/`peakLimiter`'s own measured tuning process) — the biggest net-new tuning
  surface of any candidate here, exactly the cost the brief warned this option carries.
- **Why it's ranked last, not first:** included because the brief explicitly invited it and the
  precedent is undeniable, but it fails the "prefer candidates that don't need [a pitch shifter]"
  guidance on cost alone when six cheaper, well-precedented, on-theme alternatives exist above it.

---

## Summary table

| # | Name | Tier | Reuses | Headroom | Overlap risk |
|---|------|------|--------|----------|---------------|
| 1 | Grit | reuses-existing | `DigitalReorganizer` | low, verify sweep | none |
| 2 | Tilt | composes-existing | `OnePoleLowPass` x2 | low, backstopped by `wetLimiter` | none |
| 3 | Resonance | composes-existing | `ResonantBump` | high — needs trim+limiter (proven pattern) | none |
| 4 | Spread | composes-existing | existing `dA`/`dB` formula | none | Room size (defensible) |
| 5 | Fold | composes-existing | `PolynomialDrive` | high, drives saturator harder | Tank Drive, Grit |
| 6 | Seal | composes-existing | none new | none | Hold (rule-3 risk) + Reset() contract |
| 7 | Scrub | genuinely-new | none | low | precedent is a mode, adapted here |
| 8 | Shimmer | genuinely-new | none — no pitch shifter exists | highest of all 8 | none, but priciest by far |
