# Tasks — `frogg3rs-filter-page-review`

Everything HELD: no commit, no push, until the operator lifts the hold.
Findings land as artifacts and diffs for review.

## 1. The per-knob matrix

For each Filter parameter, record: current mapping (formula, at
`FroggersAppCore.hpp:1744-1856` or the dsp setter it calls), what the ear
tracks for that control, whether the curve spends travel on it, the
default and what it decodes to, and a verdict (sound / re-curve / re-span /
re-bound), each with file:line. The fourteen, by current slot:

0. Comb offset · 1. Peak freq · 2. Peak gain (height, capped 2.0 — see
task 3) · 3. Peak Q (width 0.1..10 — evaluate whether the narrow end is
narrow enough and the broad end useful) · 4. Comb delay · 5. Comb feedback
(just re-curved; verdict expected sound) · 6. Comb LP · 7. Comb/Peak (just
floored; verdict expected sound) · 8. Scoop · 9. Topology (linear routing
morph; verdict expected sound) · 10. Scoop freq · 11. Scoop width ·
12. Comb drive (task 2) · 13. Scoop depth.

Close every knob out with a verdict — including "sound, unchanged"; an
unvisited knob is an open item. Verdicts beyond the already-ruled items
(drive, layout, height procedure) are REPORT-ONLY in this wave: they land
in the postflight for the operator to turn into future changes, not as
silent scope.

## 2. Comb drive — implement the decided compensated design

`FilterFx.hpp:414` becomes the compensated form `Sat(drive·x)/drive`, with
the saturator-bound comment REWRITTEN on compressivity (`Sat(y) ≤ y`, so
the fed-back term never exceeds the delayed signal and per-pass decay ≤
|fb| at every drive), and the finite-plot-frame bound (`FilterFx.hpp:63-65`,
`:388`) re-derived on the same argument.
Tests per the decided thresholds: exact unity identity at default and
transport-stopped override; the geometric ring-time guard parameterized
over drive knob {0, 0.5, 1} at the same 8% ratio tolerance; the travel-
audibility case with its fixture level calibrated in-prototype (≤0.5 dB at
knob 0, ≥6 dB span across the top half, knee by mid-knob, measured
old-curve values recorded beside the new constants); sweep-latch probe
untouched. Every new case proven able to fail (break once, binaries
deleted between builds).

## 3. Peak gain ceiling, revisited under the new world

Re-run the limiter measurement scenario (its constants' comment,
`FilterFx.hpp:120-132`, describes the adversarial pattern) at ceilings 3.0
and 4.0 with today's comb defaults, as a TEST_CASE-shaped harness run, not
a shell script. The recorded 1.669 is not seed-reproducible (only seed
0xC0FFEE is in-repo, giving 1.615898 at the checked-in sample count —
`FroggersDspParityTests.cpp:2221-2226`): derive fresh numbers from the
documented pattern and say so. REPORT ONLY in this wave: the constant moves only if the
numbers clear it AND the operator approves at postflight review; otherwise
it ships unchanged with the numbers recorded in the postflight.

## 4. Layout regrouping — trace before choosing

SETTLED: persistence is name-addressed (`ParameterValuesToJSON`/
`LoadParameterValuesFromJSON` by `Name()`,
`External/Sheaf/.../ParameterModulation.cpp:3139-3175`, preflight-traced)
— implement the RE-SLOT. Verify the name-addressing claim once yourself at
execution (read the two functions), then re-order the Filter entries in
`FroggersBankLayouts()`, and move every (bank,slot)-addressed companion in
the same breath: `labels.md` and `FroggersApprovedLabels()[bank][ix]`,
`ApplyBankDefaultPatch`'s Filter literals, the visualizer/underlay slot
associations, `kStopUnityDriveKnob`'s slot reference, the routing block's
`knob(Filter, N)` indices, and every test addressing Filter slots by
index. Enumerate by operand (`Filter, <digit>` and `FroggersBankId::Filter`)
and close out every hit. Target
order: peak freq/gain/Q · comb offset/delay/feedback/LP/drive · scoop/
freq/width/depth · Comb/Peak · Topology, then Crispy/Crunchy. Include
`labels.md`, `FroggersApprovedLabels()`, detents, and the visualizer
underlay slot associations in the impact enumeration.

