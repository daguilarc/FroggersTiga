# Delta — `froggers-vco-topology`

**Added 2026-08-20 by operator instruction.** The envelope is the only part
of the instrument whose controls map linearly; every other time and frequency
control is exponential. Several control bounds also sit where the control has
no audible effect, which randomization visits far more often than a hand
turning a knob does.

## ADDED Requirements

### Requirement: Envelope times map exponentially
THE attack, decay and release controls SHALL map their position to time
exponentially, matching every other time and frequency control in the
instrument, so that equal movements of the control produce equal ratios of
time rather than equal differences. Their minimum positions SHALL sit at a
short but non-zero time, so that a control at minimum is fast rather than
instantaneous. The Grace control SHALL remain linear from zero, because no
minimum hold is a real setting that an exponential mapping cannot reach.

#### Scenario: Equal movements give equal ratios
- **WHEN** the attack control is moved from its minimum to its midpoint, and
  again from its midpoint to its maximum
- **THEN** each movement multiplies the attack time by the same ratio

#### Scenario: A randomized envelope keeps its transient
- **WHEN** envelope times are randomized repeatedly
- **THEN** most draws produce a fast attack, and long attacks are the
  minority rather than half the draws

#### Scenario: Grace still reaches zero
- **WHEN** the Grace control is at its minimum
- **THEN** there is no minimum hold at all

### Requirement: Control bounds stay inside the useful range
EVERY control's bounds SHALL sit where the control still does something
audible. A control at either extreme SHALL produce a usable setting rather
than a silent or inert one, so that randomizing a control explores its
character instead of disabling it. Controls whose zero position is a real
setting — phase-modulation depth, ring-modulation depth, peak gain, fold,
scoop depth and Grace — SHALL keep reaching true zero; turning an effect off
belongs to those controls alone.

#### Scenario: A randomized patch stays audible
- **WHEN** every parameter is randomized
- **THEN** the instrument sounds at a usable level, without a sustain so low
  or an envelope so slow that notes disappear

#### Scenario: A randomized effect is audibly present
- **WHEN** the resonant peak, the scoop or the comb has its frequency
  randomized
- **THEN** that effect lands within the audible range and is heard working,
  rather than sitting below hearing where it does nothing

#### Scenario: Off is still reachable where off is meaningful
- **WHEN** phase-modulation depth, ring-modulation depth, peak gain, fold or
  scoop depth is at its minimum
- **THEN** that effect contributes nothing at all
