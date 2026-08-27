# Proposal — `frogg3rs-drive-tone-floor`

**Created 2026-08-26.** The Drive bank's Tone control spends most of its travel
below anything a randomized patch can use. Same mechanism as
`frogg3rs-reverb-wetness-and-damping-floor`'s damping floor, a bank away, and
the operator named it: "another culprit i think is Tone in the Drive page".

## Traced

`app/dsp/Drive.hpp:456` is the whole mapping:

    void SetTone(float toneKnob01) { tone.alpha = ExpMapCompute(0.02f, 1.0f, toneKnob01); }

The result is the one-pole's own coefficient (`app/dsp/DspMath.hpp:83-87`,
`out = alpha*in + (1-alpha)*out`), so a smaller alpha is a darker signal, and
`alpha == 1` at the knob's top is an exact identity — Tone's default is fully
open. It is the last stage inside `FrogBlock::Process`
(`app/dsp/Drive.hpp:485`), so it shapes the WET path, which Blend then
crossfades against the dry input. The knob reaches it unmodified from Drive slot
12 (`app/FroggersAppCore.hpp:1634`).

Randomization draws every parameter uniformly across its travel
(`External/Sheaf/projects/synth/src/ParameterModulation.cpp:2893,3595-3599`,
`std::generate_canonical`), and this mapping is geometric over a 50x range, so
the travel is spent like this — cutoffs at 48 kHz, from
`fc = -fs * ln(1 - alpha) / 2pi`:

| knob | alpha | cutoff |
|---|---|---|
| 0.00 | 0.020 | 154 Hz |
| 0.25 | 0.053 | 418 Hz |
| 0.50 | 0.141 | 1165 Hz |
| 0.75 | 0.376 | 3604 Hz |
| 1.00 | 1.000 | bypass |

Half of all draws put a low-pass below 1165 Hz on the driven signal, and a
quarter put it below 418 Hz. Blend scales how much of that reaches the output,
and Blend is drawn uniformly too, so the average randomized patch has about half
its drive path behind a filter near 1.2 kHz.

154 Hz is not a tone control. A tone control at the end of a distortion stage
sweeps something like 500 Hz to 8 kHz; below that it is a mute with extra steps.

Raising the floor to 0.1 makes the range one decade: darkest 805 Hz, and half of
draws now land above 2.9 kHz. The knob keeps its full travel, its direction, and
its exact-identity default. 0.05 (darkest 392 Hz, median 1934 Hz) is the more
conservative choice if 805 Hz turns out to have taken away a dark setting worth
keeping.

## The twin, which this change deliberately does not touch

Enumerating the concept by its operand rather than its name finds three
one-pole-alpha-from-knob mappings, and two share this exact range:

| control | mapping | darkest |
|---|---|---|
| Drive **Tone** (slot 12) | `ExpMapCompute(0.02, 1.0, knob)` | 154 Hz |
| Delay **Feedback tone** (slot 10) | `ExpMapCompute(0.02, 1.0, knob)` | 154 Hz |
| Reverb **Damping** (slot 4) | `ExpMapCompute(0.02, 0.2, 1 - knob)` | 154 Hz, already raised |

Feedback tone (`app/dsp/Delay.hpp:730`) has the identical numbers, so the same
arithmetic says the same thing about it. Its musical role is not the same: it
sits INSIDE the feedback loop, so it darkens successive repeats rather than the
through-signal, and repeats that get progressively darker is what a delay is
supposed to do. Whether its floor is a defect or the effect is an operator call,
not one to make from the arithmetic alone, so it is listed here and left alone.
It is the same one-line edit if the answer is yes.

## Not a refactor

The repeated `0.02f` is a coincidence of value across three different ranges,
not a shared concept that needs a name. All three already route through one
mapping function, so the duplication §7 cares about — the same computation
written twice — does not exist at the call graph. A shared floor constant would
actively be wrong: damping's ceiling is 0.2 and Tone's is 1.0, so one floor
cannot be right for both, and pinning them together would undo the reverb
change's own range.

## What Changes

- `SetTone`'s lower bound 0.02f -> 0.1f.
- A requirement for it, which it does not have: nothing in `openspec/specs/`
  mentions Tone's range, and no test pins it either — the two existing tests
  that call `SetTone` both pass 1.0f, the bypass default, so the floor is
  currently unasserted at both ends.
- `MANUAL.md` and `QUICK_DICT.md` gain the actual limits, which neither states.

## Non-goals

- Delay Feedback tone, pending the operator call above.
- Reverb Damping, already done.
- The knob's range, direction or default. Only what the dark end maps onto.
- The randomize draw. It stays uniform; what changes is what uniform lands on.

## Impact

- Affected specs: `froggers-sheaf-parameter-model`.
- Three changes are active. This one and
  `frogg3rs-reverb-wetness-and-damping-floor` are the same idea in two banks,
  and are separate changes rather than one because that one is landed and
  waiting only on operator ears; they share no file.
  `frogg3rs-runtime-pages-beside-the-sliders` is unrelated and also waiting on
  an operator step.
- A randomized patch stops arriving with its drive path behind a 1 kHz wall half
  the time.
