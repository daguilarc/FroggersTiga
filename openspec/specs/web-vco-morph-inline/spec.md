# web-vco-morph-inline Specification

## Purpose
TBD - created by archiving change web-knob-column-cleanup. Update Purpose after archive.
## Requirements
### Requirement: VCO morph buttons live in VCO knob columns

On the Audio host page when WASM page index is 0, VCO1–VCO3 knob columns SHALL display a clickable waveform morph control immediately to the right of the rotary knob. The control SHALL use the same waveform SVG styling as the former OLED wave buttons (blue stroke, 28×28). Clicking SHALL send `cycleVcoMorph` for that VCO index.

#### Scenario: Audio page layout

- **WHEN** the user views host page Audio
- **THEN** columns 0–2 show knob + waveform button in one horizontal row
- **AND** columns 3–7 show only the rotary knob (no morph button)

#### Scenario: Morph updates from engine

- **WHEN** a `screen` message arrives with `morphs[0..2]` after Rand waves, Marbles, or morph cycle
- **THEN** each inline waveform button SVG reflects the new morph value
- **AND** the update does not require the OLED panel or Play-only gate

#### Scenario: Non-Audio pages

- **WHEN** the user navigates to Marbles, Reverb, Filter, Drive, or Delay host page
- **THEN** VCO morph buttons are hidden or absent
- **AND** knob columns otherwise unchanged

#### Scenario: Mobile viewport

- **WHEN** the user views the Audio page on a viewport ≤720 px wide
- **THEN** VCO1–VCO3 columns still show knob and inline waveform button without wrapping below the field
- **AND** no OLED or route-summary panel appears

### Requirement: Rand waves affects inline morph buttons

The global **Rand waves** control SHALL randomize VCO morphs in WASM and the inline VCO1–VCO3 waveform buttons SHALL update on the next `screen` message, matching noriegas randomize-to-visible-controls behavior.

#### Scenario: Rand waves on Audio page

- **WHEN** the user clicks Rand waves on the Audio page
- **THEN** all three inline waveform buttons change shape within one screen tick
- **AND** no OLED or route-summary panel appears

