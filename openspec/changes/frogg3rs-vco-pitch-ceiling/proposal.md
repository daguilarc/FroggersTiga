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
(`FroggersAppCore.hpp:1531-1540`). CORRECTED BY PREFLIGHT: the pitch knobs
carry EXPLICIT defaults — `0.2468f/0.3471f/0.4058f`
(`FroggersParameters.hpp:142`), decoding to the 110/220/330 Hz launch chord
under the current range, written verbatim at launch and Reset by
`ApplyBankDefaultPatch` (`FroggersModulation.hpp:1379-1384`). Launch is
therefore preserved by RECOMPUTING the defaults, not by the floor.

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

The three defaults are recomputed for the new range so the launch chord is
numerically preserved: `0.3087f/0.4343f/0.5077f` decode to
109.97/220.02/329.96 Hz — within 0.05 Hz of today's 109.98/220.03/330.02,
far inside the fundamental-pinning tests' Goertzel resolution. Those tests
(`kAudibleFundamentalsHz{110,220,330}`,
`FroggersAudioRoutingTests.cpp:157` and its 7 assertion sites) stay
UNCHANGED and become the drift check on the recomputation. Other knob
positions re-voice (mid-knob 632 → 316 Hz) — the accepted remap trade, with
the parity-divergence note and any mid-curve pins updated old-vs-new. The
stale range citation in the comment at `FroggersAudioRoutingTests.cpp:281`
is refreshed alongside.

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
- `app/FroggersParameters.hpp:142` (three recomputed pitch defaults) —
  propagated everywhere by `ApplyBankDefaultPatch` reading the table.
- Nothing else: filter ceilings, ring-mod range, PM LFO range, Sheaf, and
  the submodule pin are all untouched. Commits on `main`, HELD until the
  operator lifts the push hold.
- Gates: app suite fresh (324 baseline), browser wasm build, operator's
  by-ear confirmation on the deployed site after push (the pitch knobs'
  top should now stay pitched and audible), archive on confirmation.
