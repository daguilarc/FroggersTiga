## ADDED Requirements

### Requirement: Primary surfaces share one horizontal axis on desktop

On viewports wider than 720px, the mod bay, knob field, page pills, and global action strip SHALL share the same horizontal center line within `#app`. The left and right edges of the mod bay SHALL align with the left and right edges of the eight-knob grid.

#### Scenario: Desktop mod bay aligns with knob grid

- **WHEN** the viewport width is greater than 720px
- **THEN** the mod bay and `.knobs` grid span the same content width inside `#app`
- **AND** their outer left edges are vertically aligned

#### Scenario: Desktop global strip centered

- **WHEN** the viewport width is greater than 720px
- **THEN** the global strip button group is horizontally centered like the page pills
- **AND** the strip does not hug the left edge while pills are centered

#### Scenario: Desktop page nav hidden

- **WHEN** the viewport width is greater than 720px
- **THEN** flanking `#page-prev` and `#page-next` controls are not visible
- **AND** page navigation remains available via page pills and `[` / `]` keys

### Requirement: Mobile field layout preserves flanking navigation

On viewports at most 720px wide, the knob field SHALL keep large prev/next controls flanking the knob grid with touch targets at least 44×44 CSS pixels. The knob block SHALL remain horizontally centered between the arrows without horizontal scroll of the knobs.

#### Scenario: Mobile arrows visible

- **WHEN** the viewport width is at most 720px
- **THEN** `#page-prev` and `#page-next` are visible beside the knob grid
- **AND** each nav control meets the 44×44 CSS pixel minimum

#### Scenario: Mobile knob grid centered between arrows

- **WHEN** the viewport width is at most 720px
- **THEN** the `.knobs` grid sits in the center column of `.field-layout`
- **AND** horizontal scroll is not required to view all eight knob columns

### Requirement: Mod bay uses equal three-column grid

The mod sources panel SHALL lay out three mod indicators in a CSS grid with three equal columns. A conflicting flex layout rule SHALL NOT override the grid on `#mod-bay`.

#### Scenario: Three equal mod cells

- **WHEN** the user views the mod sources panel at any viewport width
- **THEN** VCO Envelope, Marbles 1 S&H, and Marbles 2 S&H cells each occupy one third of the mod bay width
- **AND** no mod cell wraps to a second row at default desktop width
