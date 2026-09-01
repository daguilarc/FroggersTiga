# Postflight — `frogg3rs-vco-pitch-ceiling`

Executed 2026-09-01 in the held wave; commits land with the wave's joint
push on operator approval.

## Outcomes

- `kPitchMinHz = 20` / `kPitchMaxHz = 5000` named in `Vco.hpp` beside the
  ring-mod range, read by `PitchToPhaseIncrement`; frozen-reference
  divergence note extended (reference ceiling 20000 Hz, deliberately
  departed).
- The three pitch defaults recomputed to `0.3087/0.4343/0.5077`, decoding
  to 109.97/220.02/329.96 Hz — the launch chord preserved within 0.05 Hz.
  The `kAudibleFundamentalsHz{110,220,330}` family (7 sites) untouched and
  green: the drift check the preflight demanded.
- Preflight (fresh context) REJECTED the first artifacts on a false
  premise of the author's — "launch sits at the floor" — surfacing the
  explicit chord defaults and the default-patch pin family the original
  taxonomy couldn't see. Corrected before execution.
- One calibration sensitivity surfaced at execution:
  `encoder_edit_while_frozen_changes_the_output_measurably`'s fixture
  encoded its ~632 Hz excitation as a bare 0.5 knob. Repaired by intent —
  the fixture now derives its knob from the named range for the same
  632 Hz — margin restored to the pre-change 0.101 against the 0.05
  threshold; the test's own threshold untouched.
- Range pin renamed and retargeted:
  `vco_pitch_exp_map_matches_named_range` asserts knob 0 → kPitchMinHz/sr
  bit-exact, knob 1 → kPitchMaxHz/sr, mid-knob → the geometric mean
  (316 Hz). Stale 20-kHz prose in two test files refreshed to the named
  constants.
- Enumerations: `PitchToPhaseIncrement` callers all closed out; the three
  filter 20000 ceilings and the visualizer axis comment deliberately
  untouched, dispositions recorded.

## Gates

| gate | state |
|---|---|
| `make -C app test` | 324/0 at this packet's close (later wave packets raised the count; final wave gate 327/0), all binaries deleted first |
| fundamentals family | all six dependent tests individually green |
| `make -C app/browser build` | fresh wasm |
| operator by-ear, deployed | pending the joint push: pitch knob tops land on a high pitched note; launch chord unchanged |
