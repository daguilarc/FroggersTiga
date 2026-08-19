# desktop-v2-sequencing Specification

## Purpose
Desktop v2 and VST v2 fixed 16-step sequencer integrated with transport, scenes, manifest-owned snapshots/locks, direction/speed icon strip, and long-press clear.

## Requirements
### Requirement: v2-full-sequencer
Desktop v2 and VST v2 SHALL include a **full step sequencer** integrated with transport, scenes, and the control core. Step cells SHALL be at least **3u** wide so two-digit step numbers render without ellipsis at 1280×920.

#### Scenario: Sequencer UI visible
- **WHEN** desktop v2 or VST v2 editor is open
- **THEN** a sequencer panel shows step grid and playhead position
- **THEN** the panel is reachable from the sequencer toolbar (not the removed bottom global strip)
- **THEN** no pattern-length or step-count control is exposed

#### Scenario: Clock from transport
- **WHEN** transport Play is active and BPM is set
- **THEN** the sequencer advances steps on beat boundaries at the configured BPM
- **THEN** step changes publish `MessageIn::SequencerStepClock` to the control core

#### Scenario: Per-step scene capture
- **WHEN** the user records into step N while Write Seq. is armed
- **THEN** step N stores a snapshot of scene L/R centers (and gesture values if active) for all parameters in the current module scope or global scope per design record mode
- **THEN** playback recalls that snapshot when the playhead enters step N

#### Scenario: Fixed sixteen-step ring
- **WHEN** the sequencer initializes, loads state, or receives a controller/runtime command
- **THEN** it contains exactly 16 step slots indexed 0..15
- **THEN** the playhead cycles the fixed 16-slot ring according to direction and speed while skipping unwritten slots

#### Scenario: Sequencer and scenes
- **WHEN** a step fires during playback
- **THEN** scene blend and endpoints update to the stored step state without corrupting unstored scene metadata outside the sequencer record buffer

#### Scenario: VST sequencer host parameters
- **WHEN** FroggersTigaPluginV2 is hosted
- **THEN** BPM, play/record arm, and current step are exposed as host parameters with flat stable IDs
- **THEN** grouped display names appear in DAW trees per `froggers-v2-app-manifest`'s dual-identity rules (stable ID plus grouped display name; this scenario cited `vst-v2-midi-modulation` until that capability was retired with the untested v2 plugin wrapper, which owned no rule the manifest does not)

#### Scenario: MIDI clock sync optional input
- **WHEN** external MIDI clock is enabled in v2 MIDI settings
- **THEN** sequencer step advance follows incoming MIDI clock instead of internal BPM when sync mode is External

#### Scenario: Two-digit step labels
- **WHEN** the sequencer panel renders at 1280×920
- **THEN** step buttons 10–16 display full numbers, not ellipsis

### Requirement: Sequencer has exactly sixteen steps
Desktop v2 SHALL expose exactly 16 sequencer slots. The sequencer slot count SHALL be fixed by the product contract and SHALL NOT be user-editable through a pattern-length, step-count, or hidden advanced control. Each slot SHALL have a written/unwritten state.

#### Scenario: Fixed step count is enforced
- **WHEN** the sequencer initializes, loads state, or receives a controller/runtime command
- **THEN** it contains exactly 16 step slots
- **THEN** step addressing is clamped or rejected outside the range 0..15
- **THEN** no UI projection exposes a step-count or pattern-length editor

### Requirement: Sequencer direction and speed use a two-row icon strip
Desktop v2 SHALL render a compact two-row icon strip above the 16-step sequencer grid. The first row SHALL select direction from `<`, `>`, and `RND`; the second row SHALL select speed from `/2`, `/1.5`, `1`, `x1.5`, and `x2`.

#### Scenario: Direction controls are visible and exclusive
- **WHEN** the sequencer renders
- **THEN** `<`, `>`, and `RND` are visible in the direction row
- **THEN** exactly one direction is selected
- **THEN** `>` is selected by default for a new patch or unsaved sequencer state

#### Scenario: Speed controls are visible and exclusive
- **WHEN** the sequencer renders
- **THEN** `/2`, `/1.5`, `1`, `x1.5`, and `x2` are visible in the speed row
- **THEN** exactly one speed is selected
- **THEN** `1` is selected by default for a new patch or unsaved sequencer state

#### Scenario: Direction and speed do not alter step count
- **WHEN** the user changes direction or speed
- **THEN** the sequencer still contains exactly 16 step slots
- **THEN** existing step snapshots and locks remain assigned to their original step indices

### Requirement: Cleared steps are skipped by playback
Desktop v2 SHALL let the user clear a written step directly with a device-neutral long press on that step. Mouse press-and-hold, touch press-and-hold, and holding a mapped MIDI/controller step control SHALL emit the same step-local clear command after the long-press threshold, with no second click, menu selection, or controller confirm. Releasing before the threshold SHALL cancel the clear. This long press SHALL resolve to the addressed step and SHALL NOT create a held modifier for future parameter targets, randomization actions, Crunchy, Crispy, or MIDI/controller routing. Clearing a step SHALL mark the slot unwritten and remove its saved state. Playback SHALL skip unwritten slots instead of applying silence, reset, or default parameter values.

#### Scenario: Long press clears a written step
- **WHEN** the user long-presses a written step by mouse, touch, or mapped MIDI/controller step control
- **AND** the hold reaches the long-press threshold
- **THEN** the step is marked unwritten
- **THEN** its saved snapshot and lock values are cleared