## 5. Gates

Full app suite fresh (binaries deleted first), wasm build, the
re-measurement harness where task 3 demands it, and the operator's on-mock
look for the layout (real rendering, local server, before any push — the
established gate). Ear confirmation per knob change on the deployed site
after the hold lifts.

## 6. Comb LP direction-inversion fix (matrix finding, operator-ordered)

The defect: `ExpMapCompute(4.0f * combFreq, 20000.0f / sampleRate_,
knob(Filter, 6))` (`FroggersAppCore.hpp:~1830`) — the floor tracks the comb
delay's own frequency, and past Comb delay knob ≈ 0.85 it exceeds the fixed
ceiling, so min > max and the knob's direction reverses. Unguarded, no test.

The fix: clamp the floor to the ceiling — the LP range runs from
"just above the comb's own pitch" up to fully open, and when the comb sits
so high that 4× its frequency is already past fully open, the range
degenerates to fully open (knob inert, never reversed). One
`std::min(4.0f * combFreq, ceiling)` with a behavior comment; no new
constants.

Tests: a case asserting the mapped LP value is monotone non-decreasing in
the LP knob across a grid of Comb delay positions INCLUDING the inversion
region (CmbDly knob {0.85, 0.95, 1.0}), and that floor ≤ ceiling at every
grid point. Proven able to fail by reverting the clamp once (binary
deleted between builds). Full suite fresh afterward, zero FAIL.

## 7. The ratified matrix verdicts (operator-ordered, 2026-09-01)

All four land in this wave, one executor, after task 6 completes:

- 7a. Q floors: Peak Q and Scoop width both become
  `ExpMapCompute(0.4, 10.0, knob)` — floor raised off 0.1 in lockstep
  (Q 0.4 ≈ 3.0-octave bandwidth, preflight-computed: broad, still a
  peak; height=1 is an exact bypass at any Q, so launch is provably
  unaffected). Tests pin both floors from the constants, not literals.
- 7b. Scoop depth curve: the DEPTH wiring becomes
  `ExpMapCompute(1.0f, 0.05f, knob)` — a deliberate decreasing exponential
  (base < 1 is well-defined; comment says so), equal knob steps = equal dB
  of cut, endpoints preserved to within 2 ULP at the top (old knob-1
  value 0.050000012f becomes exactly 0.05f — inaudible; preflight-
  measured) and exactly at the bottom. The old `max(.05, 1−.95·knob)`
  linear form goes. Test: quarter-knob steps give equal dB
  (−6.505 dB/quarter, preflight-computed) within tolerance.
- 7c. Comb delay de-zipper: the comb's tap read becomes fractional
  (linear interpolation between adjacent delay-line samples by the
  fractional part of delaySamples). At integer delays the interpolation
  weight is zero — every existing comb test assigns INTEGER delay
  literals directly (1 and 37 samples, preflight-enumerated), so all stay
  bit-identical; the new test must do the same (a "round Hz" value is NOT
  integer in float32 — 100 Hz maps to 479.99997 samples). New test: at
  the top of the Comb delay knob, adjacent knob steps produce pitch steps
  bounded by a smooth curve, not whole-sample jumps.
