## MODIFIED Requirements

### Requirement: VCO morph visible on mobile Audio page

The web sim SHALL display VCO morph waveform buttons on the Audio page at mobile viewport widths without overlap from adjacent knobs. Morph controls SHALL sit in the vertical stack between the parameter label and the rotary knob inside each VCO column cell.

#### Scenario: Morph buttons visible at 375px

- **WHEN** the user opens the web sim on a viewport ≤720px wide and navigates to the Audio page
- **THEN** each VCO1–VCO3 morph waveform icon is visible between its label and knob within the same knob cell
- **AND** the icon is not obscured by an adjacent column's rotary knob

#### Scenario: Morph button tappable on mobile

- **WHEN** the user taps a VCO morph waveform button on a viewport ≤720px wide
- **THEN** the waveform cycles (sine → saw → square) and the SVG updates

### Requirement: Desktop layout unchanged

The knob grid on viewports ≥721px SHALL remain a single row of four columns. VCO morph controls SHALL use the same vertical stack between label and knob as on mobile.

#### Scenario: Desktop Audio page layout

- **WHEN** the viewport is ≥721px wide on the Audio page
- **THEN** eight knob cells appear in one 4×2 grid (four columns) as before this change
- **AND** morph waveform buttons appear between VCO labels and knobs, not beside them
