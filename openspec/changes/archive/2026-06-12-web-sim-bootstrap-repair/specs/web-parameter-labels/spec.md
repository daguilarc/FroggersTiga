## MODIFIED Requirements

### Requirement: Knob columns show ParamDisplayNames

Each knob column SHALL display the sim display name from `ParamDisplayNames` for the current host page and row after the first `screen` message. Placeholder labels **Knob 1** through **Knob 7**, em-dash placeholders, and hardcoded per-row init strings (e.g. only row 7 as **Crunch**) SHALL NOT appear in production UI.

#### Scenario: Audio page labels

- **WHEN** host page is Audio (0) and the first `screen` message has been received
- **THEN** rows 0–2 are labeled **VCO1**, **VCO2**, **VCO3**
- **AND** row 7 is labeled **Crunch**

#### Scenario: Delay page labels

- **WHEN** host page is Delay (5)
- **THEN** rows show **Delay time**, **Send**, **Feedback**, **Stereo width**, **Detune**, **Mod depth**, **Wet mix**, **Crunch**

#### Scenario: Before first screen

- **WHEN** WASM is still loading and no `screen` has arrived
- **THEN** knob column labels are empty or a loading ellipsis
- **AND** labels do not show `—` or hardcoded **Crunch** on a single column

### Requirement: Page chrome is not the only parameter naming surface

The page chrome blurb describes page role; it SHALL NOT replace per-knob labels. Users SHALL identify each parameter from the knob column label without reading the chrome blurb alone.

#### Scenario: First visit without Play

- **WHEN** a new user loads the sim before clicking Play and WASM has loaded
- **THEN** all eight knob columns show readable parameter names for the current page
