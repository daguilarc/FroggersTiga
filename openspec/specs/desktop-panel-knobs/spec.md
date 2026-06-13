# desktop-panel-knobs Specification

## Purpose
TBD - created by archiving change desktop-panel-row-grid. Update Purpose after archive.
## Requirements
### Requirement: Knobs and jacks sit inside bordered row cells

Each parameter row in `SubModulePanel` SHALL lay out its rotary knob and input jack inside the bordered row cell per `desktop-panel-row-cells`, alongside the row label — not at the panel far edge with a detached label strip.

#### Scenario: Knob inside row cell

- **WHEN** the user views any submodule panel at default window size
- **THEN** every rotary knob bounding box is fully contained in its row cell rectangle
- **AND** the corresponding input jack bounding box is fully contained in the same row cell rectangle

