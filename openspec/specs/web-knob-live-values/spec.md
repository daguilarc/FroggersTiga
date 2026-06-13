# web-knob-live-values Specification

## Purpose
TBD - created by archiving change web-knob-live-values. Update Purpose after archive.
## Requirements
### Requirement: Idle knobs show effective parameter value

When the user is not dragging a knob column, the rotary knob position SHALL reflect `rows[i].value` from each WASM `screen` message — the effective modulated parameter value from `froggers_row_value` / `Parameter::Get`, regardless of whether the row has an active mod source.

#### Scenario: Unpatched row idle refresh

- **WHEN** row mod source is **None**, audio is playing, and the user is not dragging that column
- **THEN** the knob position updates to `rows[i].value` on each `screen` tick

#### Scenario: Patched row live modulation

- **WHEN** a row has mod source VCO Envelope, Marbles 1 S&H, or Marbles 2 S&H, audio is playing, and the user is not dragging that column
- **THEN** the knob position updates to `rows[i].value` on each `screen` tick
- **AND** the knob visibly tracks CV movement (wiggles) like the desktop sim and thenoriegas.info

#### Scenario: Randomize moves knobs

- **WHEN** the user clicks page **Randomize**, **Rand All**, or **Rand Mods** while playing
- **THEN** subsequent `screen` messages update all idle knob positions to new `rows[i].value`
- **AND** patched and unpatched rows behave the same on idle display

### Requirement: Patched-row drag edits mod depth with snap

When a row has an active mod source and the user begins dragging its knob, the control SHALL switch to editing mod attenuator depth until pointer-up. On pointer-down, the knob SHALL snap to `rows[i].modDepth` before drag delta is applied.

#### Scenario: Drag start on patched row

- **WHEN** row mod source is not **None** and the user pointer-downs the knob
- **THEN** the knob snaps to the current `modDepth` from the last `screen` row
- **AND** drag sends `{ type: "modDepth" }` or `{ type: "delayModDepth" }` to the worklet

#### Scenario: Drag end resumes live display

- **WHEN** the user releases pointer after editing mod depth on a patched row
- **THEN** `knobDragging[i]` clears
- **AND** the next `screen` tick sets knob position back to `rows[i].value`

#### Scenario: Unpatched drag unchanged

- **WHEN** row mod source is **None** and the user drags the knob
- **THEN** the worklet receives `{ type: "knob" }` or `{ type: "delayKnob" }`
- **AND** idle refresh continues to use `rows[i].value`

### Requirement: Static parameter labels during mod routing

Knob column primary labels SHALL remain the static page parameter names from `HOST_PAGE_LABELS`. Mod routing SHALL NOT rename a column to **Mod depth** on idle display.

#### Scenario: Patched row label

- **WHEN** a row has an active mod source and the user is not dragging
- **THEN** the column label shows the page parameter name (e.g. VCO1, Mix, Crunch)
- **AND** mod assignment is indicated by the mod-source dropdown and route summary only

#### Scenario: Delay page hints preserved

- **WHEN** the Delay page is active
- **THEN** delay hint slot under the label keeps its reserved height per knob-column layout spec

### Requirement: Screen sync does not split display value by patch state

`syncKnobUi` SHALL use one assignment for idle knob value: `rows[i].value` for all rows. It SHALL NOT assign `rows[i].modDepth` to knob position on idle refresh.

#### Scenario: No modDepth idle branch

- **WHEN** `syncKnobUi` runs for a patched row with `knobDragging[i] === false`
- **THEN** `rotaryKnobs[i].setValue(rows[i].value)` is called
- **AND** `rows[i].modDepth` is not used for knob position on that code path

