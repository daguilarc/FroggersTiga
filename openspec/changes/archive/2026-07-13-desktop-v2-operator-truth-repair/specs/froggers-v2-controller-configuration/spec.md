# froggers-v2-controller-configuration — Delta (Packet 19)

Extends baseline `froggers-v2-controller-configuration` with per-parameter encoder controller targets that dispatch device-neutral control-core messages.

## ADDED Requirements

### Requirement: Per-parameter encoder turn targets
Desktop standalone v2 SHALL expose manifest-owned controller targets for **parameter encoder turn** on every manifest-declared interactive module-row parameter. Each turn target SHALL resolve to `page` and `row` coordinates and SHALL dispatch `ParamTurn(page, slot, delta)` on the control-core message bus when mapped physical input arrives.

#### Scenario: Relative CC maps to parameter turn
- **WHEN** the user maps MIDI CC relative encoder motion to a parameter turn target for Filter cutoff on page 2 row 0
- **THEN** incoming relative CC updates that parameter through `ParamTurn` on the control core
- **THEN** scene, modulation, and sequencer behavior match mouse ring-drag on that encoder

#### Scenario: Turn target persists by stable ID
- **WHEN** the user saves a mapping to `filter_cutoff_encoder_turn` (manifest stable ID)
- **THEN** reloading the session restores the mapping even if the display label changes

### Requirement: Per-parameter mod drill-in press targets
Desktop standalone v2 SHALL expose manifest-owned controller targets for **parameter encoder mod drill-in** on every manifest-declared interactive module-row parameter. Each drill-in target SHALL resolve to `page` and `row` coordinates and SHALL dispatch `ModDrillIn(page, slot)` on the control-core message bus when mapped physical input arrives.

#### Scenario: Encoder button maps to mod drill-in
- **WHEN** the user maps a pressable encoder button (note-on or configured CC threshold) to a mod drill-in target for Filter cutoff
- **THEN** incoming press dispatches `ModDrillIn` for that page and row
- **THEN** parameter-detail modulation opens without `ParamPress` on the whole encoder ring

#### Scenario: Drill-in does not turn the parameter
- **WHEN** mod drill-in fires from a mapped encoder button
- **THEN** the parameter scene center does not change from the press alone
- **THEN** only mod-detail navigation state changes until the user turns a depth cell or exits via Target (Back)

### Requirement: Encoder targets generated from parameter inventory
Per-parameter encoder turn and mod drill-in controller targets SHALL be generated from the same manifest parameter inventory that backs `HostParameterInventoryV2` / product row stable IDs. Targets SHALL NOT use ad hoc UI labels or carousel slot indices as persistence keys.

#### Scenario: Missing parameter rejected
- **WHEN** a saved mapping references an encoder target stable ID absent from the manifest inventory
- **THEN** the Controllers page reports the missing target
- **THEN** the runtime does not apply that mapping

#### Scenario: Controllers page lists encoder targets
- **WHEN** the user opens the MIDI/Controllers configuration page
- **THEN** encoder turn and mod drill-in targets appear in the data-driven target list with manifest display names
- **THEN** the page does not require MIDI learn or recent-event UI

### Requirement: Packet 15 message boundary prerequisite
Per-parameter mod drill-in controller targets SHALL dispatch only `ModDrillIn`. They SHALL NOT reopen whole-encoder `ParamPress` mod entry or `setSingleModSource` single-route semantics.

#### Scenario: ModDrillIn prerequisite
- **WHEN** Packet 19 lands before `ModDrillIn` exists in the control core
- **THEN** Packet 19 is blocked — implement Packet 15.2 first
