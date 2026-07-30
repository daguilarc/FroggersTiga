# Delta — `frogg3rs-dsp-recovery`

## ADDED Requirements

### Requirement: Every stateful DSP unit can reset its own recursive state
Each stateful unit in the audio path SHALL expose a reset operation that returns its own
recursive state to a silent, valid starting condition without disturbing any other unit. Reset
SHALL be per-unit; no operation SHALL reset the whole chain in response to a single unit's fault,
because doing so would audibly cut reverb tails and delay repeats unrelated to the fault.

#### Scenario: Every unit in the audio path exposes reset
- **WHEN** the set of stateful DSP units in the audio path is enumerated
- **THEN** each one exposes a reset operation
- **THEN** invoking it clears only that unit's recursive state

#### Scenario: Reset is isolated
- **WHEN** one unit's state is reset
- **THEN** every other unit's state is unchanged
- **THEN** the reverb tail and delay repeats continue uninterrupted

### Requirement: Non-finite state recovers automatically
When any unit's recursive state becomes non-finite, that unit SHALL be reset so audio resumes.
Masking a non-finite value at the output SHALL NOT be treated as recovery: the poisoned state
continues to produce corrupt values on every subsequent sample, so output masking alone yields
permanent silence with no recovery path.

#### Scenario: Poisoned state recovers rather than latching
- **WHEN** a non-finite value enters any unit's recursive state
- **THEN** that unit is reset
- **THEN** audible output resumes within a bounded number of blocks
- **THEN** output does not remain silent indefinitely

### Requirement: Sustained excessive magnitude recovers automatically
Recovery SHALL NOT be conditioned on non-finiteness alone. When a unit's state magnitude exceeds
its configured ceiling for a sustained window, that unit SHALL be reset even though its state
remains finite.

**Rationale.** The failure this requirement exists for was observed to drive the filter stage past
the output clamp to a magnitude of approximately 8.7, with repeated multi-second clipping bursts,
while remaining finite the entire time. A finiteness-only guard does not fire on that case at all.
That is the whole argument, and it survives whatever the exact magnitude turns out to be: the
observation needed is only that the state went large and stayed finite.

Those particular numbers are **indicative, not authoritative** — they came from a reproduction
that drove a *shadow copy* of the audio chain rather than the real engine (the same methodological
error this change exists to stop repeating; see the proposal's "Known-unfixed" section). They are
therefore not a measurement this requirement rests on, and they are explicitly **not** the source
of the ceiling: the sibling requirement below fixes the ceiling by derivation from the chain's
signal bounds, precisely so that no threshold in this spec depends on a sampled observation. The
real-engine reproduction is a separate deliverable, and it may legitimately report that the
failure does not reproduce — which would not weaken this requirement, since a finiteness-only
guard would still be an incomplete guard.

#### Scenario: Finite blowup recovers
- **WHEN** a randomize action is followed by a parameter sweep that drives a stage into sustained
  large-magnitude oscillation
- **AND** no value in that state is ever non-finite
- **THEN** the offending unit is reset
- **THEN** output returns to a sane magnitude and remains there

#### Scenario: Legitimately loud audio is not reset
- **WHEN** a deliberately loud but musically valid patch is played
- **THEN** no unit is reset
- **THEN** the output is unaltered by the recovery mechanism

### Requirement: Recovery thresholds are derived from the signal bounds
The magnitude ceiling SHALL be derived from the chain's own documented signal bounds — the
saturator that bounds the filter input, and the maximum gain the filter stages can apply — rather
than from a sample of observed values. A threshold taken from measurement is only as
representative as the patches that happened to be tested and cannot be shown correct.

The derivation SHALL be recorded beside the constant, so the ceiling cannot be re-tuned later
without re-deriving it.

#### Scenario: The ceiling is justified from bounds, not samples
- **WHEN** the magnitude ceiling is set
- **THEN** its derivation from the input bound and the maximum stage gain is recorded
- **THEN** the margin between the maximum legitimate magnitude and the ceiling is stated

### Requirement: Output never exceeds full scale
The final output stage SHALL NOT emit samples beyond full scale. Emitting above full scale
delegates clipping to the audio device, which clips at a threshold many times below the emitted
magnitude and converts any overload into square-wave distortion, while providing no protection to
the listener.

The limit SHALL be a hard bound at full scale. The output stage SHALL NOT apply saturation,
soft-knee limiting, or any other tone-shaping: the instrument's timbre is defined by its ported
signal chain, and the output guard SHALL remain tonally transparent for all in-range material.

#### Scenario: Overload is bounded at full scale
- **WHEN** the signal reaching the output stage exceeds full scale
- **THEN** the emitted sample is bounded to full scale

#### Scenario: In-range audio passes through unaltered
- **WHEN** the signal reaching the output stage is within full scale
- **THEN** the emitted sample is bit-identical to the input sample
- **THEN** no saturation or tonal shaping has been applied

#### Scenario: Existing non-finite and denormal handling is preserved
- **WHEN** a non-finite or denormal sample reaches the output stage
- **THEN** it is handled exactly as before this change

### Requirement: No unreachable defensive clamps
Guards SHALL NOT be added against conditions that cannot occur. Specifically, divisor clamps
SHALL NOT be added to the comb delay-length or resonant-bump coefficient computations: their
divisors are exponentially mapped between strictly positive endpoints, and knob values are
clamped to the unit interval before reaching them, so a zero divisor is unreachable.

#### Scenario: Impossible branches stay absent
- **WHEN** the filter coefficient computations are reviewed
- **THEN** no clamp guards against a zero or negative divisor
- **THEN** the reachability argument is cited where a reader would otherwise add one
