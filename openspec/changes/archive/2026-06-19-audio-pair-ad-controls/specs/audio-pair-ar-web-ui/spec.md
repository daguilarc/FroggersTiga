## ADDED Requirements

### Requirement: Third knob row on Audio page

When the browser simulator displays host page 0 (Audio), the knob grid SHALL show **twelve** cells arranged as three rows of four columns.

Cells 0–7 SHALL remain the existing Audio parameters (VCO1 through Crispy). Cells 8–11 SHALL be the four pair-AR parameters in order: Attack 1+2, Release 1+2, Attack 2+3, Release 2+3.

Each pair-AR cell SHALL use the same column layout as existing cells: label, knob, mod-source select.

#### Scenario: Non-Audio pages unchanged

- **WHEN** the user navigates to Reverb, Filter, Drive, or Delay
- **THEN** the grid shows eight cells in two rows only

#### Scenario: Pair-AR knobs drive engine

- **WHEN** the user adjusts Release 1+2 on the Audio third row
- **THEN** the worklet receives the updated value and audio reflects longer pair-12 fall time

### Requirement: Web labels from shared authority

Web pair-AR labels SHALL match `ParamDisplayNames::forAudioPairAr` (via WASM screen payload or generated constants) — not hardcoded strings diverging from desktop.

#### Scenario: Label parity desktop vs web

- **WHEN** Audio page is open on desktop and web with default locale
- **THEN** all four pair-AR labels match between hosts
