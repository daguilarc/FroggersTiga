# sim-parameter-display-names Specification

## Purpose

Authoritative sim UI column titles for desktop, WASM screen payloads, and Quick Dict left tokens. Firmware 4-char OLED names are unchanged.
## Requirements
### Requirement: Sim hosts show full parameter column titles

Desktop submodule panels and web knob columns SHALL display the **sim display name** from `ParamDisplayNames` for each row, not the 4-character firmware OLED label.

#### Scenario: Audio VCO rows

- **WHEN** the user views the Audio panel or web Audio page
- **THEN** rows 0–2 are labeled **VCO1**, **VCO2**, **VCO3**

#### Scenario: Reverb panel readable labels

- **WHEN** the user views the Reverb panel or web Reverb page
- **THEN** rows include **Wet/dry**, **Room size**, **Decay**, **Pre-delay**, **Damping**, **Stereo width**, **Diffusion**, **Crunch**
- **AND** labels do **not** show `RVMX`, `RMOD`, `RRAT`, or **LFO depth** / **LFO rate**

#### Scenario: Filter panel

- **WHEN** the user views the Filter panel or web Filter page
- **THEN** row 0 is **Comb offset**, rows 1–3 are **Peak freq**, **Peak gain**, **Peak Q**, row 4 is **Comb delay**, rows 5–6 are **Comb feedback**, **Comb LP**, row 7 is **Crunch**

#### Scenario: Drive panel

- **WHEN** the user views the Drive panel or web Drive page
- **THEN** row 4 is **XOR**, row 5 is **Bit depth**
- **AND** **Reorganizer** does not appear

#### Scenario: Delay page

- **WHEN** the user views the Delay panel or web page 6
- **THEN** rows show **Delay time**, **Send**, **Feedback**, **Stereo width**, **Detune**, **Mod depth**, **Wet mix**, **Crunch**

### Requirement: Fuegoizer row labeled Crunch

Row index 7 on every sim page SHALL display **Crunch** in desktop and web UI.

#### Scenario: Any page knob 8

- **WHEN** the user views any sim page column for knob 8
- **THEN** the label reads **Crunch**
- **AND** the label does not read `FUEG` or **Fuegoizer**

### Requirement: Single dictionary source

`sim/ParamDisplayNames.hpp` SHALL be the only authoritative mapping from `(hostPage, row)` to display string for desktop and WASM screen payloads.

#### Scenario: Desktop and web agree

- **WHEN** the same page and row are shown on desktop and web
- **THEN** both hosts show identical display text

### Requirement: Page chrome is not the only naming surface

The page chrome blurb describes module role; it SHALL NOT replace per-knob labels.

#### Scenario: First visit without Play

- **WHEN** the sim page has loaded and the user has not clicked Play
- **THEN** all eight knob columns show readable parameter names for the current page from static sim display names
- **AND** no knob column shows `—` or a hardcoded placeholder
- **AND** labels match `ParamDisplayNames.hpp` (e.g. Filter row 0 **Comb offset**)

#### Scenario: After Play screen update

- **WHEN** WASM has loaded and posts a `screen` message
- **THEN** knob values and OLED refresh from WASM
- **AND** column labels remain sim display names unless mod routing active on that row

