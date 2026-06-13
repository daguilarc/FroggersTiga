# web-knob-column-stack Specification

## Purpose
TBD - created by archiving change web-knob-col-mobile-align. Update Purpose after archive.
## Requirements
### Requirement: Knob column vertical stack template

Every `.knob-col` on the web sim SHALL use a single vertical stack layout on all viewports: parameter label, optional hint, optional VCO morph slot, rotary knob, mod-source label, mod `<select>`. Each child SHALL be horizontally centered within the bordered cell. The template SHALL NOT use a horizontal knob+morph row.

#### Scenario: Non-VCO column stack

- **WHEN** the user views any knob column without a VCO morph (columns 3–7 on Audio, or any column on other pages)
- **THEN** the column shows label, hint slot, centered knob, mod label, and mod select in vertical order
- **AND** the knob is centered on the cell's horizontal midline

#### Scenario: VCO column stack on Audio

- **WHEN** the user views VCO1–VCO3 columns on the Audio page
- **THEN** the waveform morph control appears between the parameter label and the rotary knob
- **AND** the knob remains centered below the morph control

#### Scenario: Mobile no cross-column overlap

- **WHEN** the viewport is at most 720px wide and the user views the Audio page
- **THEN** VCO morph buttons do not overlap or cover knobs in adjacent columns
- **AND** each morph button is fully visible inside its own bordered cell

#### Scenario: Uniform morph row on Audio page

- **WHEN** the user views the Audio page
- **THEN** all eight knob columns reserve the same vertical space for the morph row
- **AND** columns 4–8 show an invisible placeholder where VCO columns show the waveform button

#### Scenario: Desktop four-column grid preserved

- **WHEN** the viewport is wider than 720px
- **THEN** the knob field remains a four-column grid with the same outer alignment as mod bay and page chrome
- **AND** intra-column stack does not reintroduce page-nav arrows

