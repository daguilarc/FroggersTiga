## MODIFIED Requirements

### Requirement: Permanent modulation source display names use Random S&H
The Froggers v2 product manifest SHALL declare permanent modulation source `displayName` values for the random lanes as **Random S&H 1** and **Random S&H 2**. Stable IDs MAY retain internal `random_marbles_*` identifiers. Operator-visible UI projections SHALL use the manifest `displayName` and SHALL NOT show the substring **Marbles**.

#### Scenario: Random lane UI names
- **WHEN** desktop v2 projects permanent modulation source labels into the performance band or mod detail grid
- **THEN** the random lanes read **Random S&H 1** and **Random S&H 2**
- **THEN** no operator-visible chrome string contains **Marbles**

### Requirement: Controller targets omit Shift
The Froggers v2 product manifest controller-target declarations SHALL NOT include a Shift / held-modifier target. Desktop v2 HAS NO held-gesture model.

#### Scenario: No midi_shift_button declaration
- **WHEN** `controllerTargetDeclarations()` is enumerated
- **THEN** no entry uses stable ID `midi_shift_button`
- **THEN** scene targets occupy the slots immediately after MIDI CC A/B

### Requirement: Per-parameter encoder targets are inventory-generated
The Froggers v2 product manifest SHALL generate controller-target declarations for encoder turn and encoder mod drill-in from interactive module-row / `HostParameterInventoryV2` PageKnob stable IDs. Stable IDs SHALL use the suffix pattern `{pageKnobStableId}_encoder_turn` and `{pageKnobStableId}_encoder_mod_drill_in`. Binding roles SHALL be `encoder turn` and `encoder mod drill-in`.

#### Scenario: Encoder target count matches product rows
- **WHEN** `controllerTargetDeclarations()` is enumerated
- **THEN** the table includes exactly two encoder targets per interactive product row
- **THEN** each encoder target carries page/row coordinates for control-core dispatch