- 7d. Scoop/Scoop-depth wiring swap: slot 8 "Scoop" drives the wet/dry
  blend (scoopMix), slot 11 "Scoop depth" drives the notch's dip depth
  (SetHeight, with 7b's curve) — names now mean what they say, matching
  the pre-existing spec prose. Persistence is by name: saved non-default
  scoop values re-interpret (accepted, same class as a re-curve); both
  defaults are 0 so launch is unchanged. The change's parameter-model
  delta is updated to describe the swapped (now name-true) wiring, and
  every test addressing either control follows its parameter
  (preflight: zero tests address slots 8/11 by index; labels are
  positional and unchanged; the one consumer needing an edit is this
  change's own parameter-model delta, whose slots 8-11 line flips to the
  swapped, name-true wiring).

Each fix: proven able to fail once (binaries deleted between builds),
full suite fresh at the end, wasm rebuilt, zero FAIL.

## 8. Scoop moves upstream of the peak (operator-ordered, 2026-09-01)

The ear finding the matrix's per-knob lens missed: the scoop is a SECOND
ResonantBump wired as a post-blend series dip over the whole signal
(`FilterFx.hpp:640,:852`, blend at the return), with freq/width formulas
and default centers identical to the peak's — so at overlapping centers,
Scoop-up subtracts exactly what Peak-up added: "more knob = less filter."

The fix — topology, not curves: the scoop shapes the chain's INPUT.

    scoopedIn = input·(1−scoopMix) + scoopNotch(input)·scoopMix
    (comb and peak both eat scoopedIn; the peak's resonance then lives ON
    TOP of the scooped material and survives any Scoop setting)

- The Scoop knob (slot 8) keeps its meaning-shape: how much of the input
  is pre-scooped. Depth/freq/width knobs unchanged (their maps were just
  fixed; SetHeight semantics untouched).
- Bounds, preflight-verified and then MEASURED, not assumed: the RBJ cut's
  |H| ≤ 1 at all frequencies for height ≤ 1 (algebra on
  `ResonantBump::UpdateCoefficients`, `FilterFx.hpp:276-298`) — but that is
  steady-state; scoop params refresh per sample, and this file's own record
  shows analogy-picked transient bounds measuring 80% wrong. RE-RUN the
  limiter measurement harness (the existing Pattern-2 idiom) with
  scoop-state modulation included in the adversarial pattern, and record
  the numbers.
- NaN blast radius, addressed not ignored: scoopNotch has shipped a
  non-finite state bug before (`FroggersAppCore.hpp:1787-1798`, fixed
  2026-07-27); upstream it feeds the comb's delay line and the peak's
  recursion. Tier-2 recovery (`RecoveryTier.hpp`, the `FilterFx.hpp:697`
  classification — which is fault recovery, NOT plotting; the plan's old
  visualizer bullet was miscited and is retired) remains the mechanism; the
  finiteness suite gains scoop-modulation-while-feeding-the-branches
  coverage. No new defensive branch without a demonstrated failing input.
- scoopNotch has NO visualizer wiring (only peak/comb UIStates populate,
  `FroggersAppCore.hpp:1290-1291`); nothing to reorder there.
- The internal input mix stays PLAIN LINEAR (`input·(1−m) + notch·m`) so
  scoopMix 0 is bit-exact pass-through by the same IEEE argument the
  topology morph's comment already proves (`FilterFx.hpp:791-795`) —
  launch bit-identical.
- Tests: `filter_fx_chain_parallel_matches_manual_comb_peak_scoop_blend`
  (`FroggersDspParityTests.cpp:1773-1889`, blend 0.4 / scoop 0.6) —
  expected expression rewritten to the new order;
  `filter_fx_chain_zero_scoop_mix_is_unaffected_by_scoop_notch_settings`
  (:1990-2023) stays valid unchanged. NEW directional pin: peak boosted at
  a frequency, scoop centered on it, increasing Scoop must not cancel the
  peak's contribution (band energy at the peak's center with Scoop full
  stays above the unboosted baseline) — a small Goertzel helper is
  duplicated into `FroggersDspParityTests.cpp` per that file's
  independent-replica convention (the routing suite's helper is in a
  disjoint binary, `app/Makefile:124-125,189-190`); positive control runs
  the pin against the OLD topology and shows the cancellation caught.
- Spec wording, enumerated: BOTH copies of the "Peak stage reads the
  chain's own input" language (main spec
  `froggers-sheaf-parameter-model/spec.md:84-86` at sync time, the
  change's delta :26-29 now) say the branches read the SCOOPED input;
  the delta's Scoop-stage description (:21) and the two "into the
  output" comment sites (`FroggersAppCore.hpp:1870-1871`) reworded to
  input-shaping.
- Preflight (fresh context) first; executor; postflight; commit and push
  in the normal flow; operator re-ear on the deployed site.
