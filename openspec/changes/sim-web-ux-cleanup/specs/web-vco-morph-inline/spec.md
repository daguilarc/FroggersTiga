## MODIFIED Requirements

### Requirement: VCO morph buttons live in VCO knob columns

On the Audio host page, VCO1–VCO3 knob columns SHALL display a clickable waveform morph control immediately to the right of the rotary knob. The control SHALL use the same waveform SVG styling (blue stroke, 28×28). Clicking SHALL send `cycleVcoMorph` for that VCO index after the engine is ready.

#### Scenario: Audio page layout

- **WHEN** the user views host page Audio (index 0)
- **THEN** columns 0–2 show knob + waveform button in one horizontal row
- **AND** columns 3–7 show only the rotary knob (no morph button)

#### Scenario: Morph updates from engine

- **WHEN** a `screen` message arrives with `morphs[0..2]` after Rand waveforms, Marbles, or morph cycle
- **THEN** each inline waveform button SVG reflects the new morph value

#### Scenario: Morph click requires engine

- **WHEN** the user clicks a VCO morph button before the engine is ready
- **THEN** status prompts to click Play first
- **AND** no worklet message is sent

#### Scenario: Morph click updates SVG immediately

- **WHEN** the user clicks a VCO morph button while playing on the Audio page
- **THEN** the waveform SVG updates within the same animation frame (optimistic local cycle)
- **AND** the next `screen` message reconciles morph values from WASM

### Requirement: Rand waveforms affects inline morph buttons

The global **Rand waveforms** control SHALL randomize VCO morphs in WASM and the inline VCO1–VCO3 waveform buttons SHALL update on the next `screen` message.

#### Scenario: Rand waveforms on Audio page

- **WHEN** the user clicks Rand waveforms on the Audio page
- **THEN** all three inline waveform buttons change shape within one screen tick
