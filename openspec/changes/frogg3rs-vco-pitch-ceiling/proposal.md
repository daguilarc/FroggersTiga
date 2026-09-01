# Proposal — `frogg3rs-vco-pitch-ceiling`

**Created 2026-09-01, operator-approved.** The three VCO pitch knobs spend
roughly their top third on fundamentals nobody can use: the ceiling is the
textbook 20 kHz audibility limit, which is a display-axis number, not an
oscillator-pitch number. The range's literals are also unnamed and repeated,
which is how the ceiling survived every sweep — no change ever named the
concept, so nothing enumerated it.

## The mechanism, traced

`dsp::Vco::PitchToPhaseIncrement` (`app/dsp/Vco.hpp:164-167`):

    ExpMapCompute(20.0f / sampleRate, 20000.0f / sampleRate, pitchKnob01)

Exponential, floor 20 Hz, ceiling 20 000 Hz, both inline literals. All three
Audio-bank pitch knobs (slots 0-2) route here per sample
(`FroggersAppCore.hpp`, RouteAudioSample's VCO loop). The pitch knobs carry
no explicit default, so launch sits at knob 0 = the 20 Hz floor.

The `20000` family, enumerated and classified:

| site | purpose | disposition |
|---|---|---|
| `Vco.hpp:166` | VCO pitch ceiling | CHANGES — this proposal |
| `FroggersAppCore.hpp:1744` | Peak freq ceiling | keep — a filter opened fully out of the way is the point |
| `FroggersAppCore.hpp:1806` | Scoop freq ceiling | keep — same |
| `FroggersAppCore.hpp:1831` | Comb LP ceiling | keep — same |
| `FroggersTransferFunctionVisualizer.hpp:37` (comment) | display-axis span | verify at execution; a plot axis spanning the audible spectrum is correct and unrelated to pitch |

The in-file precedent for doing it right is the ring-mod carrier:
`kRingModMinHz = 20 / kRingModMaxHz = 5000` (`Vco.hpp:132-133`), with a
comment deliberately refusing pitch's 20/20000.

## The change

Name the range and drop the ceiling:

- `static constexpr float kPitchMinHz = 20.0f;`
- `static constexpr float kPitchMaxHz = 5000.0f;`

declared beside `kRingModMinHz/MaxHz`, read by `PitchToPhaseIncrement`.
5000 matches the ring-mod ceiling precedent and clears the top of the piano
(C8 ≈ 4186 Hz) with headroom, returning about two octaves of the knob to
pitches that are actually musical.

Consequences, stated: launch is untouched (the floor does not move and the
pitch knobs default to it); every knob position above the floor re-voices
lower — mid-knob moves from √(20·20000) ≈ 632 Hz to √(20·5000) ≈ 316 Hz —
the same accepted remap trade as the comb feedback curve, and it gets the
same treatment: a parity-divergence note against the frozen reference, and
every test pinning a mid-curve pitch value updated with old vs new stated.

## Enumeration duties for execution

- Every caller of `PitchToPhaseIncrement`, FOUND vs CHANGED, zeros included
  (expected: the per-sample pitch path and any tests naming it).
- Every test pinning an absolute frequency produced by a pitch knob
  position (parity suite Vco cases, routing tests asserting pitch-derived
  values): mid-curve pins change with old vs new named; the knob-0 floor
  pins must NOT move.
- The frozen-reference divergence note: the port's pitch range now
  deliberately diverges from `src/core`'s; extend the file's existing
  divergence-note idiom, do not weaken parity elsewhere.
- The visualizer :37 comment's 20/20000 reference: read what it actually
  describes and disposition it (expected unchanged as a display axis).

## Impact

- `app/dsp/Vco.hpp` (constants + `PitchToPhaseIncrement` + comment).
- Tests pinning mid-curve pitch values, per enumeration.
- Nothing else: filter ceilings, ring-mod range, PM LFO range, defaults,
  Sheaf, and the submodule pin are all untouched. Commits on `main`.
- Gates: app suite fresh (324 baseline), browser wasm build, operator's
  by-ear confirmation on the deployed site after push (the pitch knobs'
  top should now stay pitched and audible), archive on confirmation.
