## ADDED Requirements

### Requirement: VCO morph evaluates linearly from knob

`VcoWaveMorph::GetMorph` SHALL return the modulated knob value clamped to 0–1. It SHALL NOT use `ExpParam::Compute` with `min=0`.

#### Scenario: Mid morph is audible

- **WHEN** audio is playing and VCO1 morph knob is set to 0.5
- **THEN** `ModulatedMorph(0)` is approximately 0.5
- **AND** oscillator output is a blend between sine and saw, not pure sine

#### Scenario: Full square morph

- **WHEN** VCO2 morph knob is 1.0
- **THEN** `ModulatedMorph(1)` is 1.0
- **AND** EvalWaveMorph produces square-weighted output

### Requirement: Wave cycle and randomize use mutation queue

`CycleVcoMorph` and `RandomizeVcoMorphs` initiated from desktop UI SHALL enqueue `HostMutation` events and SHALL NOT write `m_vcoMorph[]` on the message thread.

#### Scenario: Wave button click

- **WHEN** the user clicks the VCO1 wave control during Play
- **THEN** morph updates on the audio thread before the next `ProcessBlock`
- **AND** the wave icon updates on the next UI refresh

#### Scenario: Randomize waves during Play

- **WHEN** the user clicks **Randomize VCO Waveform** (or **Rand waves**) during Play
- **THEN** all three VCO morph values change to new values in 0–1
- **AND** timbre changes audibly

### Requirement: Morph mutations drain when audio is stopped

When audio is not running, pending morph mutations SHALL still be drained on the UI timer so wave icons and stored morph values update without requiring Play.

#### Scenario: Randomize while stopped

- **WHEN** audio is stopped and the user clicks **Rand waves** (or **Randomize VCO Waveform** before compact-layout rename)
- **THEN** wave icons change on the Audio panel within one timer tick
- **AND** morph knob values are updated before the user presses Play

### Requirement: Cycle advances sine saw square bands

Clicking a wave control SHALL cycle morph through low / mid / high bands corresponding to sine, saw, and square dominant regions.

#### Scenario: Three clicks cycle

- **WHEN** the user clicks the same VCO wave control three times during Play
- **THEN** morph passes through three audibly distinct timbre regions
- **AND** the painted icon shape changes each time
