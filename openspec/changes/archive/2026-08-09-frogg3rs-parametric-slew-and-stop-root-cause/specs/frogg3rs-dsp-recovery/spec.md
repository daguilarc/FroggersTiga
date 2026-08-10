# Delta — `frogg3rs-dsp-recovery`

Two sibling live changes already shaped this capability (stateful-unit reset, non-finite/magnitude
recovery, feedback-loop range stability, output limiting). This delta adds two further, distinct
facts this change measured and fixed — one at the signal source (`DigitalReorganizer`), one in the
one recursive loop that had no in-loop saturator (`dsp::Reverb`) — traced end-to-end in
`proposal.md` §2.

## ADDED Requirements

### Requirement: The drive stage maps silence to silence
`DigitalReorganizer` (the Drive bank's Flip/Bit-depth bit-mangling stage, `app/dsp/Drive.hpp`) SHALL
return exactly `0.0f` for a `0.0f` input, at every `flip`/`hashBits` setting reachable from the Flip
and Bit-depth (Hash) knobs. It SHALL do so by subtracting its own silent-input response from its
mangled output — `Mangle(input, flip, hashBits) - Mangle(0.0f, flip, hashBits)`, where `Mangle` is
the pre-existing bit-scramble factored into its own static helper — computed fresh on every sample
rather than cached, because `flip`/`hashBits` are public fields reassignable outside
`SetFlip()`/`SetHash()`, and a setter-cached correction would go stale the moment either field is
assigned directly.

This is a **deliberate, documented parity divergence** from the frozen firmware this stage was
ported from (`src/core/PolynomialDrive.hpp`), which has the same `f(0) != 0` bit-scramble property
and relied on an analog output stage's AC coupling — absent from this port — to remove the resulting
DC offset for free. It is the same class of divergence as `dsp::Comb::GetFeedback`'s `±1.1 → ±0.95`
and the resonant-peak ceiling's `10× → 4× → 2×`, and it carries the same kind of in-code note. The
frozen `src/core/PolynomialDrive.hpp` SHALL NOT be edited to match; the divergence lives only in this
port.

At the pass-through configuration (`flip == 0, hashBits == 0`) the correction term is exactly zero,
so this requirement changes nothing observable there: `Process(1.0f) == 1.0f` continues to hold, and
every parity case recorded at default Flip/Hash stays green.

#### Scenario: Zero input yields zero output at any Flip/Hash setting
- **WHEN** `DigitalReorganizer::Process(0.0f)` is called at any reachable `flip`/`hashBits` pair
- **THEN** it returns exactly `0.0f`

#### Scenario: Pass-through configuration is unaffected
- **WHEN** `flip == 0` and `hashBits == 0`
- **THEN** `Process(input)` reconstructs `input` exactly, unchanged from before this fix

#### Scenario: A fully silent, fully static chain decays to and stays at zero
- **WHEN** the transport is stopped, every voice is `Idle`, all stateful units are `Reset()`, and
  Drive Flip/Blend are held at values that previously seeded a nonzero constant (`flip == 128`,
  Blend > 0), with no modulation active anywhere
- **THEN** the output peak measured after the flush is exactly `0.0` at every checkpoint, rather than
  climbing toward the output stage's ceiling over roughly half a second and pinning there
- **THEN** the same rig with Flip forced to `0`, and separately with Blend forced to `0`, also
  measures exactly `0.0` — positive controls proving the rig could have detected the nonzero case
  (`app/FroggersStopFlushRepro.cpp`, the S1.2 case, `RunF3SilentChainCase`)

#### Scenario: The divergence is documented and its own parity cases are re-asserted, not deleted
- **WHEN** `FroggersDspParityTests.cpp`'s `DigitalReorganizer` cases run at nonzero flip/hash
- **THEN** they are asserted against the corrected formula, not the frozen firmware's raw formula
- **THEN** the code carries an in-line note recording the divergence and why

### Requirement: Recursive loops saturate their own feedback, not only their coefficient range
Every stateful feedback loop in the signal chain (`dsp::Comb`, `dsp::StereoDelay`, `dsp::Reverb`) SHALL
bound the value it feeds back into its own delay line with an in-loop saturator
(`dsp::PadeSaturator::Saturate`), in addition to whatever range its feedback coefficient is clamped
to. A coefficient clamped well below unity (comb's `±0.95`, delay's `≤0.98`) and a coefficient
designed to approach unity for a musically long tail (reverb's `fb`, up to ≈`0.99998` at maximum
Hold) both still need it: the saturator is what keeps a sustained overdrive bounded near
`|input| + fb`, instead of climbing toward the unbounded `input / (1 − fb)` a purely linear loop
would reach.

`dsp::Reverb` was the one loop of the three without this. Its tank wrote `preOut + aFb * fb` /
`preOut + bFb * fb` straight into `lineA`/`lineB`, unsaturated, so any sustained overdrive reaching
it — regardless of cause — grew toward the output stage's ceiling, invisibly to the
`dsp::FiniteOnly` recovery tier (finite the whole time, never non-finite, so nothing in the recovery
system ever fired). It now saturates the feedback tap the same way `dsp::StereoDelay` already does
(`fbk * PadeSaturator::Saturate(fbL)`): `preOut + fb * PadeSaturator::Saturate(aFb)` /
`preOut + fb * PadeSaturator::Saturate(bFb)`, reusing the same saturator type in the same in-loop
position — a deliberate mirror of that existing fix, not a new mechanism.

#### Scenario: Reverb tank bounds a sustained overdrive
- **WHEN** the reverb tank receives a sustained, full-scale excitation with Hold and Decay at their
  maximum
- **THEN** the tank's internal magnitude is bounded at approximately `|input| + fb`
- **THEN** an otherwise-identical unsaturated replica of the same tank, run in the same pass, reaches
  many multiples of that bound, demonstrating the saturator is what is doing the work

#### Scenario: Ordinary reverb tails are essentially untouched
- **WHEN** the reverb is excited at ordinary, non-overdriven levels
- **THEN** the tail's measured retention against an unsaturated replica differs by only a few percent
  absolute

#### Scenario: A loud, Hold-pinned tail decays audibly rather than riding the limiter
- **WHEN** Hold and Decay are both at maximum and the tank has been driven hard, then the input stops
- **THEN** the output declines gradually across multiple checkpoints rather than staying pinned at the
  post-burst peak
- **THEN** an unsaturated replica run in lockstep on the same signal is asserted to stay pinned
  (`>0.9×` retention), so the comparison is demonstrated to be meaningful rather than assumed
  (`FroggersDspParityTests.cpp`'s
  `reverb_hold_at_max_tail_stays_audible_and_decays_gradually_not_pinned`)
