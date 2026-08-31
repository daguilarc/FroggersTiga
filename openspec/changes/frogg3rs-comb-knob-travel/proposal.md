# Proposal — `frogg3rs-comb-knob-travel`

**Created 2026-08-31.** Two Filter-bank knobs spend their travel badly, and
for the same reason: they are linear (or exponential in the wrong variable)
where the ear hears something else. The comb does NOT need to be audible at
launch (operator ruling); every default stays byte-identical.

## The mechanism, traced

The comb reaches the output through exactly two routes
(`app/dsp/FilterFx.hpp:725-774`):

    combPath = trim · comb(input)
    peakIn   = input·(1−topology) + combPath·topology      Topology, slot 9
    mixed    = peakPath·(1−blend) + combPath·blend         Comb/Peak, slot 7

Both knobs arrive RAW — `app/FroggersAppCore.hpp:1855` passes
`knob(Filter, 9)` and `knob(Filter, 7)` unmapped. Both defaults are 0
(`app/FroggersParameters.hpp:185-186`), so at launch the comb is fully
disconnected, which is intended and stays.

Comb feedback (slot 5) maps through `Comb::GetFeedback`
(`app/dsp/FilterFx.hpp:525-534`): bipolar about knob 0.5, each half sweeping
t = 2·|knob−0.5| through `ZeroedExpCompute(0.25, t)`
(`app/dsp/DspMath.hpp:59-62`) scaled to ±0.95
(`kMaxFeedbackMagnitude`, a deliberate divergence from the firmware's ±1.1
so the loop always decays — that comment and cap stay).

## Why the travel feels wrong

What the ear tracks in a feedback comb is ring time, ~1/(1−|fb|). Under the
current curve, walking a half outward: 60% of travel reaches |fb|≈0.71
(~3.5 regenerations), 80% ≈0.85 (~6.6), and the whole climb to the
20-regeneration drone happens in the last sliver before the rail. A fresh
launch has the knob at 0 — the −0.95 rail — so sweeping up collapses through
the entire audible range in the first ~20% of the knob, exactly as reported.

The blend has the mirror problem: a linear crossfade holds the comb below
−6 dB for the whole lower half of its travel, so its interest crowds the top.

## The fix, two curves, endpoints untouched

1. **Feedback: spend travel log-linearly in ring time.** Per half:

       |fb|(t) = 1 − ExpMapCompute(1.0f, 1.0f − kMaxFeedbackMagnitude, t)

   i.e. the feedback GAP decays geometrically from 1 to 0.05, so ring time
   is exactly 20^t — every equal knob step multiplies ring time by the same
   ratio. t=0 still gives 0 (center silent), t=1 still gives exactly
   0.95 (rails unchanged, so the launch value at knob 0 is bit-identical
   and `comb_feedback_at_both_knob_extremes_decays_to_silence` pins the
   same endpoints). Sign convention and bipolar shape unchanged.

2. **Comb/Peak: equal-power crossfade.** `app/dsp/FilterFx.hpp:771` becomes

       mixed = peakPath·cos(blend·π/2) + combPath·sin(blend·π/2)

   Mid-knob puts the comb at −3 dB instead of −6; both extremes remain
   exactly single-path (cos 0 = 1, sin 0 = 0), so blend=0 is bit-identical
   to today, including the launch patch. The comb and peak paths are
   decorrelated by the comb's delay line, which is the case equal-power
   crossfades exist for.

NOT changed, with dispositions: **Topology** stays a linear morph — it is a
routing crossfade whose spec already promises "no switched positions
anywhere in its travel", and linear is the honest shape for a topology
blend. **Comb drive** already maps geometrically with a −12 dB floor
(`ExpMapCompute(0.25, 4.0, ·)`, unity at its 0.5 default) and is untouched.
**Defaults** all stay: slot 5 knob 0 maps to the same −0.95, slots 7/9 stay
0 and bit-identical through both new curves at their zero points, so the
launch/Reset/New equality suite and the default patch are unaffected by
construction.

## Enumeration duties for execution

- `GetFeedback` callers: the audio path (`FroggersAppCore.hpp:1826`), the
  transfer-function visualizer path (which reads `comb.feedback` state at
  `FilterFx.hpp:440` and so follows automatically — verify, do not assume),
  and every test naming it. Report FOUND vs CHANGED, zeros included.
- `ZeroedExpCompute` users after the change — if `GetFeedback` was its last
  caller, say so and leave the helper (other banks may use it; enumerate).
- Tests pinning mid-curve feedback or blend values (extremes are unchanged
  by construction; only mid-curve pins can move). The DSP parity suite
  already treats `GetFeedback` as a deliberate divergence from the frozen
  firmware — extend that divergence note, do not weaken parity elsewhere.

## Impact

- `app/dsp/FilterFx.hpp` (`GetFeedback`, the mix line, their comments).
- Tests under `app/` that pin mid-curve values, if any (enumeration
  decides; expected small).
- No Sheaf changes, no submodule pin movement, no parameter-model slot
  changes, no default changes. All commits on `main`.
- Gates: app suite fresh (currently 322/0), browser wasm build; the
  operator's by-ear confirmation on the deployed site AFTER push — the
  feedback sweep should be interesting across its whole travel and the
  blend should present the comb by mid-knob.
