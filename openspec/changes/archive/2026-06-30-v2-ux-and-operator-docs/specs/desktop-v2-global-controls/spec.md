## MODIFIED Requirements

### Requirement: v2-two-gesture-lanes

Desktop v2 SHALL provide **two** gesture lanes at launch with independent selection and value controls.

#### Scenario: Two gesture controls

- **WHEN** desktop v2 renders the performance band
- **THEN** Gesture 1 and Gesture 2 each have select/toggle and a horizontal weight slider (0.0–1.0)
- **THEN** adjusting a weight slider posts `MessageIn::GestureWeight` for that lane
- **THEN** `SetGestureCount(2)` is satisfied before parameter groups are created

#### Scenario: Gesture badges on encoder rings

- **WHEN** both gestures are active on a parameter
- **THEN** the encoder shows two gesture badges via `gesturesAffectingMask`

### Requirement: v2-three-scene-buttons

Scene controls SHALL be labeled **Scene** (not Module) and provide three buttons S1, S2, S3 plus a scene blend slider in the performance band.

#### Scenario: Scene S2 selects endpoint

- **WHEN** the user presses Scene S2
- **THEN** the control core updates the less-selected scene endpoint to ordinal 1 (zero-based index 1)
- **THEN** UI shows left/right scene labels from UIState

#### Scenario: Scene blend slider

- **WHEN** the user moves the scene blend slider to 0.5
- **THEN** `SetSceneBlend(0.5)` is applied and encoder rings reflect blended centers

### Requirement: v2-sequencer-global-strip

Sequencer transport controls (Play, Record arm, BPM, pattern length) SHALL be visible in the performance band. The global strip MAY omit duplicate sequencer controls once the performance band is active.

#### Scenario: Sequencer controls visible

- **WHEN** desktop v2 renders the performance band
- **THEN** BPM and pattern length are editable
- **THEN** sequencer record arm toggles step capture mode per `desktop-v2-sequencing`

### Requirement: v2-scene-gesture-lfo-vco-buttons

Gesture, LFO, and VCO buttons SHALL route through the message bus with **two** gesture lanes.

#### Scenario: Gesture toggle and value per lane

- **WHEN** the user toggles Gesture 1 or Gesture 2 and adjusts that lane's weight slider
- **THEN** gesture selection and value update UIState on the control thread for the active lane only

#### Scenario: LFO and VCO buttons wired

- **WHEN** the user clicks LFO or VCO in the global strip
- **THEN** the control core receives a bus message that updates scope/mod grid focus per design record
- **THEN** buttons are hidden if bus handlers are not implemented (no dead controls)
