# Delta — `froggers-sheaf-parameter-model`

## RENAMED Requirements

- FROM: `### Requirement: The reverb wet mix always leaves dry signal in the output`
- TO: `### Requirement: A wet/dry control cannot remove the instrument`

## MODIFIED Requirements

### Requirement: A wet/dry control cannot remove the instrument

A control that crossfades a dry signal against a processed one SHALL NOT be able
to remove the dry signal entirely. The ceiling SHALL be applied to the mapped
mix value rather than to the knob's range, so the control keeps sweeping its
whole travel and only the value its top end maps to is bounded.

This holds for every such crossfade, not for whichever one was most recently
reported. At the Reverb bank's maximum the dry signal SHALL still make up at
least 40% of that stage's output. The Delay bank's wet mix is the same
expression and SHALL carry the same kind of ceiling.

A wet/dry control SHALL NOT attenuate the dry signal in exchange for a processed
signal that cannot exist. Its authority to remove dry signal SHALL scale with
how much signal reaches the processed path: where nothing feeds that path the
control SHALL have no effect, and it SHALL earn its full travel as the path
comes to hold signal. Authority SHALL follow the processed path's measured
level rather than the amount being fed into it, so that a path made loud by
feedback rather than by its feed still grants the control its travel.

Where the feed is switched fully off, the measurement's TARGET SHALL drop at
once rather than tracking the tail the path still holds. The measurement itself
SHALL settle at its ordinary release rate, so the audible result is a fade of
about 100ms rather than a cut. The discontinuity belongs in the target, not in
the output.

#### Scenario: Full wet still passes dry signal
- **WHEN** the Reverb Wet/dry control is at its maximum
- **THEN** the dry signal's contribution to this stage's output is at least 40%

#### Scenario: The control keeps its full travel
- **WHEN** the Reverb Wet/dry control is swept from minimum to maximum
- **THEN** the resulting mix rises across the whole sweep, reaching its ceiling
  only at the top

#### Scenario: The wettest setting is still audible
- **WHEN** a wet/dry control is at its maximum and its processed path is fed
- **THEN** the output still carries dry signal

#### Scenario: An unfed processed path makes the control inert
- **WHEN** the Delay bank's Send is at zero and Wet mix is swept to maximum
- **THEN** the output is the dry signal and does not fall silent

#### Scenario: A loud echo earns the control its travel
- **WHEN** the Delay bank's Send is low, Feedback is high, and the echo is loud
- **AND** Wet mix is at maximum
- **THEN** the control removes dry signal in proportion to that echo, not to Send

#### Scenario: Switching the feed off fades rather than tracking the tail
- **WHEN** Send is turned to zero while echoes are still sounding
- **THEN** the control's authority falls at its release rate rather than
  following the echoes' own decay
- **AND** the output settles to the dry signal

#### Scenario: The patch the instrument ships with stays audible
- **WHEN** the default patch is played with Wet mix at maximum
- **THEN** the instrument is audible

## ADDED Requirements

### Requirement: The signal is not folded to mono before it reaches the device

The delay and reverb stages SHALL carry their stereo pairs to the output rather
than summing them mid-chain. Folding to a single channel SHALL happen at the
output, and only where the device itself is mono.

Both stages already compute a stereo pair internally. A stage that computes a
pair and sums it on the next line spends the work and discards the result, and
it renders every control downstream of the sum unable to affect the output.

A control named for a stereo property SHALL be able to change the output. Where
a Width control's effect cancels exactly in a sum, the sum is the defect, not
the control.

#### Scenario: A stereo device receives a stereo image
- **WHEN** the host offers two or more output channels and a Width control is
  away from its centre
- **THEN** the two channels differ

#### Scenario: The Reverb Width control changes the output
- **WHEN** the Reverb bank's Width is swept
- **THEN** the output changes

#### Scenario: The Delay Stereo width control changes the output
- **WHEN** the Delay bank's Stereo width is swept
- **THEN** the output changes
