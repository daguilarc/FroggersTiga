# Postflight — `frogg3rs-filter-page-review`

Executed 2026-09-01 in the held wave. Commits land with the wave's joint
push on operator approval. Suite at wave close: 327/327, all binaries
deleted first; wasm fresh.

## Executed this wave

- **Comb drive compensated** (`Sat(drive·x)/drive`): knob is pure
  saturation depth. Measured depth across knob {0,.25,.5,.75,1}:
  new [0.16, 0.62, 2.18, 6.16, 12.04] dB — monotonic, knee by mid-knob,
  9.9 dB span over the top half; old curve was the mirror image
  [12.20, 6.64, 2.18, 0.14, 0.00] (its "depth" at low knob was loop
  attenuation — the conflation, quantified). Unity default bit-identical;
  ring-time law holds at drive {0, .5, 1} within the same 8% band; bound
  comments rewritten on compressivity; break-once controls ran, one of
  which caught and fixed a defect in the new test's own first draft.
- **Filter bank re-slotted** to peak/comb/scoop/routing order; persistence
  proven name-addressed (`ParameterModulation.cpp:3145,3166` quoted in the
  execution report), so saved patches are unaffected by construction.
  Labels, routing, defaults, visualizer associations, and every
  slot-addressed test moved together; the verbatim-labels drift check
  caught a third hardcoded order array mid-execution — the guard working.
- **Peak ceiling measured, constant unchanged** (report-only, as ruled):
  10 seeds × 50k samples per candidate, log-uniform height draw through
  the real chain at today's defaults —
  pre-limiter worst 1.710 / 2.230 / 2.682 at ceiling 2.0 / 3.0 / 4.0;
  post-limiter worst nearly flat: 0.882 / 0.886 / 0.888; time-in-reduction
  ~99.99% at all three. The limiter absorbs the growth; the open question
  is purely the audible character of heavier sustained reduction. The
  harness stays in the tree as a measurement case.

## The per-knob matrix (report-only verdicts, operator to ratify)

| slot | knob | verdict |
|---|---|---|
| 0 | Peak freq | sound — exponential over the full audible range |
| 1 | Peak gain | ruled: ceiling decision open, measurement above |
| 2 | Peak Q | RE-SPAN — floor Q=0.1 (~6.7 octaves) reads as a tilt, not a peak; raise the floor |
| 3 | Comb offset | sound — 1..100 ms exponential; 96 kHz clamp trims a negligible top sliver |
| 4 | Comb delay | RE-BOUND — whole-sample truncation zippers the top ~15% (4→5 samples ≈ 4 semitones); lower the ceiling or interpolate |
| 5 | Comb feedback | ruled this wave — sound |
| 6 | Comb LP | BUG, urgent — floor tracks 4×combFreq and crosses the fixed ceiling when Comb delay knob ≳ 0.85: ExpMap min>max, the knob's direction REVERSES; unguarded, untested |
| 7 | Comb drive | ruled this wave — sound |
| 8 | Scoop | RE-CURVE — linear amplitude where the ear wants dB (last quarter carries most of 26 dB); map exponentially like Peak gain. NAMING: wired as dip depth though named like a blend — see slot 11 |
| 9 | Scoop freq | sound — deliberately shares Peak freq's shape |
| 10 | Scoop width | RE-SPAN — inherits Peak Q's floor overreach; fix in lockstep with slot 2 |
| 11 | Scoop depth | wired as the wet/dry blend (raw, linear over correlated signals — likely fine, needs listening); NAMING swapped with slot 8 relative to the old spec text. The delta now records what the code does; renaming or rewiring is the operator's call |
| 12 | Comb/Peak | ruled this wave — sound |
| 13 | Topology | ruled this wave — sound |

Full row detail with file:line citations is in the execution report
(matrix agent, 2026-09-01); mid-curve pin inventory per knob recorded
there for future re-curves' blast radius.

## Executed after operator ratification (tasks 6-7, own preflights)

- Comb LP inversion FIXED: floor clamped to the ceiling
  (`std::min(4·combFreq, ceiling)`); above the 0.8495 threshold the knob
  goes inert at fully-open instead of reversing (measured: decreasing
  0.9947→0.9270 before, constant after; normal increasing behavior below
  the threshold preserved). Monotonicity test with break-once control.
- Q floors raised in lockstep (Peak Q and Scoop width):
  `ExpMapCompute(0.4, 10)` — Q 0.4 ≈ 3.0-octave bandwidth, launch provably
  unaffected (height 1 is an exact bypass at any Q). Floor-pinning test.
- Scoop depth cut is geometric: `ExpMapCompute(1.0, 0.05)` — measured
  −6.50515 dB per quarter knob, endpoints exact at 0 / within 2 ULP at 1.
- Comb delay de-zippered: fractional tap (PureDelay's lerp idiom, weight
  zero at integer delays — the integer-literal comb tests pass unchanged,
  bit-identical). Measured near the knob ceiling: fine-step pitch jump
  ~522× smaller than the old whole-sample jump.
- Scoop/Scoop-depth wiring SWAPPED to name-true: slot 8 Scoop = wet/dry
  blend, slot 11 Scoop depth = dip depth (with the geometric cut); labels
  positional and unmoved; persistence name-keyed; delta updated. One
  orphaned test comment corrected in passing.
- Suite at wave close: 333/333, all binaries deleted first; wasm fresh.

## Open for the operator (after this wave ships)

1. Peak gain ceiling — 2.0 / 3.0 / 4.0 with the measurement above;
   ear-gated, its harness permanent in the tree.
2. Scoop's blend-curve (slot 8, linear over correlated signals) —
   needs-listening; likely fine.
