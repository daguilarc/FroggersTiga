# web-mod-route-summary Specification

## Purpose
TBD - created by archiving change web-sim-layout-ux. Update Purpose after archive.
## Requirements
### Requirement: Mod route summary hidden when empty

The `#mod-route-summary` element SHALL be hidden when the current page has no active mod routes. It SHALL NOT render an empty bordered box or “No mod routes” placeholder strip.

#### Scenario: Audio page default

- **WHEN** the Audio page has no mod assignments
- **THEN** `#mod-route-summary` is not visible
- **AND** no bordered panel appears between page chrome and knobs

#### Scenario: Pre-Play page load

- **WHEN** the page loads before the user clicks Play and before the first WASM `screen` message
- **THEN** `#mod-route-summary` is not visible
- **AND** no bordered panel appears between page chrome and knobs

#### Scenario: Route exists

- **WHEN** at least one knob on the current page has mod source not **None**
- **THEN** `#mod-route-summary` is visible with clickable route lines
- **AND** layout does not shift knobs when summary appears (reserved space optional; hidden preferred over empty box)

