# Delta — `froggers-sheaf-parameter-model`

The Reverb bank's wet-mix ceiling and its damping range are both bare literals
today. Nothing in the live specs mentions either, so a future edit could move
either one and no test would notice. Both are stated here as requirements, at
the values this change sets them to.

## ADDED Requirements

### Requirement: The reverb wet mix always leaves dry signal in the output

The Reverb bank's Wet/dry control SHALL NOT be able to remove the dry signal.
At the control's maximum, the dry signal SHALL still make up at least 40% of
this stage's output.

The ceiling SHALL be applied to the mapped mix value rather than to the knob's
range, so the control keeps sweeping its whole travel and only the value its
top end maps to is bounded.

#### Scenario: Full wet still passes dry signal

- **WHEN** the Reverb Wet/dry control is at its maximum
- **THEN** the dry signal's contribution to this stage's output is at least 40%

#### Scenario: The control keeps its full travel

- **WHEN** the Reverb Wet/dry control is swept from minimum to maximum
- **THEN** the resulting mix rises across the whole sweep, reaching its ceiling
  only at the top

### Requirement: The damping range excludes the inaudible end

The Reverb bank's Damping control SHALL map geometrically onto the damping
filter's coefficient, with a floor high enough that its darkest setting still
passes audible content.

Turning the control UP SHALL darken the tail: the control's top end maps to the
smallest coefficient, and a smaller coefficient is a lower cutoff.

The floor exists because randomization draws each parameter uniformly across its
travel. Under a geometric mapping, half of all draws land below the range's
geometric mean, so a floor an order of magnitude below audibility makes half of
every randomized reverb a tail with nothing left in its top. The floor SHALL be
chosen so that the geometric mean of the range is a cutoff that still reads as a
reverb tail rather than as mud.

#### Scenario: The darkest setting still passes audible content

- **WHEN** the Damping control is at its maximum
- **THEN** the damping filter's cutoff is above 100 Hz at a 48 kHz sample rate

#### Scenario: Up is darker

- **WHEN** the Damping control is raised
- **THEN** the damping filter's coefficient falls, and its cutoff with it

#### Scenario: The mapping is geometric across its whole travel

- **WHEN** the Damping control is at its midpoint
- **THEN** the resulting coefficient is the geometric mean of the coefficients
  at the control's two ends