#### Scenario: Short hold does not clear a written step
- **WHEN** the user presses and releases a written step before the long-press threshold
- **THEN** the step remains written
- **THEN** its saved snapshot and lock values remain unchanged

#### Scenario: Written-step mask creates odd effective lengths
- **WHEN** fewer than 16 steps are written
- **THEN** playback traverses the fixed 16-slot ring according to direction and speed
- **THEN** it applies only written slots
- **THEN** the audible effective sequence length is the written-step pattern, including odd counts

#### Scenario: Empty written-step mask is transport no-op
- **WHEN** every step is unwritten
- **AND** sequencer playback is running
- **THEN** the sequencer emits no snapshot recalls, lock recalls, gates, silence, reset, or default-value events
- **THEN** the synth audio engine continues processing the current live parameter state
- **THEN** this behavior matches pressing play before any sequencer steps have been recorded

### Requirement: Fixed sequencer controls fit the app screen
At the default standalone size of 1280x920, the two-row icon strip and all 16 sequencer step slots SHALL be visible without truncation, overlap, or scrolling.

#### Scenario: App screen layout includes the full sequencer
- **WHEN** desktop v2 lays out the app screen at 1280x920
- **THEN** the direction row, speed row, and all 16 step cells fit inside the sequencer region
- **THEN** none of the labels `<`, `>`, `RND`, `/2`, `/1.5`, `1`, `x1.5`, or `x2` is truncated
- **THEN** the sequencer region does not overlap the module parameter grid, parameter-detail grid, global controls, global top-chrome oscilloscope, runtime buttons, or carousel header

### Requirement: v2-sequencer-gate-for-adsr
The sequencer SHALL provide a per-step or global gate signal usable by `VcoAdsrState` gated ADSR envelopes.

#### Scenario: Step gate for ADSR
- **WHEN** sequencer playback is active and a step defines gate on
- **THEN** ADSR envelopes receive gate high for the step duration
- **WHEN** gate is off for a step
- **THEN** ADSR envelopes receive gate low and release per release times

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
- **THEN** for every step `i` in `0..15` the system sets written state for targeted steps per scope policy
- **THEN** every targeted step `i` receives randomized scene slot centers and randomized crunchy scene centers
- **THEN** step gesture weights are cleared by the scope policy (Step/All steps scope => gestures cleared)
- **THEN** step gate is not randomized by this scope (gate remains whatever value was present in the step snapshot)

#### Scenario: Step-scope randomize payload
- **WHEN** `Rand-seq` is triggered with `Step` scope for a target step index `S`
- **THEN** step `S` receives randomized scene slot centers and randomized crunchy scene centers
- **THEN** written state for step `S` is set per scope policy
- **THEN** step `S` gesture weights are cleared by the scope policy (Step scope => gestures cleared)
- **THEN** step gate is not randomized by this scope

#### Scenario: Full-step context randomize payload
- **WHEN** the operator triggers full-step randomize (context menu "Randomize" for a specific step)
- **THEN** only that specific step snapshot is overwritten
- **THEN** the overwritten step is marked written
- **THEN** step gesture weights are randomized and step gate is randomized

#### Scenario: Rand Mods per-step in All-steps scope
- **WHEN** operator triggers `Rand Mods` with **All steps** sequencer scope active
- **THEN** every written step in `0..15` receives independent randomized mod sources/depths in that step's snapshot
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
- **WHEN** Write Seq. is armed and the operator starts sequence then lets three beats elapse
- **THEN** steps 0, 1, and 2 each are written after the third beat advance
- **THEN** step 0 was not overwritten by a duplicate capture on the first beat advance

### Requirement: v2-write-seq-capture-feedback
The sequencer UI SHALL provide visible feedback when a step snapshot is captured while Write Seq. is armed.

#### Scenario: Flash on capture
- **WHEN** `captureLiveToSequencerStep(N)` completes while Write Seq. is armed
- **THEN** step cell N shows a brief visual capture indicator (flash or pulse)

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
- **THEN** the action overwrites all 16 steps, independent of `m_editStep` and `m_playhead`

### Requirement: v2-rand-mods-target-selection
When using Rand Mods from the global-command band, target step selection SHALL mirror `v2-rand-seq-target-selection-when-playing`.

#### Scenario: Rand Mods Step scope targets playhead while playing
- **WHEN** sequencer playback is running (playing == true) and Rand Mods is triggered with **Step** scope
- **THEN** only `m_steps[m_playhead]` mod fields are randomized

#### Scenario: Rand Mods Step scope targets edit step while stopped
- **WHEN** sequencer playback is stopped (playing == false) and Rand Mods is triggered with **Step** scope
- **THEN** only `m_steps[m_editStep]` mod fields are randomized

#### Scenario: Rand Mods All-steps scope writes full pattern
- **WHEN** Rand Mods is triggered with **All steps** scope
- **THEN** every step in `0..15` receives independent randomized mod fields

### Requirement: v2-story-decision-variant-matrix
The spec SHALL include a decision matrix for story variants so behavior can be iterated multiplicatively without ambiguity.

#### Scenario: Decision matrix enumerated
- **WHEN** planning or QA references Story 1/2/3
- **THEN** the matrix includes at least these axes: `audio-start-order` (alias `sequencer-start-order`), `randomization-scope`, `randomization-target-policy`, `write-target-policy`, and `mod-randomization-policy`
- **THEN** each accepted variant maps to a deterministic expected outcome and test case
