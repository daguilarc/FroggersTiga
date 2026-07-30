## ADDED Requirements

### Requirement: Control core manifest boundary
Desktop v2 control core SHALL expose a manifest-backed structural boundary while preserving existing message-bus, scene, modulation-depth drill-down, fixed 16-step sequencer snapshot/lock, sequencer direction/speed, and UI-state behavior.

#### Scenario: Manifest-backed parameter lookup
- **WHEN** `ParamIncDec` targets a page row declared in the manifest
- **THEN** the control core resolves the target through the manifest stable control entry
- **THEN** scene, sequencer-lock, and modulation-depth behavior match the v2 control-core scenarios

#### Scenario: Missing manifest entry rejected
- **WHEN** a UI or MIDI message targets a control absent from the manifest
- **THEN** the control core rejects the message without mutating parameter state

### Requirement: Sequencer snapshots use manifest field coverage
Sequencer snapshot capture and apply SHALL use exactly 16 manifest-declared step slots and manifest-declared persistence fields for centers, parameter locks, gates, mod sources, and mod depths.

#### Scenario: Snapshot coverage check
- **WHEN** the sequencer snapshot coverage test runs
- **THEN** it fails if the sequencer exposes any count other than 16 step slots
- **THEN** every manifest field marked `sequencerPersistent` is captured and applied
- **THEN** no captured field lacks a manifest declaration

### Requirement: Sequencer direction and speed preserve fixed step slots
Sequencer direction and speed changes SHALL alter traversal and timing only. They SHALL NOT add, remove, renumber, or resize the 16 step snapshots.

#### Scenario: Direction changes preserve step slots
- **WHEN** the user selects `<`, `>`, or `RND`
- **THEN** the sequencer still exposes exactly 16 step slots
- **THEN** existing step snapshots and locks remain assigned to the same indices

#### Scenario: Speed changes preserve step slots
- **WHEN** the user selects `/2`, `/1.5`, `1`, `x1.5`, or `x2`
- **THEN** the sequencer still exposes exactly 16 step slots
- **THEN** existing step snapshots and locks remain assigned to the same indices
