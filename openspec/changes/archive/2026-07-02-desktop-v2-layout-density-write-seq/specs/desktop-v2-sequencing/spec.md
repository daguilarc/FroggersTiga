## ADDED Requirements

### Requirement: v2-story-coverage-sequencer-off-and-on

The implementation SHALL support three operator stories as first-class acceptance paths: (1) play-only with sequencer off, (2) sequencer clock enabled before audio start, and (3) audio start before sequencer start with step-wise randomization.

#### Scenario: Story 1 play-only

- **WHEN** the operator starts audio with `Play` and never enables sequencer playback
- **THEN** all non-sequencer features remain fully usable (scene, randomize, crunchy, mod routing, audio export)
- **THEN** no sequencer snapshot capture occurs

#### Scenario: Story 2 sequence-before-audio

- **WHEN** sequencer playback is enabled before audio is started, then audio starts later
- **THEN** sequencer state remains valid and resumes advancing/capturing once audio clock runs
- **THEN** randomize actions from sequencer scope are applied to sequence snapshots according to `v2-sequence-randomization-scope-policy`

#### Scenario: Story 3 audio-before-sequence

- **WHEN** audio starts first, then sequencer starts, then step-wise randomization is triggered
- **THEN** randomization applies to the selected step scope without corrupting unrelated steps
- **THEN** dice step-scope targeting remains deterministic for steps beyond index 1

### Requirement: v2-sequence-randomization-scope-policy

Sequence randomization behavior SHALL define the deterministic payload written into step snapshots for each scope. Step snapshots SHALL include **per-step mod source assignments and mod depths** (same shape as live `ParamState` mod fields), captured on Write Seq. and restored on step recall.

#### Scenario: All-steps randomize payload

- **WHEN** `Rand-seq` is triggered with `All steps` scope
- **THEN** for every step `i` in `0..patternLength-1` the system sets `hasData = true`
- **THEN** every step `i` receives randomized scene slot centers and randomized crunchy scene centers
- **THEN** step gesture weights are cleared by the scope policy (Step/All steps scope => gestures cleared)
- **THEN** step gate is not randomized by this scope (gate remains whatever value was present in the step snapshot)

#### Scenario: Step-scope randomize payload

- **WHEN** `Rand-seq` is triggered with `Step` scope for a target step index `S`
- **THEN** step `S` receives randomized scene slot centers and randomized crunchy scene centers
- **THEN** `hasData` for step `S` is set to true
- **THEN** step `S` gesture weights are cleared by the scope policy (Step scope => gestures cleared)
- **THEN** step gate is not randomized by this scope

#### Scenario: Full-step context randomize payload

- **WHEN** the operator triggers full-step randomize (context menu “Randomize” for a specific step)
- **THEN** only that specific step snapshot is overwritten
- **THEN** the overwritten step has `hasData = true`
- **THEN** step gesture weights are randomized and step gate is randomized

#### Scenario: Rand Mods per-step in All-steps scope

- **WHEN** operator triggers `Rand Mods` with **All steps** sequencer scope active
- **THEN** every step in `0..patternLength-1` receives independent randomized mod sources/depths in that step's snapshot
- **THEN** playback of step `N` recalls step `N`'s mod routing, not a single global route

#### Scenario: Rand Mods per-step in Step scope

- **WHEN** operator triggers `Rand Mods` with **Step** scope targeting step `S`
- **THEN** only step `S`'s snapshot mod fields are randomized
- **THEN** other steps retain prior mod assignments

#### Scenario: Write Seq. captures mod routing

- **WHEN** Write Seq. captures step `N`
- **THEN** the snapshot includes current mod source and depth for every page/row in the snapshot model
- **WHEN** playhead enters step `N`
- **THEN** live mod routing matches step `N`'s stored mod fields

#### Scenario: Scene endpoint/blend update is global per randomize

- **WHEN** `Rand-seq` is triggered with `Step` or `All steps` scope
- **THEN** global scene endpoints and global scene blend are randomized once per user trigger
- **THEN** subsequent playback uses the new global scene endpoints when applying step snapshots

### Requirement: v2-write-seq-playing-edit-step-follow

When Write Seq. is armed and the sequencer is playing, `m_editStep` SHALL equal `m_playhead` after Start Sequence and after each beat advance.

#### Scenario: Edit highlight tracks playhead while writing

- **WHEN** Write Seq. is armed and Start Sequence is pressed
- **THEN** step 0 shows edit-step highlight
- **WHEN** the playhead advances to step 2
- **THEN** step 2 shows edit-step highlight

### Requirement: v2-write-seq-no-duplicate-step-zero

When Write Seq. is armed, Start Sequence SHALL capture step 0 once. The first beat advance after start SHALL NOT capture step 0 a second time.

#### Scenario: Three beats produce three distinct captures

- **WHEN** Write Seq. is armed, pattern length is 16, and the operator starts sequence then lets three beats elapse
- **THEN** steps 0, 1, and 2 each have `hasData` after the third beat advance
- **THEN** step 0 was not overwritten by a duplicate capture on the first beat advance

### Requirement: v2-write-seq-capture-feedback

The sequencer UI SHALL provide visible feedback when a step snapshot is captured while Write Seq. is armed.

#### Scenario: Flash on capture

