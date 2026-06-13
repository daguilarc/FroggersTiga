## ADDED Requirements

### Requirement: Knob column labels are the primary parameter names on all viewports

Each knob column SHALL display the `ParamDisplayNames` string above the rotary control. The page chrome blurb SHALL NOT be the only place parameter identity is communicated.

#### Scenario: Audio page before Play on mobile

- **WHEN** the page loads on a viewport ≤720 px wide and the user has not clicked Play
- **THEN** knob columns 0–7 show **VCO1**, **VCO2**, **VCO3**, **Cross-coupler**, **Phase mod 1**, **Phase mod 2**, **VCO level**, **Crunch**
- **AND** no column shows placeholder `—`

### Requirement: Mobile OLED is a compact strip without black void

On viewports ≤720 px, the OLED panel SHALL NOT reserve a 220 px empty black area. It SHALL collapse to a compact strip showing wave morph buttons (Audio page VCO rows) and mod badges only. Parameter names and value bars SHALL be hidden on mobile OLED because knob columns already show names.

#### Scenario: Mobile layout height

- **WHEN** the sim is stopped on a 390 px wide viewport
- **THEN** the OLED region height is ≤64 px
- **AND** no large empty black rectangle appears between knobs and page pills

#### Scenario: Desktop OLED unchanged

- **WHEN** the viewport is wider than 720 px
- **THEN** the full eight-row OLED mock with names, value bars, wave buttons, and badges is visible

### Requirement: Page chrome shows module role not per-knob list

The page chrome title SHALL show page name and index (e.g. **Audio (1/6)**). The blurb MAY contain one sentence describing module role. It SHALL NOT list comma-separated parameter names as a substitute for knob column labels.

#### Scenario: Audio page chrome

- **WHEN** host page is Audio (0)
- **THEN** chrome title is **Audio (1/6)**
- **AND** knob columns independently show each parameter name
