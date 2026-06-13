## MODIFIED Requirements

### Requirement: Each knob column is a bordered cell

Each of the eight `.knob-col` elements on the current page SHALL render as a bordered vertical cell containing, top to bottom: parameter title label, optional hint slot (fixed height), rotary knob row, mod-source label, and mod `<select>`. The cell SHALL have visible border and panel background. Column geometry SHALL be identical on all six host pages including Delay.

#### Scenario: Audio page columns

- **WHEN** the user views the Audio page
- **THEN** eight bordered column cells appear in the grid with uniform height
- **AND** no page-specific accent border wraps the page chrome

#### Scenario: Delay page columns match other pages

- **WHEN** the user views the Delay page
- **THEN** knob columns use the same border, padding, and min-height as Audio–Drive pages
- **AND** the page chrome border color matches non-Delay pages

#### Scenario: Randomize mod does not resize columns

- **WHEN** the user clicks **Randomize mod** on the Delay page
- **THEN** mod source dropdowns update
- **AND** knob column heights remain uniform across all eight columns

### Requirement: Hint slot uses reserved height

The hint slot below the parameter label SHALL reserve fixed vertical space on every column. Hint text SHALL NOT be hidden when a mod source is assigned.

#### Scenario: Delay time hint with mod patched

- **WHEN** row 0 on the Delay page has a mod source assigned
- **THEN** the hint slot keeps its reserved height (text may remain `~0–2 s` or empty)
- **AND** the column height matches unpatched columns
