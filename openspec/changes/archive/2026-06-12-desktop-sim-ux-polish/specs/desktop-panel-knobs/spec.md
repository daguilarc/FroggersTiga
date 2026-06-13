## ADDED Requirements

### Requirement: Submodule panels use compact rotary knobs

Each parameter row in `SubModulePanel` (knobs 1–7 and FUEG) SHALL use a rotary knob control with a fixed footprint of at most **44×44 px**, replacing vertical linear sliders. Row height SHALL be reduced so eight rows fit without excessive empty vertical space at six-panel layout.

#### Scenario: Knob interaction unpatched

- **WHEN** row has no mod source (mod index 255) and the user drags a knob
- **THEN** `setKnob(row, value)` is called with the knob value

#### Scenario: Knob interaction patched

- **WHEN** row has a mod source and the user drags a knob
- **THEN** `setModDepth(row, value)` is called with attenuator depth

### Requirement: Full VCO row labels at six-panel width

On the Audio panel, rows 0–2 SHALL display **VCO1**, **VCO2**, **VCO3** (sim display aliases per `desktop-host-corrections`) plus the wave morph control without ellipsis truncation at default window size (**1680×720** per `desktop-compact-layout`; was 2016×720).

#### Scenario: Label readability

- **WHEN** the Audio panel is shown at default size
- **THEN** each of VCO1, VCO2, VCO3 is fully visible
- **AND** the wave button remains visible beside the label

### Requirement: Knobs show effective value when modded and idle

When a row has an active mod patch and the user is not dragging that knob, the knob position SHALL reflect the **effective modulated parameter value** after the last audio block, not merely the static attenuator depth.

#### Scenario: Live modulation display

- **WHEN** VCO feat mod is patched to a knob, audio is playing, and the user is not dragging that knob
- **THEN** the knob position updates at UI refresh rate to track modulated parameter change

#### Scenario: Drag overrides live display

- **WHEN** the user begins dragging a patched knob
- **THEN** the knob switches to editing mod depth until drag ends
