# desktop-v2-global-controls Specification

## Purpose
Desktop v2 provides global randomization, Crunchy, Shift, two gesture lanes, and three-scene controls, with keyboard- and MIDI-driven Shift and a Rand All scope that excludes scene storage, gesture metadata, and Crispy rows by default.
## Requirements
### Requirement: v2-global-strip-extended-controls
Desktop v2 SHALL provide global randomization actions, global **Crunchy**, and **Shift** in the **top chrome stack global-command band** (`GlobalStripV2`). Standalone desktop v2 SHALL NOT allocate a center-column `CenterGlobalClusterV2` or a bottom global strip row for these controls; see `desktop-v2-center-global-cluster` for the retired center-column placement.

#### Scenario: Preserved v1 randomization buttons
- **WHEN** desktop v2 renders the top chrome stack global-command band at 1280×920
- **THEN** Rand All, Rand Mods, Rand waveforms, and Rand Resample are present and invoke the same `DesktopHostIO` mutations as v1 (adapted for ADSR and global Crunchy)

#### Scenario: Crunchy encoder in global-command band
- **WHEN** desktop v2 renders the top chrome stack global-command band
- **THEN** a **Crunchy** encoder ring is visible and labeled **Crunchy**
- **THEN** it controls global fuegoization per `desktop-v2-encoder-rings` / global Crunchy spec

#### Scenario: Shift in global-command band
- **WHEN** desktop v2 renders the top chrome stack global-command band
- **THEN** Shift is visible and toggles shift-held semantics per `desktop-v2-global-controls` v2-shift-keyboard-and-midi

### Requirement: v2-two-gesture-lanes
Desktop v2 SHALL provide **two** gesture lanes at launch with independent selection and value controls, rendered in the performance band (`PerformanceBandV2`), not the top chrome global-command band.

#### Scenario: Two gesture controls
- **WHEN** desktop v2 renders the performance band
- **THEN** Gesture 1 and Gesture 2 each have select/toggle and value control (slider or encoder)
- **THEN** `SetGestureCount(2)` is satisfied before parameter groups are created

#### Scenario: Gesture badges on encoder rings
- **WHEN** both gestures are active on a parameter
- **THEN** the encoder shows two gesture badges via `gesturesAffectingMask`

### Requirement: v2-three-scene-buttons
Scene controls SHALL be labeled **Scene** (not Module) and provide three buttons S1, S2, S3 plus a scene blend slider, rendered in the performance band (`PerformanceBandV2`).

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

**Pending:** change `desktop-v2-operator-truth-repair` (capability `desktop-v2-randomization-authority`, packet 4) extends this requirement so Rand All and Rand Mods also consume the `All Scenes`/`Current Scene` and `All Steps`/`Current Step` scope radios in the global-command band, and so Rand Mods randomizes live mod depths rather than sequencer-snapshot-only state. This baseline requirement remains authoritative as written until that change archives; the scope-radio and live-mod-depth behavior described there is not yet implemented.

#### Scenario: Rand All skips scene storage
- **WHEN** Rand All completes
- **THEN** scene L/R stored centers for parameters remain unchanged
- **THEN** gesture selection state is cleared before randomization (if gesture was active)

#### Scenario: Rand All skips crispy rows by default
- **WHEN** Rand All runs with default policy
- **THEN** per-page Crispy rows (row 9 on expanded modules 1–5, row 9 on ADSR) are not randomized (matching v1 FUEG exclusion)
