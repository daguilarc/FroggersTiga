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
signal that cannot exist. Where the processed path holds no signal because
nothing feeds it, the control SHALL have no effect rather than fading the
instrument toward silence.

#### Scenario: The wettest setting is still audible
- **WHEN** a wet/dry control is at its maximum and its processed path is fed
- **THEN** the output still carries dry signal

#### Scenario: An unfed processed path makes the control inert
- **WHEN** the Delay bank's Send is at zero and Wet mix is swept to maximum
- **THEN** the output is the dry signal and does not fall silent

#### Scenario: The patch the instrument ships with stays audible
- **WHEN** the default patch is played with Wet mix at maximum
- **THEN** the instrument is audible
