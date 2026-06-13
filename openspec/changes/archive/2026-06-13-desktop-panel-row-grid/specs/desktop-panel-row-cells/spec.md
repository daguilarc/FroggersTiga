## ADDED Requirements

### Requirement: Each parameter row is a bordered cell

Each of the eight parameter rows in a `SubModulePanel` (rows 0–6 plus row 7 Crunch/FUEG) SHALL render inside a visible bordered cell. Cells SHALL stack vertically in a single column (eight rows, one column). Border stroke SHALL be 1 px using panel chrome contrast colour.

#### Scenario: Audio panel row cells

- **WHEN** the user views the Audio submodule panel
- **THEN** eight bordered cells appear stacked vertically
- **AND** each cell contains one parameter label, one knob, and one input jack
- **AND** VCO rows 0–2 additionally contain the wave morph control inside the same cell

#### Scenario: Delay panel row cells

- **WHEN** the user views the Delay submodule panel
- **THEN** eight bordered cells appear with the same grid structure as other panels
- **AND** no wave morph control is shown (Delay has no VCO wave rows)

### Requirement: Label and controls are grouped inside the cell

Within each row cell, the parameter label SHALL NOT span the full panel width detached from its knob. The label, knob, optional wave button, and input jack SHALL be laid out within the same cell bounds.

#### Scenario: Visual grouping

- **WHEN** the user looks at any parameter row
- **THEN** the label and its knob appear in the same bordered rectangle
- **AND** the input jack appears in that same rectangle adjacent to the knob

#### Scenario: No full-width label strip

- **WHEN** measuring layout at default window width (1440×720)
- **THEN** the horizontal gap between label text and knob is less than the full panel inner width minus control cluster width

### Requirement: Row cell layout uses one loop

`SubModulePanel::layoutPanel` SHALL compute all eight row cell bounds in one loop over row indices. Border painting SHALL use the same stored bounds in one loop. The implementation SHALL NOT duplicate layout logic per row with copy-paste blocks that differ only by index.

#### Scenario: Maintainability

- **WHEN** reviewing `SubModulePanel` layout code
- **THEN** rows 0–6 share one layout path
- **AND** row 7 uses the same path with `hasWave = false`
