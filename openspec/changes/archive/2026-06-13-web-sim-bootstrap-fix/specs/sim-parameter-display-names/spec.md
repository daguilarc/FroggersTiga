## MODIFIED Requirements

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
