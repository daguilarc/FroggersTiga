## ADDED Requirements

### Requirement: v2-sequencer-snapshot-mod-fields

`SequencerStepSnapshot` SHALL store per-page/row mod source index and mod depth matching the live `ParamState` mod field shape. `captureSequencerStepSnapshot` SHALL copy live mod assignments into the snapshot. `applySequencerStepSnapshot` SHALL restore mod assignments from the snapshot into the control core and host routing.

#### Scenario: Capture includes mod routing

- **WHEN** `captureSequencerStepSnapshot` runs with non-default mod sources assigned on multiple pages/rows
- **THEN** the output snapshot contains those mod source indices and depths

#### Scenario: Apply restores mod routing

- **WHEN** `applySequencerStepSnapshot` runs with a snapshot that stores mod fields differing from live state
- **THEN** live mod source and depth for each page/row match the snapshot after `populateUiState`

#### Scenario: Factory seed includes mod defaults

- **WHEN** `captureFactoryStepSnapshot` runs on a blank step
- **THEN** mod fields are set to factory defaults (no mod source / zero depth per inventory defaults)
- **THEN** `hasData` is true

### Requirement: v2-factory-seed-blank-step-on-advance

`FroggersV2HostBridge::onSequencerStepAdvance` SHALL seed blank steps before applying the landed step snapshot.

#### Scenario: Blank step seeded once per advance

- **WHEN** playhead lands on step `S` with `m_steps[S].hasData == false`
- **THEN** `captureFactoryStepSnapshot(m_steps[S])` runs before `applySequencerStepSnapshot(currentStep())`
- **THEN** step `S` has `hasData == true` with factory-derived scene centers

#### Scenario: Written step not re-seeded

- **WHEN** playhead lands on step `S` with `m_steps[S].hasData == true`
- **THEN** `captureFactoryStepSnapshot` is not called for step `S`

### Requirement: v2-rand-mods-per-step-snapshots

Rand Mods from the center cluster SHALL randomize mod source and depth into step snapshot fields per active sequencer scope, not live-global-only routing.

#### Scenario: Step scope writes one step snapshot

- **WHEN** Rand Mods is triggered with **Step** scope and sequencer is stopped
- **THEN** only `m_steps[m_editStep]` mod fields change

#### Scenario: All steps scope writes every step

- **WHEN** Rand Mods is triggered with **All steps** scope and pattern length is `L`
- **THEN** steps `0..L-1` each receive independent randomized mod fields

## MODIFIED Requirements

### Requirement: v2-rand-seq-step-scope-target

`FroggersV2ControlCore::onRandSequencerStep` with scope `kRandSeqScopeStep` SHALL select the target step from playback state:

- `targetStep = m_playhead` when `m_playing == true`
- `targetStep = m_editStep` when `m_playing == false`

Then the system SHALL randomize scene slots into `m_steps[targetStep]`, zero gestures, and set `hasData = true`. The `message.slot` parameter is ignored for step-index selection.

#### Scenario: Step scope targets playhead while playing

- **WHEN** `onRandSequencerStep` runs with `kRandSeqScopeStep`, `m_playing == true`, `m_playhead == 3`, and `m_editStep == 0`
- **THEN** only `m_steps[3]` is written

#### Scenario: Step scope targets edit step while stopped

- **WHEN** `onRandSequencerStep` runs with `kRandSeqScopeStep`, `m_playing == false`, `m_playhead == 3`, and `m_editStep == 0`
- **THEN** only `m_steps[0]` is written

### Requirement: v2-sequencer-control-core

The control core SHALL accept sequencer clock and step messages and apply per-step scene snapshots during playback. Step snapshots SHALL include mod routing fields per `v2-sequencer-snapshot-mod-fields`.

#### Scenario: Clock advances steps

- **WHEN** `MessageIn::SequencerStepClock` arrives at a beat boundary
- **THEN** the sequencer playhead advances and recalls the step's stored scene snapshot including mod routing

#### Scenario: Record captures step

- **WHEN** record arm is on and the user commits step N
- **THEN** current scene L/R centers, active gesture values, and mod routing are stored into step N
