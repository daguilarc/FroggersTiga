# Delta — `field-button-input-latency`

The requirements below were proposed by a change that was archived undelivered
because its targets were frozen. They are delivered here, with the measurement
that decides each one named rather than implied.

## ADDED Requirements

### Requirement: Parameter smoothing is not moved to block rate

`FroggersEngine` SHALL continue to apply parameter updates once per sample.
`RuntimeParam`'s one-pole advances one step per call with its alpha derived from
a 1 kHz natural frequency at the sample rate, so calling it once per block would
change the smoothing time by the block size.

At the Field's block size of 48 and rate of 48 kHz, a block-rate call happens at
1 kHz, and a 1 kHz cutoff cannot be expressed at a 1 kHz update rate:
`OPLowPassFilter::SetAlphaFromNatFreq` clamps at `x_maxCutoff` 0.499. Measured
time to 90% of a step is 0.375 ms per sample, 18.0 ms per block at the same
alpha, and 1.0 ms per block re-derived — the last by clamping the request away
rather than honouring it.

The separable cost was the coefficient recompute, not the smoother advance, and
it is removed by the requirement below instead.

#### Scenario: Smoothing time is unchanged by this work
- **WHEN** a parameter is stepped to a new target
- **THEN** it reaches 90% of the step in the same number of samples as before

### Requirement: Per-sample filter coefficients are computed once

`FroggersEngine` SHALL compute each output filter's coefficients at most once
per sample, and SHALL read each parameter smoother at most once per sample.

A filter whose output no sample reads SHALL NOT be configured at all.

#### Scenario: One coefficient recompute per sample
- **WHEN** a sample is processed
- **THEN** `ResonantBump::UpdateCoefficients` runs once, not six times

#### Scenario: Each smoother advances once per sample
- **WHEN** a sample is processed
- **THEN** no `RuntimeParam` has `Process()` called on it more than once

### Requirement: LED transmission is throttled

LED state SHALL be computed every poll and transmitted on change or at a bounded
rate, so that I2C traffic does not scale with poll rate.

#### Scenario: Static LEDs do not transmit every poll
- **WHEN** no LED value changes between polls
- **THEN** no transmission is issued for those polls

### Requirement: A silent reverb costs no reverb processing

On builds that HAVE a reverb page, reverb processing SHALL be skipped while the
mix rests at zero, with distinct enter and exit thresholds so a knob resting at
the boundary does not alternate between states.

Froggers Guitar has no reverb page and this requirement does not apply to it.

#### Scenario: Zero mix skips the reverb
- **WHEN** the reverb mix is at zero on a Solo build
- **THEN** reverb processing does not run for that sample

#### Scenario: A knob at the threshold does not chatter
- **WHEN** the mix sits exactly at the bypass boundary
- **THEN** the state does not alternate between processed and skipped

### Requirement: Improvement claims carry a measured quantity

A latency or freezing fix SHALL NOT be reported as delivered on the strength of
the edit alone. The controlling quantity SHALL be named, measured before and
after, and both numbers reported. Where the quantity did not move, the result is
void rather than negative.

#### Scenario: A fix with no moved number
- **WHEN** a headroom change is made and no measured quantity moves
- **THEN** the change is reported as unproven rather than as an improvement
