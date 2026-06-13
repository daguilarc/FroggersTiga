## ADDED Requirements

### Requirement: Mod rack CV scopes are wide trace displays

Each mod rack box (MIDI, VCO Envelope, Marbles 1, Marbles 2) SHALL show a **horizontal CV trace** (`CvScopeDisplay`), not a vertical level slider. The trace area SHALL be rectangular: width fills the mod box minus padding; height at least **40px**. Boxes SHALL be evenly spaced with visible gaps (≥16px) across the mod rack row.

#### Scenario: Scope aspect ratio

- **WHEN** the user views the mod rack at default window size
- **THEN** each scope is substantially wider than tall (approximately 3:1 or greater width:height)
- **AND** scopes are evenly spaced with gaps between boxes

#### Scenario: Trace vs slider

- **WHEN** Marbles or MIDI mod CV changes over time during Play
- **THEN** the display shows a scrolling trace line
- **AND** the UI does not resemble a vertical slider thumb on a track
