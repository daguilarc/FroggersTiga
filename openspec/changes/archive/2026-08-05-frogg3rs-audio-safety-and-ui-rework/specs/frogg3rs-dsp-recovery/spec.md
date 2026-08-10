# Delta — `frogg3rs-dsp-recovery`

Supersedes the predecessor change's output-stage requirement. That requirement mandated a **hard
bound at full scale** and explicitly forbade "saturation, soft-knee limiting, or any other
tone-shaping". The operator reversed that decision on 2026-07-28 (*"we need feedback stability and
a limiter"*). It is rewritten here rather than left to be contradicted by the code.

## MODIFIED Requirements

### Requirement: Output is limited, then bounded
The final output stage SHALL reduce overloads by **gain reduction** rather than by truncation, and
SHALL additionally guarantee a hard ceiling at full scale.

Gain reduction alone cannot provide that guarantee. A feed-forward limiter without lookahead
computes its gain from the signal it has already passed, so its reduction always lags: a signal
whose magnitude changes faster than the attack time constant overshoots by construction. This was
measured, not assumed — a steady 1.5× tone settled at a **stable periodic peak of 1.027** with the
gain envelope converged, and the self-oscillating comb produced a raw **1.560** with the envelope
already at 0.62.

The stage SHALL therefore be a limiter **followed by** a hard bound at full scale. The bound is a
backstop for what the limiter's lag lets through, not the primary mechanism.

**This is not a reinstatement of the defect this change exists to fix.** That defect was truncating
a signal **eight times** full scale, which produces a square wave. Truncating a residual few percent
after gain reduction has already done the work is inaudible, and the limiter is what keeps the
residual small. If the backstop is engaging on anything other than rare transients, the limiter or
the upstream ranges are wrong and those are what get fixed.

*(Amended 2026-07-29. The original wording forbade truncation outright. Implementation proved that
unsatisfiable without lookahead, and lookahead was rejected because it adds latency to a live
instrument.)*

The limiter SHALL be transparent to material that never exceeds its threshold: a signal below the
threshold SHALL pass through bit-identical, with no gain applied. It SHALL NOT apply per-sample
waveshaping, because that would colour every sample it touches rather than only loud passages.

The limiter is a **safety net, not a mixing tool**. With the signal-range requirements below in
force it is expected never to engage on musical material; if it engages routinely, the ranges are
wrong and the ranges are what get fixed.

The limiter SHALL NOT be treated as a substitute for state recovery. Bounding the output does not
clear a poisoned recursive state, which continues to produce corrupt values on every later sample.

The implementation SHALL carry a comment at its definition recording that this stage does **not**
need to be internal to a future VST/plugin build — a plugin host owns final gain staging and
commonly supplies its own limiting, so in that context the stage is redundant and a candidate to
bypass or compile out.

#### Scenario: Overload is reduced by gain, then bounded
- **WHEN** the signal reaching the output stage exceeds full scale
- **THEN** gain reduction attenuates it
- **THEN** no emitted sample exceeds full scale, under any input
- **THEN** the reduction, not the bound, is what does the work

#### Scenario: The backstop is rare
- **WHEN** a randomize storm drives the instrument through many extreme patches
- **THEN** the proportion of samples reaching the hard bound is negligible
- **THEN** a backstop engaging routinely is treated as a defect in the limiter or the ranges

#### Scenario: In-range audio is untouched
- **WHEN** every sample reaching the output stage is below the limiter threshold
- **THEN** the emitted samples are bit-identical to the input samples
- **THEN** no gain reduction has been applied

#### Scenario: Existing non-finite and denormal handling is preserved
- **WHEN** a non-finite or denormal sample reaches the output stage
- **THEN** it is handled exactly as before this change

#### Scenario: The plugin-context note exists
- **WHEN** the limiter's definition is read
- **THEN** a comment records that it need not be internal to a VST/plugin build, and why

## ADDED Requirements

### Requirement: Feedback loops are stable by construction
No parameter setting reachable by any means — knob, randomize, modulation, or preset — SHALL place
a feedback loop at or above unity gain. A loop at or above unity does not decay; it drives itself to
whatever bound exists and holds there indefinitely, which is heard as the instrument seizing rather
than as a musical effect.

Stability SHALL be a property of the parameter's **range**, not of which values a randomizer
happens to draw. Constraining only the randomizer leaves the same setting reachable by hand and
creates two contradictory definitions of the parameter's range.

Where this diverges from the frozen firmware the port was derived from, the divergence is
**intended** and SHALL be recorded, and any pinned parity expectation SHALL be rewritten to match
rather than treated as a regression.

#### Scenario: Comb feedback decays
- **WHEN** the comb feedback control is set to either extreme of its range
- **THEN** the loop gain magnitude is below unity
- **THEN** the comb's output decays to silence once its input stops

#### Scenario: Randomize cannot arm a self-oscillating loop
- **WHEN** a large number of randomize operations are performed
- **THEN** no draw produces sustained output above full scale
- **THEN** the instrument remains playable after every draw

### Requirement: Stage gains are bounded to a musical maximum
No single filter stage SHALL be able to multiply the signal by an amount that turns a full-scale
input into a gross overload. Resonant gain SHALL remain large enough to be an audible effect and
small enough that its maximum, applied to the loudest legitimate input, does not exceed what the
output stage can handle without gain reduction.

#### Scenario: Peak resonance is bounded
- **WHEN** the resonant peak control is at its maximum
- **THEN** the stage's centre-frequency gain is within the documented musical ceiling
- **THEN** a full-scale input through that stage does not require limiting

### Requirement: Envelope times stay within a usable range
Envelope attack SHALL span a range whose maximum is musically useful rather than the widest value
the underlying implementation permits.

#### Scenario: Attack maximum is usable
- **WHEN** the attack control is at its maximum
- **THEN** the attack time is within the range documented as usable for this instrument

### Requirement: No unreachable defensive clamps
Guards SHALL NOT be added against conditions that cannot occur. Specifically, divisor clamps SHALL
NOT be added to the comb delay-length or resonant-bump coefficient computations: their divisors are
exponentially mapped between strictly positive endpoints, and knob values are clamped to the unit
interval before reaching them, so a zero divisor is unreachable.

This does **not** apply to a bound whose limit is reachable in normal operation — a coefficient that
can legitimately reach the edge of stability is constrained, not guarded, and that is a range
decision rather than a defensive branch.

#### Scenario: Impossible branches stay absent
- **WHEN** the filter coefficient computations are reviewed
- **THEN** no clamp guards against a zero or negative divisor
- **THEN** the reachability argument is cited where a reader would otherwise add one
