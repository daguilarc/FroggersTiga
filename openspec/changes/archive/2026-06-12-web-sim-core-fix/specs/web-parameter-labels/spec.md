## ADDED Requirements

### Requirement: Knob columns show ParamDisplayNames

Each knob column SHALL display the sim display name from `ParamDisplayNames` for the current host page and row. Placeholder labels **Knob 1** through **Knob 7** SHALL NOT appear in production UI.

#### Scenario: Audio page labels

- **WHEN** host page is Audio (0)
- **THEN** rows 0–2 are labeled **VCO1**, **VCO2**, **VCO3**
- **AND** row 7 is labeled **Crunch**

#### Scenario: Delay page labels

- **WHEN** host page is Delay (5)
- **THEN** rows show **Delay time**, **Send**, **Feedback**, **Stereo width**, **Detune**, **Mod depth**, **Wet mix**, **Crunch** (Detune/Comb line from `delay-grain-filter-row0`)

### Requirement: Mod depth overrides column label only when patched

When a row has an active mod source, the knob column label SHALL change to **Mod depth**. The parameter display name SHALL remain visible on the OLED row for that index.

#### Scenario: Patched row

- **WHEN** row 0 on Audio page has mod source **Marbles 1**
- **THEN** knob column 0 label reads **Mod depth**
- **AND** OLED row 0 still shows **VCO1**

### Requirement: Page chrome is not the only parameter naming surface

The page chrome blurb describes page role; it SHALL NOT replace per-knob labels. Users SHALL identify each parameter from the knob column label and OLED name without reading the chrome blurb alone.

#### Scenario: First visit without Play

- **WHEN** a new user loads the sim before clicking Play
- **THEN** all eight knob columns show readable parameter names for the current page
