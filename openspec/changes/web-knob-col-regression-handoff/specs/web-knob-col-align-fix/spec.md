## ADDED Requirements

### Requirement: Knob column content stays inside bordered cell

Every `.knob-col` on the web sim SHALL render all child content (label, morph slot, hint, knob, mod label, mod select) within the visible border and background of the cell on viewports 375px and 960px wide. No child SHALL visually extend outside the cell's padding box in a way that appears detached from the box.

#### Scenario: Mobile cell containment

- **WHEN** the viewport width is 375px and the user views any host page
- **THEN** each knob column's label, knob, and mod controls align within that column's bordered rectangle
- **AND** no control appears consistently offset to the right of its border

#### Scenario: Desktop cell containment

- **WHEN** the viewport width is at least 960px
- **THEN** eight knob columns in the 4×2 grid show all content inside each bordered cell
- **AND** mod bay horizontal alignment with the knob grid is preserved

### Requirement: VCO morph visible above overlapping controls

On the Audio page, VCO1–VCO3 morph waveform controls SHALL be fully visible and SHALL NOT be obscured by the rotary knob in the same column or by an adjacent column's knob.

#### Scenario: Morph not behind same-column knob

- **WHEN** the user views VCO1 on the Audio page at 375px
- **THEN** the waveform SVG is fully visible in the morph row
- **AND** the rotary knob does not cover any part of the waveform icon

#### Scenario: Morph not behind neighbor knob

- **WHEN** the user views VCO1–VCO3 at 375px in the two-column grid
- **THEN** no morph waveform is hidden under an adjacent column's rotary knob

#### Scenario: Morph remains tappable

- **WHEN** the user taps a visible VCO morph waveform
- **THEN** the SVG cycles and `cycleVcoMorph` fires after engine ready
