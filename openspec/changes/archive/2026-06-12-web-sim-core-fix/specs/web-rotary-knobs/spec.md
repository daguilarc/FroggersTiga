## ADDED Requirements

### Requirement: Parameter controls are rotary knobs

Each of the eight knob columns SHALL use a rotary knob control (circular arc with pointer), not a vertical range slider. Touch target SHALL be at least **44×44 px**.

#### Scenario: Visual appearance

- **WHEN** any host page is displayed at viewport width ≤720 px
- **THEN** each parameter control renders as a rotary knob
- **AND** eight knobs fit in the field layout without horizontal scrolling

#### Scenario: Unpatched drag

- **WHEN** row mod source is **None** and the user drags a knob
- **THEN** the worklet receives `{ type: "knob", index, value }` or `{ type: "delayKnob", row, value }` on the Delay page

#### Scenario: Patched drag

- **WHEN** row mod source is not **None** and the user drags a knob
- **THEN** the worklet receives `{ type: "modDepth" }` or `{ type: "delayModDepth" }` with attenuator depth 0–1

### Requirement: Knob position syncs from screen payload

When the user is not dragging a knob, knob position SHALL reflect the `screen` row value (parameter value when unpatched, mod depth when patched), matching current slider sync semantics.

#### Scenario: Screen update while idle

- **WHEN** a `screen` message arrives and the user is not dragging that column
- **THEN** the rotary knob position updates to the row's effective value
