# froggers-v2-controller-configuration Specification

## Purpose
Desktop standalone v2 MIDI/Controllers configuration uses manifest target IDs, explicit mapping fields, multi-target fan-out, and product-formatted readback without MIDI learn or recent-event UI.

## Requirements
### Requirement: Labeled controller configuration page
Desktop standalone v2 SHALL expose a MIDI/Controllers configuration page where every setup field has a visible label. The page SHALL show selected MIDI input, connection/receiving/error state, explicit mapping event fields, assignment targets, message kind, channel/CC/note details, multi-target fan-out status, persistence status, target readback, and sequencer clock source. It SHALL NOT introduce a MIDI learn mode or recent-event list.

#### Scenario: Mapping event fields are explicit
- **WHEN** the user edits a controller mapping row
- **THEN** the Controllers page shows editable event fields for message kind, channel, controller/note number, optional value/range fields, and one or more manifest target IDs
- **THEN** committing the row binds that physical event to every selected target through the controller model
- **THEN** the mapping flow does not require a separate learn mode, transient heard-event feedback area, or recent-event list

#### Scenario: Physical input fans out to multiple targets
- **WHEN** a new mapping uses the same selected input, message kind, channel, and controller number as an existing mapping for a different target
- **THEN** the Controllers page shows the existing and new targets as a multi-target fan-out
- **THEN** the mapping can be committed without requiring a warning override
- **THEN** incoming physical input updates every target in that fan-out through the controller model

#### Scenario: Target readback uses context display
- **WHEN** a mapped controller changes a target
- **THEN** the Controllers page shows the target's current value using the parameter row display format when that format is available
- **THEN** the normal Controllers page does not show raw normalized values
- **THEN** any raw normalized readback is limited to diagnostics, generated reports, or tests

#### Scenario: MIDI clock source syncs only the sequencer
- **WHEN** the user selects a MIDI clock source for the sequencer
- **THEN** the Controllers page shows that clock source as sequencer timing input
- **THEN** incoming MIDI clock advances or synchronizes sequencer timing according to the sequencer clock contract
- **THEN** incoming MIDI clock does not create controller mappings, parameter gestures, or private modulation routes

#### Scenario: Mapped step control long press clears step
- **WHEN** a controller mapping targets a sequencer step control
- **AND** the user holds that mapped control past the step long-press threshold
- **THEN** the runtime emits the same step-local clear command as mouse press-and-hold or touch press-and-hold on that step
- **THEN** the addressed sequencer step is marked unwritten
- **THEN** the hold does not create a general parameter gesture, randomization modifier, Crunchy modifier, Crispy modifier, or private modulation route

### Requirement: Controller mappings target manifest IDs
Controller mappings SHALL reference manifest-owned MIDI target IDs rather than UI labels, row coordinates, or plugin parameter display names.

#### Scenario: Target label changes without mapping loss
- **WHEN** a target display label changes in the manifest
- **THEN** saved controller mappings still resolve through the unchanged target ID

#### Scenario: Missing target rejected
- **WHEN** a saved mapping references a target ID absent from the manifest
- **THEN** the configuration page reports the missing target
- **THEN** the runtime does not apply that mapping

### Requirement: Desktop and hosted MIDI projections differ by host
Desktop standalone SHALL consume configured hardware MIDI through the controller model. VST/AU SHALL use DAW host-parameter mapping and SHALL NOT create a second private raw-MIDI modulation table.

#### Scenario: Desktop CC assignment updates semantic target
- **WHEN** desktop standalone maps CC 74 to a manifest target
- **THEN** incoming CC 74 updates that target through the controller model

#### Scenario: Hosted MIDI avoids duplicate route
- **WHEN** FroggersTigaPluginV2 receives MIDI in a DAW
- **THEN** MIDI-driven changes apply through DAW host-parameter mapping semantics
- **THEN** no hidden private CC-to-mod-source table is mutated
