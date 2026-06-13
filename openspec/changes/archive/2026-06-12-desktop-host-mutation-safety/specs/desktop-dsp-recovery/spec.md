## ADDED Requirements

### Requirement: In-play FX soft reset

The desktop audio callback SHALL detect non-finite samples in the output block and reset FX state on the audio thread without requiring the user to restart the application.

#### Scenario: Reverb NaN cleared mid-playback

- **WHEN** reverb feedback produces a non-finite sample
- **THEN** the next block begins after `SoftResetFxState()` and output is finite (given finite input)

#### Scenario: Delay buffers cleared with engine

- **WHEN** in-play recovery runs
- **THEN** `DelayState::softResetFx()` clears delay buffers alongside engine FX reset
