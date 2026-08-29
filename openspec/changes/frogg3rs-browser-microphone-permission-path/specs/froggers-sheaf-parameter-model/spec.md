# Delta — `froggers-sheaf-parameter-model`

## ADDED Requirements

### Requirement: A wet/dry control cannot remove the instrument

A control that crossfades a dry signal against a processed one SHALL NOT be able
to remove the dry signal entirely. Its mapped maximum SHALL leave dry signal
audible, capped at the mapped value rather than by shortening the knob's travel,
so the control still sweeps its whole range.

This holds for every such crossfade, not for whichever one was most recently
reported. The Reverb bank's wet mix carries this cap already; the Delay bank's
wet mix is the same expression and SHALL carry it too.

A wet/dry control SHALL NOT attenuate the dry signal in exchange for a processed
signal that cannot exist. Its authority to remove dry signal SHALL scale with
how much signal reaches the processed path: where nothing feeds that path the
control SHALL have no effect, and it SHALL earn its full travel as the path
comes to hold signal. Authority SHALL follow the processed path's measured
level rather than the amount being fed into it, so that a path made loud by
feedback rather than by its feed still grants the control its travel.

One discontinuity is permitted and required: where the feed is switched fully
off, the control SHALL become inert at once rather than following the decaying
tail the path still holds.

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

#### Scenario: Switching the feed off makes the control inert at once
- **WHEN** Send is turned to zero while echoes are still sounding
- **THEN** the control has no effect and the output is the dry signal

#### Scenario: The patch the instrument ships with stays audible
- **WHEN** the default patch is played with Wet mix at maximum
- **THEN** the instrument is audible
