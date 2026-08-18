# Delta — `froggers-sheaf-parameter-model`

**Added 2026-08-17.** Two requirements the stop investigation proved missing (`../../proposal.md` §1a,
§1b): nothing bounded an envelope ramp's duration, and nothing required a randomize draw to actually land
the drawn value.

## ADDED Requirements

### Requirement: Envelope ramps complete in bounded time at every Curve setting
Every envelope stage SHALL complete within a small fixed multiple of its knob-mapped duration at every Curve setting including the maximum, at every supported sample rate. The Curve control SHALL shape a ramp's trajectory, never its reachability: no Curve value may reduce a ramp's per-sample progress below a fixed fraction of its linear step. At Curve's zero default the ramp SHALL be bit-identical to the linear ramp.

#### Scenario: Maximum Curve still completes
- **WHEN** any stage runs at the maximum Curve setting with any knob time, at any supported sample rate
- **THEN** it completes within the fixed multiple of the knob-mapped duration
- **THEN** the observed worst-case multiple is reported by the test that guards this, not assumed

#### Scenario: A pending release is never stranded
- **WHEN** a release is pending while Grace is active and the voice is in any stage, at any Curve setting including the maximum
- **THEN** the voice reaches Release within a bounded time: the bounded completion of its remaining Attack/Decay plus the Grace minimum-hold
- **THEN** Grace's minimum-hold guarantee is preserved unchanged — a short gate still completes Attack and Decay before Release begins (audit-corrected 2026-08-17: the first draft forced Release at Grace expiry from any stage, which would have clipped legitimate notes mid-attack during play, contradicting the approved Grace requirement; transport Stop's immediate release is specified in `froggers-transport-and-reset-controls`, not here)

### Requirement: A randomize draw lands the drawn value
A randomize operation on a parameter's value SHALL result in a commanded value equal to the drawn uniform value, regardless of any live modulation on the parameter at the instant of the draw. Repeated randomize operations SHALL NOT drift the commanded value toward either clamp: the landed values' distribution follows the draw, not the modulation.

#### Scenario: Randomize under audio-rate modulation stays uniform
- **WHEN** a parameter carries full-depth audio-rate modulation and is randomized many times
- **THEN** the commanded values' empirical distribution matches the draw distribution within tolerance
- **THEN** the fraction of draws landing exactly on a clamp boundary stays consistent with the draw, not with accumulation
