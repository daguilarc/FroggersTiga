# Tasks — `frogg3rs-comb-knob-travel`

## 0. Tree and sweep

`git status --short -- app External/Sheaf` and the submodule's own status
both clean before any evidence run. Hygiene sweep covers `app/dsp/` and
`app/` test files touched; name what was swept.

## 1. The feedback curve

In `app/dsp/FilterFx.hpp`, `Comb::GetFeedback` (~:525): each half's
magnitude becomes `1 − ExpMapCompute(1.0f, 1.0f − kMaxFeedbackMagnitude, t)`
with t and the bipolar sign convention unchanged. The long decay-behaviour
comment above it stays; add to it only the curve's meaning — the feedback
gap falls geometrically so equal knob steps multiply ring time by equal
ratios, and both rails and the center are numerically identical to the old
curve. No change/task references in comments.

## 2. The blend crossfade

Same file, `FilterFxChain::Process` (~:771): the linear mix becomes
equal-power, `peakPath·cos(blend·π/2) + combPath·sin(blend·π/2)`, using the
file's existing math includes/idiom. The method's header comment gains one
sentence saying the blend is equal-power and both extremes are exactly
single-path.

## 3. Enumerations, closed out in the same breath

- Every `GetFeedback` caller: FOUND vs CHANGED, zeros included. The
  visualizer reads `comb.feedback` state (`FilterFx.hpp:440`) — verify it
  therefore tracks the new curve with no edit, by reading its input path,
  and say so with the line read.
- Every `ZeroedExpCompute` caller after the change, both trees' app-reachable
  code. If `GetFeedback` was its only user, report that and leave the helper
  in place only if another bank still calls it; otherwise flag it.
- Every test naming `GetFeedback`, comb feedback values, or the blend mix:
  feedback rails/center and blend=0 are bit-identical by construction and
  their pins must NOT move; blend=1 holds only to REQUIRE_NEAR tolerance
  (float cos(π/2) residual); mid-curve pins change, each named with old vs
  new value — the two known ones are
  `comb_get_delay_samples_and_asymmetric_feedback` and
  `filter_fx_chain_parallel_matches_manual_comb_peak_scoop_blend`. The parity suite's deliberate-divergence note for `GetFeedback`
  is extended, not weakened.

## 4. Gates

- `rm -f app/build/froggers_audio_routing_tests` then `nice make -C app
  test` — full count (322 baseline), zero FAIL.
- `rm` the wasm artifact, `nice make -C app/browser build` — fresh mtime.
- A `TEST_CASE` proves the new feedback curve's property: for a few equal
  knob steps on one half, the measured ring-time ratios are equal within
  tolerance, and the old curve's values at rails/center reproduce exactly.
  Prove it can fail by breaking the curve once (binary deleted between
  runs), then restore. No shell harnesses.

## 5. Ship and confirm

Commit on `main`, push. OPERATOR, on the deployed site, by ear: the
feedback knob is interesting across its whole travel in both directions
from center; the comb becomes clearly present by mid-blend; launch sound
unchanged. Archive on the operator's confirmation.
