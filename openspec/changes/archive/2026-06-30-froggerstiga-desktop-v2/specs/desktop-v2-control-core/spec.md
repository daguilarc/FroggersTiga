## ADDED Requirements

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

### Requirement: v2-sequencer-control-core
The control core SHALL accept sequencer clock and step messages and apply per-step scene snapshots during playback.

#### Scenario: Clock advances steps
- **WHEN** `MessageIn::Clock` arrives at a beat boundary
- **THEN** the sequencer playhead advances and recalls the step's stored scene snapshot

#### Scenario: Record captures step
- **WHEN** record arm is on and the user commits step N
- **THEN** current scene L/R centers (and active gesture values) are stored into step N
