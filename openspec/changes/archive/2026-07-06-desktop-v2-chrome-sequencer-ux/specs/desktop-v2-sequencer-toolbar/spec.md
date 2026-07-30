## ADDED Requirements

### Requirement: v2-sequencer-toolbar-adjacent-to-grid

The sequencer panel SHALL render a toolbar row directly above the step grid containing clock, transport, Rand-seq controls, and edit-step navigation. BPM and Steps editors SHALL sit in this toolbar, not in `PerformanceBandV2`.

#### Scenario: BPM and Steps beside grid

- **WHEN** the sequencer panel is visible
- **THEN** BPM and Steps controls appear in the same horizontal band as prev/next edit-step arrows
- **THEN** the step grid is immediately below this toolbar

#### Scenario: Start Sequence in toolbar

- **WHEN** standalone desktop v2 shows the sequencer
- **THEN** **Start Sequence** and **Stop Sequence** buttons are in the sequencer toolbar
- **THEN** they are distinct from audio **Play** / **Stop** in the top transport row

### Requirement: v2-sequencer-write-seq-control

The sequencer panel toolbar SHALL expose a toggle labeled **Write Seq.** (not **Record**). It arms step snapshot capture into the step buffer. Audio file export uses the separate **Record audio** cluster per `desktop-v2-audio-export`.

#### Scenario: Write Seq distinct from audio Record

- **WHEN** standalone desktop v2 is visible
- **THEN** top transport row shows round red **Record audio** for audio export
- **THEN** sequencer toolbar shows **Write Seq.** for step capture
- **THEN** the two controls are not the same widget

#### Scenario: VST exposes write-seq as host parameter

- **WHEN** FroggersTigaPluginV2 is hosted
- **THEN** write-seq arm is available as a host parameter (display name **Write Seq.**)
- **THEN** the plugin editor shows **Write Seq.** in the sequencer toolbar (no audio Record row)

### Requirement: v2-sequencer-write-seq-stopped

When **Start Sequence** is off and **Write Seq.** is armed, changing edit step (prev/next arrow or single-click on grid) SHALL capture live scene state into the **previous** edit step before updating `m_editStep`, then recall the **new** edit step snapshot into live controls.

Disarming **Write Seq.** while stopped SHALL capture live state into the **current** edit step once (commit without navigating).

#### Scenario: Step programming while stopped

- **WHEN** **Write Seq.** is armed, edit step is 2, and the operator tweaks knobs then single-clicks step 5
- **THEN** `m_steps[2]` receives the live snapshot and `hasData = true`
- **THEN** `m_editStep` becomes 5
- **THEN** live controls recall `m_steps[5]` (or factory recall if `hasData == false`)

#### Scenario: Commit current step on disarm

- **WHEN** **Write Seq.** is armed, edit step is 4, and the operator toggles **Write Seq.** off without changing edit step
- **THEN** `m_steps[4]` receives the live snapshot and `hasData = true`

#### Scenario: Write Seq off does not save on navigate

- **WHEN** **Write Seq.** is off and the operator changes edit step
- **THEN** live snapshot is not written to the previous edit step
- **THEN** the new edit step snapshot is still recalled into live controls

### Requirement: v2-sequencer-write-seq-playing

When **Start Sequence** is on and **Write Seq.** is armed:

1. Pressing **Start Sequence** (transition to playing) SHALL immediately capture live state into `m_steps[m_playhead]`.
2. On each playhead beat advance, SHALL capture live state into the **step being left** (index `(m_playhead + patternLength - 1) % patternLength` **after** advance), not the step landed on.

After each capture (and on every advance regardless of arm state), live controls SHALL recall `m_steps[m_playhead]`.

#### Scenario: Step 0 captured on first beat

- **WHEN** pattern length is 16, playhead starts at 0, **Write Seq.** is armed, and the first beat advances playhead to 1
- **THEN** `m_steps[0]` receives the live snapshot from the beat on step 0
- **THEN** live controls recall `m_steps[1]`

