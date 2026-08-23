# desktop-v2-control-core Specification

## Purpose
Desktop v2 routes all UI and MIDI control through a message bus into `FroggersV2ControlCore`, which owns scene storage, modulation-depth drill-down, the shift interaction matrix, manifest-backed parameter resolution, and 16-step sequencer snapshots (including mod routing), and publishes lock-free `FroggersV2UIState` snapshots for the UI thread to read.
## Requirements
### Requirement: v2-control-core-message-bus
Desktop v2 SHALL route all UI and MIDI control through a message bus into `FroggersV2ControlCore` publishing lock-free `FroggersV2UIState` snapshots.

#### Scenario: Paint reads UIState only
- **WHEN** encoder components repaint
- **THEN** they read only from `FroggersV2UIState` atomics
- **THEN** no raw `Parameter` or `ModMgr` fields are read on the UI thread

#### Scenario: Message bus applies turns
- **WHEN** `ParamIncDec` arrives
- **THEN** the manager updates scene centers or depths per mod-view and shift context
- **THEN** `Compute` runs before the next `PopulateUIState`

### Requirement: v2-modulation-depth-drill-down
Pressing a parameter encoder while shift is not held SHALL open modulation-depth view per Sheaf bank semantics.

#### Scenario: Open and close mod view
- **WHEN** the user presses an assigned parameter cell without shift
- **THEN** visible cells become `[depth₀…depthₙ, target]`
- **WHEN** the user presses the target cell again
- **THEN** mod view closes

#### Scenario: Dropdown assigns source mod view edits depth
- **WHEN** the user assigns a mod source via row dropdown
- **THEN** the lit cell updates immediately
- **WHEN** the user opens mod view and drags a depth cell
- **THEN** bipolar depth for that source changes without using patch cables

### Requirement: v2-global-scene-storage
Scene endpoints (S1/S2/S3 ordinals with left/right selection and blend) SHALL be stored globally across all modules and parameters in the control core, independent of the active module carousel index.

#### Scenario: Scene persists across module change
- **WHEN** the user stores different scene L/R values on Filter and switches module to Reverb
- **THEN** returning to Filter retains prior scene L/R values

### Requirement: v2-shift-interaction-matrix
The control core SHALL implement the interaction matrix documented in `design.md` section 6 (normal view, mod view, shift held, Crunchy/ADSR Crispy exceptions).

#### Scenario: Shift suppresses encoder turns
- **WHEN** shift is held
- **THEN** `ParamIncDec` messages from encoders are ignored

#### Scenario: Revert clears depths
- **WHEN** shift+press revert fires on a parameter
- **THEN** base center and all modulation depths for that parameter reset to defaults

### Requirement: v2-scene-blend-and-two-gestures
The control core SHALL support three scene ordinals, scene blend, and **two** gesture lanes with arena sizing fixed before group creation.

#### Scenario: Scene blend affects center
- **WHEN** scene blend is 0.5 and left/right scene centers differ
- **THEN** blended center follows Smart Grid scene interpolation

#### Scenario: Two gesture lanes independent
- **WHEN** gesture 0 is selected on a parameter and gesture 1 is not
- **THEN** only gesture 0 effective weight applies to that parameter's center edit

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

