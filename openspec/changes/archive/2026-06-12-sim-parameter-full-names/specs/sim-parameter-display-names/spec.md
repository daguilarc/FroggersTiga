## ADDED Requirements

### Requirement: Sim hosts show full parameter column titles

Desktop submodule panels and web knob columns SHALL display the **sim display name** from `ParamDisplayNames` for each row, not the 4-character firmware OLED label.

#### Scenario: Reverb panel readable labels

- **WHEN** the user views the Reverb panel or web Reverb page
- **THEN** row labels include **Wet/dry**, **Room size**, **Decay**, **Pre-delay**, **Damping**, **LFO depth**, **LFO rate**, and **Crunch**
- **AND** labels do **not** show `RVMX`, `RSIZ`, `FUEG`, etc.

#### Scenario: Audio VCO rows

- **WHEN** the user views the Audio panel or web Audio page
- **THEN** rows 0–2 are labeled **VCO1**, **VCO2**, **VCO3**

#### Scenario: Delay page

- **WHEN** the user views the Delay panel or web page 6
- **THEN** rows show **Delay time**, **Send**, **Feedback**, **Stereo width**, **Tone**, **Mod depth**, **Wet mix**, **Crunch**

### Requirement: Fuegoizer row labeled Crunch

Row index 7 (fuegoizer) on every sim page SHALL display **Crunch** in desktop and web UI.

#### Scenario: Filter Crunch row

- **WHEN** the user views any sim page column for knob 8
- **THEN** the label reads **Crunch**
- **AND** the label does not read `FUEG` or **Fuegoizer**

### Requirement: Single dictionary source

`sim/ParamDisplayNames.hpp` SHALL be the only authoritative mapping from `(hostPage, row)` to display string for desktop and WASM screen payloads.

#### Scenario: Desktop and web agree

- **WHEN** the same page and row are shown on desktop and web
- **THEN** both hosts show identical display text