#### Scenario: Step 0 captured on Start Sequence

- **WHEN** playhead is 0, **Write Seq.** is armed, and the operator presses **Start Sequence**
- **THEN** `m_steps[0]` immediately receives the current live snapshot before the first beat

#### Scenario: Wrap captures last step

- **WHEN** playhead advances from 15 to 0 on a 16-step pattern with **Write Seq.** armed
- **THEN** `m_steps[15]` receives the live snapshot from the beat on step 15

#### Scenario: Write Seq off during playback

- **WHEN** **Write Seq.** is off and the playhead advances
- **THEN** no step buffer is overwritten
- **THEN** live controls still recall the landed step snapshot

### Requirement: v2-sequencer-rand-seq-labeled-dice

The Rand-seq dice control SHALL display the text label **Rand-seq** adjacent to the dice icon (icon + label or label under icon). Tooltip SHALL read **Randomize sequencer steps (scene slots)**.

#### Scenario: Dice is identifiable

- **WHEN** the sequencer toolbar is visible
- **THEN** the operator can read **Rand-seq** without hovering
- **THEN** the control is not an unlabeled white icon only

### Requirement: v2-sequencer-scope-radio-buttons

Rand-seq scope SHALL be a mutually exclusive **radio button** pair:

- **Step** — randomize edit step only
- **All steps** — randomize every step index `0 .. patternLength-1`, overwriting existing `hasData` snapshots

Controls SHALL use `juce::ToggleButton` with a shared `radioGroupId` and radio-button appearance (not checkbox square styling). Default selection SHALL be **Step**.

#### Scenario: Step scope selected by default

- **WHEN** the sequencer panel is first shown
- **THEN** **Step** scope is selected
- **THEN** pressing dice randomizes `m_editStep` only

#### Scenario: All steps overwrites non-blank

- **WHEN** scope is **All steps**, pattern length is 16, and step 3 has `hasData == true` with stored scene data
- **THEN** pressing dice rewrites step 3 with a fresh randomized scene snapshot
- **THEN** all indices `0..15` receive independent randomized snapshots

#### Scenario: Radio mutual exclusion

- **WHEN** the operator selects **All steps**
- **THEN** **Step** deselects
- **WHEN** the operator selects **Step**
- **THEN** **All steps** deselects

### Requirement: v2-sequencer-edit-step-always-selected

Exactly one edit step SHALL always be selected in `SequencerState::m_editStep`. Default value SHALL be `0`. Navigation SHALL wrap within `0 .. patternLength-1`. There SHALL be no operator action or API that clears edit-step selection.

#### Scenario: Boot default edit step

- **WHEN** desktop v2 or VST v2 loads with pattern length ≥ 1
- **THEN** `m_editStep == 0`

#### Scenario: Pattern length change clamps edit step

- **WHEN** pattern length shrinks so `m_editStep >= patternLength`
- **THEN** `m_editStep` becomes `0`

#### Scenario: Arrow wrap preserves selection

- **WHEN** pattern length is 8 and `m_editStep == 0`
- **THEN** previous-step sets `m_editStep` to `7`
- **WHEN** `m_editStep == 7`
- **THEN** next-step sets `m_editStep` to `0`

#### Scenario: Grid click selects edit step

- **WHEN** the operator single-clicks step K with **Write Seq.** off
- **THEN** `m_editStep` becomes K and live controls recall step K
- **WHEN** **Write Seq.** is on, the previous edit step is captured per `v2-sequencer-write-seq-stopped`

#### Scenario: Audio Play without sequencer running

- **WHEN** audio **Play** is active and **Start Sequence** is off
- **THEN** `m_editStep` remains a valid index in `0 .. patternLength-1`
- **THEN** Rand-seq **Step** scope still targets `m_editStep`
