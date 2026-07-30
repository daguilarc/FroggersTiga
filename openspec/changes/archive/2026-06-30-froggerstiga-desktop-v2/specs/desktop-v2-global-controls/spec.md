## ADDED Requirements

### Requirement: v2-global-strip-extended-controls
Desktop v2 SHALL provide a global control strip containing v1 randomization actions, global **Crunchy**, and Sheaf-style mode buttons.

#### Scenario: Preserved v1 randomization buttons
- **WHEN** desktop v2 renders the global strip
- **THEN** Rand All, Rand Mods, Rand waveforms, and Rand Resample are present and invoke the same `DesktopHostIO` mutations as v1 (adapted for ADSR and global Crunchy)

#### Scenario: Crunchy encoder in global strip
- **WHEN** desktop v2 renders the global strip
- **THEN** a **Crunchy** encoder ring is visible and labeled **Crunchy**
- **THEN** it controls global fuegoization per `desktop-v2-encoder-rings` / global Crunchy spec

#### Scenario: Mode buttons present
- **WHEN** desktop v2 renders the global strip
- **THEN** Shift, Gesture 1, Gesture 2, LFO, VCO, and Sequencer controls are visible alongside randomization and Crunchy

### Requirement: v2-two-gesture-lanes
Desktop v2 SHALL provide **two** gesture lanes at launch with independent selection and value controls.

#### Scenario: Two gesture controls
- **WHEN** desktop v2 renders the global strip
- **THEN** Gesture 1 and Gesture 2 each have select/toggle and value control (slider or encoder)
- **THEN** `SetGestureCount(2)` is satisfied before parameter groups are created

#### Scenario: Gesture badges on encoder rings
- **WHEN** both gestures are active on a parameter
- **THEN** the encoder shows two gesture badges via `gesturesAffectingMask`

### Requirement: v2-sequencer-global-strip
The global strip SHALL expose sequencer transport: Play/Stop (or shared transport), Record arm, BPM, and pattern length.

#### Scenario: Sequencer controls visible
- **WHEN** desktop v2 renders the global strip
- **THEN** BPM and pattern length are editable
- **THEN** sequencer record arm toggles step capture mode per `desktop-v2-sequencing`

### Requirement: v2-three-scene-buttons
Scene controls SHALL be labeled **Scene** (not Module) and provide three buttons S1, S2, S3 plus a scene blend slider.

#### Scenario: Scene S2 selects endpoint
- **WHEN** the user presses Scene S2
- **THEN** the control core updates the less-selected scene endpoint to ordinal 1 (zero-based index 1)
- **THEN** UI shows left/right scene labels from UIState

#### Scenario: Scene blend slider
- **WHEN** the user moves the scene blend slider to 0.5
- **THEN** `SetSceneBlend(0.5)` is applied and encoder rings reflect blended centers

### Requirement: v2-shift-keyboard-and-midi
Shift state SHALL toggle from the computer Shift key and from any MIDI-assignable button.

#### Scenario: Keyboard shift semantics
- **WHEN** Shift is held and the user presses a parameter encoder
- **THEN** revert-to-default applies per interaction matrix in design.md
- **WHEN** Shift is held
- **THEN** encoder drag turns are suppressed

#### Scenario: MIDI shift binding
- **WHEN** a MIDI button is bound to Shift
- **THEN** press/release maps to shift-held true/false

### Requirement: v2-rand-all-scope
Rand All SHALL randomize all module pages, ADSR rows 0–8, and global Crunchy per policy; it SHALL NOT mutate scene endpoint storage or gesture metadata.

#### Scenario: Rand All skips scene storage
- **WHEN** Rand All completes
- **THEN** scene L/R stored centers for parameters remain unchanged
- **THEN** gesture selection state is cleared before randomization (if gesture was active)

#### Scenario: Rand All skips crispy rows by default
- **WHEN** Rand All runs with default policy
- **THEN** per-page Crispy rows (row 9 on expanded modules 1–5, row 9 on ADSR) are not randomized (matching v1 FUEG exclusion)

### Requirement: v2-scene-gesture-lfo-vco-buttons
Gesture, LFO, and VCO buttons SHALL route through the message bus with **two** gesture lanes.

#### Scenario: Gesture toggle and value per lane
- **WHEN** the user toggles Gesture 1 or Gesture 2 and adjusts that lane's value control
- **THEN** gesture selection and value update UIState on the control thread for the active lane only
