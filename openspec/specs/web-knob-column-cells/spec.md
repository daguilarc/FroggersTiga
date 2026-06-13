# web-knob-column-cells Specification

## Purpose
TBD - created by archiving change web-sim-layout-ux. Update Purpose after archive.
## Requirements
### Requirement: Each knob column is a bordered cell

Each of the eight `.knob-col` elements on the current page SHALL render as a bordered vertical cell containing, top to bottom: parameter title label, rotary knob, mod-source label, and mod `<select>`. The cell SHALL have visible border and panel background.

#### Scenario: Audio page columns

- **WHEN** the user views the Audio page
- **THEN** eight bordered column cells appear in one row labeled VCO1 through Crunch
- **AND** no outer group meta-panel wraps subsets of columns (no VCOs / Coupling / Output boxes)

#### Scenario: Page change updates labels only

- **WHEN** the user navigates to the Filter page
- **THEN** the same eight column cells remain; only labels and values update
- **AND** no group wrapper DOM is added or removed

### Requirement: No group meta-panels

The web sim SHALL NOT render `knob-group` outer panels, group titles, or `HOST_PAGE_GROUPS` layout wrappers. Module context stays in page chrome title + blurb only.

#### Scenario: No nested group chrome

- **WHEN** inspecting the knob field on any page
- **THEN** `.knobs` contains `.knob-col` children directly (or via one flat layout pass)
- **AND** zero elements with class `knob-group` or `knob-group-title` exist in the DOM