- **WHEN** `captureLiveToSequencerStep(N)` completes while Write Seq. is armed
- **THEN** step cell N shows a brief visual capture indicator (flash or pulse)

### Requirement: v2-step-factory-default-seeding-for-blank-steps

When sequencer playback advances into a step whose snapshot has `hasData = false`, the system SHALL treat that step as factory defaults.

#### Scenario: Blank step uses factory defaults

- **WHEN** sequencer playback advances into step `S` whose `hasData` is false
- **THEN** the system sets `m_steps[S]` to the factory step payload equivalent to `captureFactoryStepSnapshot(m_steps[S])`
- **THEN** subsequent application of step `S` writes factory default scene centers into the control core

#### Scenario: Written step overrides factory defaults

- **WHEN** step `S` already has `hasData = true`
- **THEN** playback applies the stored step snapshot payload, not factory defaults

### Requirement: v2-rand-seq-target-selection-when-playing

When using `Rand-seq` dice, the targeted step for `Step` scope SHALL be deterministic across playback state.

#### Scenario: Dice Step scope targets playhead while playing

- **WHEN** sequencer playback is running (playing == true)
- **THEN** a `Rand-seq` dice action in `Step` scope targets `m_playhead`, not `m_editStep`

#### Scenario: Dice Step scope targets edit step while stopped

- **WHEN** sequencer playback is stopped (playing == false)
- **THEN** a `Rand-seq` dice action in `Step` scope targets `m_editStep`

#### Scenario: Dice All-steps scope overwrites full pattern

- **WHEN** a `Rand-seq` dice action is triggered in `All steps` scope
- **THEN** the action overwrites all steps in `0..patternLength-1`, independent of `m_editStep` and `m_playhead`

### Requirement: v2-rand-mods-target-selection

When using Rand Mods from the center cluster, target step selection SHALL mirror `v2-rand-seq-target-selection-when-playing`.

#### Scenario: Rand Mods Step scope targets playhead while playing

- **WHEN** sequencer playback is running (playing == true) and Rand Mods is triggered with **Step** scope
- **THEN** only `m_steps[m_playhead]` mod fields are randomized

#### Scenario: Rand Mods Step scope targets edit step while stopped

- **WHEN** sequencer playback is stopped (playing == false) and Rand Mods is triggered with **Step** scope
- **THEN** only `m_steps[m_editStep]` mod fields are randomized

#### Scenario: Rand Mods All-steps scope writes full pattern

- **WHEN** Rand Mods is triggered with **All steps** scope
- **THEN** every step in `0..patternLength-1` receives independent randomized mod fields

## MODIFIED Requirements

### Requirement: v2-full-sequencer

Desktop v2 and VST v2 SHALL include a **full step sequencer** integrated with transport, scenes, and the control core. Step cells SHALL be at least **3u** wide so two-digit step numbers render without ellipsis at 1280×920.

#### Scenario: Sequencer UI visible

- **WHEN** desktop v2 or VST v2 editor is open
- **THEN** a sequencer panel shows step grid, pattern length, and playhead position
- **THEN** the panel is reachable from the sequencer toolbar (not the removed bottom global strip)

#### Scenario: Clock from transport

- **WHEN** transport Play is active and BPM is set
- **THEN** the sequencer advances steps on beat boundaries at the configured BPM
- **THEN** step changes publish `MessageIn::SequencerStepClock` to the control core

#### Scenario: Per-step scene capture

- **WHEN** the user records into step N while Write Seq. is armed
- **THEN** step N stores a snapshot of scene L/R centers (and gesture values if active) for all parameters in the current module scope or global scope per design record mode
- **THEN** playback recalls that snapshot when the playhead enters step N

#### Scenario: Pattern length

- **WHEN** the user sets pattern length to 16
- **THEN** the playhead cycles steps 0–15
- **THEN** supported lengths include at least 4, 8, 16, 32, and 64 steps

#### Scenario: Sequencer and scenes

- **WHEN** a step fires during playback
- **THEN** scene blend and endpoints update to the stored step state without corrupting unstored scene metadata outside the sequencer record buffer

#### Scenario: VST sequencer host parameters

- **WHEN** FroggersTigaPluginV2 is hosted
- **THEN** BPM, pattern length, play/record arm, and current step are exposed as host parameters with flat stable IDs
- **THEN** grouped display names appear in DAW trees per `vst-v2-midi-modulation` dual-ID rules

#### Scenario: MIDI clock sync optional input

- **WHEN** external MIDI clock is enabled in v2 MIDI settings
- **THEN** sequencer step advance follows incoming MIDI clock instead of internal BPM when sync mode is External

#### Scenario: Two-digit step labels

- **WHEN** pattern length is 16 and the sequencer panel renders at 1280×920
- **THEN** step buttons 10–16 display full numbers, not ellipsis

### Requirement: v2-story-decision-variant-matrix

The spec SHALL include a decision matrix for story variants so behavior can be iterated multiplicatively without ambiguity.

#### Scenario: Decision matrix enumerated

- **WHEN** planning or QA references Story 1/2/3
- **THEN** the matrix includes at least these axes: `audio-start-order` (alias `sequencer-start-order`), `randomization-scope`, `randomization-target-policy`, `write-target-policy`, and `mod-randomization-policy`
- **THEN** each accepted variant maps to a deterministic expected outcome and test case
