## ADDED Requirements

### Requirement: Current-page mod route summary

The web UI SHALL render a read-only summary of mod assignments on the **current host page only**. For each row with mod source not equal to None, the summary SHALL show parameter name, source label, and mod depth percentage.

#### Scenario: Summary lists active routes

- **WHEN** on Audio page row **V1VO** has mod source Marbles 1 at 50% depth
- **THEN** summary includes `V1VO ← Marbles 1 · 50%`

#### Scenario: Empty summary

- **WHEN** no rows on the current page have an active mod source
- **THEN** summary displays `No mod routes on this page`

#### Scenario: Delay page summary

- **WHEN** on Delay page **DTIM** is modulated by VCO level
- **THEN** summary uses Delay row names and `DelayState` mod sources

### Requirement: Mod column labeling

Each knob column SHALL stack: parameter label, vertical slider, **Mod source** select, and when a source is selected the slider SHALL control mod depth and display a **Mod depth** label.

#### Scenario: Depth mode when mod assigned

- **WHEN** the user selects Marbles 2 on row 3's mod source dropdown
- **THEN** moving the slider sends mod depth messages
- **AND** the slider label reads **Mod depth**

#### Scenario: Knob mode when none

- **WHEN** mod source is None
- **THEN** the slider sends knob value messages
- **AND** the slider label matches the parameter name

## ADDED Requirements

### Requirement: Web mod assignment is dropdown-only

Web mod source assignment SHALL use only per-knob `<select>` dropdowns. No patch cables, drag wires, or tap-to-connect on mod sources or parameter rows. The mod route summary and any other UI element SHALL NOT assign mod indices — only reflect dropdown state.

#### Scenario: No drag mod assignment on web

- **WHEN** the user interacts with the web simulator on any viewport size
- **THEN** no UI element provides drag-to-connect mod routing
- **AND** changing a mod source requires the per-knob dropdown

#### Scenario: Route summary is read-only

- **WHEN** the user taps a line in the mod route summary
- **THEN** no mod assignment changes unless the user edits the corresponding dropdown

## MODIFIED Requirements

### Requirement: Web dropdown assignment

Web SHALL use per-knob **dropdown** mod assignment on the current page only. Cables are out of scope for web v2.1 and v2 web UX upgrades. Dropdown options SHALL be exactly: **None | VCO level | Marbles 1 | Marbles 2** (indices 255, 4, 5, 6). Randomize mod SHALL assign only those indices.

#### Scenario: Dropdown sets core index

- **WHEN** the user selects Marbles 2 on knob row 3's dropdown
- **THEN** WASM calls `SetRowModSource(3, 6)`

#### Scenario: Web randomize mod sim-valid

- **WHEN** the user clicks page **Randomize mod** or global **Randomize mod (all)**
- **THEN** assigned indices are only 255, 0, 4, 5, or 6

#### Scenario: Route summary updates after dropdown change

- **WHEN** the user changes a mod source dropdown
- **THEN** the route summary updates on the next screen refresh without page reload
