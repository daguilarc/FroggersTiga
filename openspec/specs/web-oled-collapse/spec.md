# web-oled-collapse Specification

## Purpose
TBD - created by archiving change web-sim-layout-ux. Update Purpose after archive.
## Requirements
### Requirement: No empty black OLED void

The `#oled` panel SHALL NOT display a large empty black rectangle when the sim is stopped, before Play, or when OLED content is empty.

#### Scenario: Pre-Play load

- **WHEN** the page loads before the user clicks Play
- **THEN** the OLED region is hidden or collapsed to zero/minimal height
- **AND** no 220 px black box appears below the knobs

#### Scenario: After Stop

- **WHEN** the user clicks Stop
- **THEN** the OLED region collapses or hides again
- **AND** knob groups and labels remain visible

### Requirement: OLED visible when playing on desktop

When audio is playing and viewport is wider than 720 px, the OLED SHALL show the full eight-row value mock with names, bars, wave buttons (Audio), and mod badges.

#### Scenario: Playing on desktop

- **WHEN** Play is active on a viewport >720 px
- **THEN** OLED shows eight rows of live values from WASM `screen`
- **AND** OLED height fits content (no forced empty min-height)

### Requirement: Mobile OLED compact strip when playing

On viewports ≤720 px with Play active, OLED SHALL be a compact strip (≤48 px) showing wave buttons on Audio and mod badges only — not duplicate knob labels or value bars.

#### Scenario: Mobile playing

- **WHEN** Play is active on a 390 px wide viewport
- **THEN** OLED height is ≤48 px
- **AND** knob column labels remain the primary name surface

