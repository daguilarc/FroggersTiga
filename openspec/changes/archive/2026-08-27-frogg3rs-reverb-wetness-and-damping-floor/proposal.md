# Proposal — `frogg3rs-reverb-wetness-and-damping-floor`

**Created 2026-08-26.** Two ceilings on the Reverb bank, both one-line edits,
both aimed at what a randomized patch sounds like: the wet mix is capped too
high, and the Damping knob's travel spends most of itself in a range where the
tail is inaudible mud.

## What the operator asked for

Reduce the maximum reverb wetness to 60%, and raise the floor of the Damping
parameter so muddy, unmusical results stop dominating random outcomes. "It will
be obvious if this floor is too low" — so the floor is a value to try and judge
by ear, not one to derive and declare finished.

## Wet mix

`app/FroggersAppCore.hpp:1827` is the whole mechanism:

    constexpr float kMaxReverbWetMix = 0.7f;

It caps the MAPPED value, not the knob range, so the control keeps sweeping its
full travel and only its top end moves. The blend it feeds is
`(1 - mix) * dry + mix * wet` (`app/dsp/Reverb.hpp`), so the cap is exactly "how
little dry signal the knob can leave": 0.7 leaves 30% dry at maximum, 0.6 leaves
40%.

The cap already exists for this reason — the comment above it records the
operator's own 2026-07-29 request, that mix == 1.0 removes the dry signal
entirely and reads as a drop in level rather than as more reverb. This moves the
same cap further in the same direction. Nothing else reads `kMaxReverbWetMix`.

## Damping floor

`app/dsp/Reverb.hpp:384`:

    static float DampAlphaFromKnob(float knob01) { return ExpMapCompute(0.001f, 0.2f, 1.0f - knob01); }

The result is fed straight to the shared damping filter's coefficient
(`app/dsp/Reverb.hpp:553`, `dampFilter.alpha = DampAlphaFromKnob(dampKnob01)`),
and the knob reaching it is the Reverb bank's slot 4 with nothing in between
(`app/FroggersAppCore.hpp:1859`). The filter is
`output = alpha * input + (1 - alpha) * output` (`app/dsp/DspMath.hpp:83-87`),
so a SMALLER alpha is a darker tail.

`ExpMapCompute(lo, hi, t)` interpolates geometrically from `lo` at `t = 0` to
`hi` at `t = 1`. With the `1 - knob01` argument, that puts the knob's TOP at
alpha 0.001 and its bottom at alpha 0.2. At 48 kHz those are damping cutoffs of
roughly 7.6 Hz and 1.7 kHz.

Why that dominates random patches: `Modifier::Random` draws each parameter's
normalized value from `std::generate_canonical`, uniform over [0, 1)
(`External/Sheaf/projects/synth/src/ParameterModulation.cpp:2893,3595-3599`).
Uniform travel over a geometric mapping means half of all draws land below the
geometric mean, `sqrt(0.001 * 0.2)` = 0.0141 — a cutoff near 108 Hz. Half of
every randomized reverb therefore has a tail with nothing above roughly 100 Hz
left in it. That is the mud.

Raising the floor to 0.02 makes the range one decade, 0.02–0.2, whose geometric
mean is 0.0632: half of draws now land above about 500 Hz, and the darkest
reachable setting is about 154 Hz instead of 7.6 Hz. The knob keeps its full
travel and its direction; only the dark end moves.

The floor is the one number here worth arguing with. 0.01 (darkest ~76 Hz,
median ~342 Hz) is the more conservative choice if 0.02 turns out to have taken
away a dark setting worth keeping.

## Hygiene found in the tree this touches

`MANUAL.md`'s Damping entry has the direction backwards: it says higher knob
values give a brighter, less-damped tail, and the trace above shows the
opposite. Fixed in this change, since the same edit is what the new floor's
description has to be consistent with.

## What Changes

- `kMaxReverbWetMix` 0.7f -> 0.6f.
- `DampAlphaFromKnob`'s lower bound 0.001f -> 0.02f.
- Requirements for both, which neither constant currently has: nothing in
  `openspec/specs/` mentions the wet cap or the damping range today, so both are
  values a future edit could move without any test noticing.
- `MANUAL.md`'s Damping direction, and both entries' stated limits.

## Non-goals

- The knob ranges themselves. Both controls keep their full travel; only what
  the travel maps onto moves.
- Every other reverb parameter, and every other bank.
- The randomize weights. The draw stays uniform; what changes is what the
  uniform draw lands on.

## Impact

- Affected specs: `froggers-sheaf-parameter-model` (which owns the Reverb bank's
  parameters).
- A randomized patch keeps a usable dry signal and a tail you can still hear the
  top of.
