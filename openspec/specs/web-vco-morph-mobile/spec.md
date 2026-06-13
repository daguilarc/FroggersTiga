# web-vco-morph-mobile Specification

## Purpose
TBD - created by archiving change web-vco-morph-mobile. Update Purpose after archive.
## Requirements
### Requirement: VCO morph visible on mobile Audio page

The web sim SHALL display VCO morph waveform buttons beside their knobs on the Audio page at mobile viewport widths without overlap from adjacent knobs.

#### Scenario: Morph buttons visible at 375px

- **WHEN** the user opens the web sim on a viewport ≤720px wide and navigates to the Audio page
- **THEN** each VCO1–VCO3 morph waveform icon is visible to the right of its knob within the same knob cell
- **AND** the icon is not obscured by an adjacent column's rotary knob

#### Scenario: Morph button tappable on mobile

- **WHEN** the user taps a VCO morph waveform button on a viewport ≤720px wide
- **THEN** the waveform cycles (sine → saw → square) and the SVG updates

### Requirement: Desktop layout unchanged

The knob grid on viewports ≥721px SHALL remain a single row of four columns with morph buttons beside VCO knobs.

#### Scenario: Desktop Audio page layout

- **WHEN** the viewport is ≥721px wide on the Audio page
- **THEN** eight knob cells appear in one 4×2 grid (four columns) as before this change

### Requirement: Mobile page navigation preserved

Mobile layout fixes SHALL preserve flanking page-nav arrows and centered `#app` column alignment.

#### Scenario: Mobile nav arrows remain

- **WHEN** the viewport is ≤720px wide
- **THEN** previous/next page buttons remain visible flanking the knob grid

