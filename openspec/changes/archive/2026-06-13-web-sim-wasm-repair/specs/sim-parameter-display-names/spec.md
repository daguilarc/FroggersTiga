## MODIFIED Requirements

### Requirement: Page chrome is not the only naming surface

The page chrome blurb describes module role; it SHALL NOT replace per-knob labels.

#### Scenario: First visit without Play

- **WHEN** WASM has loaded successfully and the user has not clicked Play
- **THEN** all eight knob columns show readable parameter names for the current page from the first `screen` message
- **AND** no knob column shows `—` or a hardcoded placeholder after bootstrap completes
