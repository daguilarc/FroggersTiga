## MODIFIED Requirements

**Audit 2026-06-30:** Main spec `v2-rand-all-scope` incorrectly states Rand All leaves “scene L/R stored centers” unchanged. Code randomizes `sceneCenter[0..2]` (`onRandAll` L482–489) while preserving S1/S2/S3 ordinals + blend. Per-page **Randomize** is not implemented yet (`MainComponent.cpp` L86).

### Requirement: v2-rand-all-scope

Rand All SHALL randomize `sceneCenter[0..2]` and `modDepth[*]` for all musical rows on all module pages, skip `crispyRowForPage(page)` on every page, randomize **all three Crunchy scene slots**, **randomize L/R endpoint ordinals (distinct) and scene blend in [0, 1]**, and clear gesture selection before randomizing gesture weights.

Rand All SHALL NOT require preserving prior endpoint or blend values.

#### Scenario: Rand All rewrites stored scene positions and morph assignment

- **WHEN** Rand All completes
- **THEN** `sceneCenter[0]`, `sceneCenter[1]`, and `sceneCenter[2]` receive new values for each randomized row
- **THEN** `m_sceneLeftOrdinal`, `m_sceneRightOrdinal`, and `m_sceneBlend` receive new values per `randomize-scene-endpoints-and-blend`

#### Scenario: Rand All clears gesture selection

- **WHEN** Rand All completes
- **THEN** active gesture lane is cleared and gesture weights zeroed before row randomization

#### Scenario: Rand All skips crispy rows

- **WHEN** Rand All runs
- **THEN** `crispyRowForPage(page)` is skipped on every page (Audio row 7, expanded modules and ADSR row 9)

### Requirement: v2-per-page-randomize-scope

Carousel **Randomize** SHALL randomize all three `sceneCenter` slots for every musical row on the **active module page only**, using the same per-row scene loop as Rand All. It SHALL skip `crispyRowForPage(page)`. It SHALL NOT randomize mod depths (use **Rand mod**), **Crunchy scene slots**, or gesture state. It SHALL NOT change scene endpoint ordinals or blend.

On v2 hosts the control core is knob authority; host `EnqueueRandomizePanel` SHALL NOT be called.

#### Scenario: Per-page randomize matches rand-all scene policy on one page

- **WHEN** the operator clicks **Randomize** on the Filter module
- **THEN** `sceneCenter[0..2]` on Filter rows 0–8 are randomized (row 9 Crispy skipped)
- **THEN** Audio and other modules' `sceneCenter[*]` are unchanged
- **THEN** Filter mod depths are unchanged unless **Rand mod** is used separately

#### Scenario: Per-page randomize preserves endpoint metadata

- **WHEN** per-page Randomize completes
- **THEN** S1/S2/S3 ordinals and scene blend are unchanged

## ADDED Requirements

**Merged from `v2-ux-and-operator-docs` (2026-06-30).** Performance-band gesture/scene/sequencer controls landed in tasks §4; label polish remains in Phase C.

### Requirement: v2-two-gesture-lanes

Desktop v2 SHALL provide **two** gesture lanes at launch with independent selection and value controls.

#### Scenario: Two gesture controls

- **WHEN** desktop v2 renders the performance band
- **THEN** Gesture 1 and Gesture 2 each have select/toggle and a horizontal weight slider (0.0–1.0)
- **THEN** adjusting a weight slider posts `MessageIn::GestureWeight` for that lane

#### Scenario: Gesture badges on encoder rings

- **WHEN** both gestures are active on a parameter
- **THEN** the encoder shows two gesture badges via `gesturesAffectingMask`

### Requirement: v2-three-scene-buttons

Scene controls SHALL provide three buttons S1, S2, S3 plus a scene blend slider in the performance band.

#### Scenario: Scene S2 selects endpoint

- **WHEN** the user presses Scene S2
- **THEN** the control core updates the less-selected scene endpoint to ordinal 1 (zero-based index 1)

#### Scenario: Scene blend slider

- **WHEN** the user moves the scene blend slider to 0.5
- **THEN** `SetSceneBlend(0.5)` is applied and encoder rings reflect blended centers

### Requirement: v2-sequencer-performance-band

Sequencer transport controls (Start Sequence, Record arm, BPM, pattern length) SHALL be visible in the performance band. The global strip SHALL omit duplicate sequencer controls.

#### Scenario: Sequencer controls in performance band

- **WHEN** desktop v2 renders the performance band
- **THEN** BPM and pattern length are editable
- **THEN** sequencer record arm toggles step capture mode per `desktop-v2-sequencing`

### Requirement: v2-scene-gesture-lfo-vco-buttons

Gesture buttons SHALL route through the message bus with **two** gesture lanes. LFO and VCO buttons SHALL be hidden until bus handlers exist (no dead controls).

#### Scenario: Gesture toggle and value per lane

- **WHEN** the user toggles Gesture 1 or Gesture 2 and adjusts that lane's weight slider
- **THEN** gesture selection and value update UIState on the control thread for the active lane only

### Requirement: v2-crunchy-encoder-ring

The global strip **Crunchy** control SHALL be an `EncoderRingComponent` with scene arcs, not a plain rotary slider. It SHALL participate in the scene system per `desktop-v2-scene-core-parity` requirement `crunchy-scene-encoder-parity`.

#### Scenario: Crunchy ring in global strip

- **WHEN** desktop v2 or VST v2 renders `GlobalStripV2`
- **THEN** Crunchy appears as a labeled **5u×5u** encoder ring adjacent to Rand / Shift controls
- **THEN** drag on the ring posts `ParamTurn(kNumHostPages, 0, delta)` and edits Crunchy scene slots
