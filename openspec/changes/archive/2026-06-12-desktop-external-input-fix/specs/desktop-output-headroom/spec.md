## ADDED Requirements

### Requirement: Output soft limiting on desktop sim

After stereo bus processing and before DAC output, the desktop audio callback SHALL apply a soft clip/limit so samples stay within approximately ±1.0 float.

#### Scenario: Hot patch does not hard-clip DAC

- **WHEN** delay send, comb feedback, and reverb mix produce peaks above 1.0 internally
- **THEN** output buffers passed to the device are bounded without NaN
- **AND** audio continues on subsequent blocks

### Requirement: Comb state cleared on FX soft reset

`SoftResetFxState()` SHALL clear comb filter delay line and feedback state in addition to existing reverb line reset.

#### Scenario: Recovery after non-finite block

- **WHEN** a non-finite output block triggers soft reset
- **THEN** comb delay memory is zeroed
- **AND** delay stereo buffers are cleared via existing `DelayState::softResetFx`
