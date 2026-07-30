## ADDED Requirements

### Requirement: v2-audio-transport-play-label

Standalone desktop v2 audio transport SHALL label the start-audio button **Play** and the stop-audio button **Stop**. The label **Engine** SHALL NOT appear on audio transport controls.

#### Scenario: Play starts audio engine

- **WHEN** the operator presses **Play** in the top transport row
- **THEN** `AudioEngine` starts and the button disables until **Stop**
- **THEN** button text reads **Play**, not **Engine**

#### Scenario: Sequencer transport remains distinct

- **WHEN** the operator presses **Start Sequence** in the sequencer toolbar
- **THEN** sequencer playback starts without renaming audio **Play**

### Requirement: v2-sequencer-rand-seq-scope-semantics

Rand-seq dice SHALL randomize **scene slots only** (same per-row `sceneCenter[0..2]` policy as carousel Rand All), invoke `randomizeSceneEndpointsAndBlend()` once per dice press, and write into step buffer(s) per scope:

- **Step** (`kRandSeqScopeStep`): edit step `m_editStep` only
- **All steps** (`kRandSeqScopePattern`): every index `i` in `0 .. m_patternLength-1`, **including steps with `hasData == true`**

Context-menu **Randomize** on a single step (`kRandSeqScopeFullStep`) SHALL continue to call `randomizeFullStepSnapshot` without blank-only skips.

#### Scenario: Step mode dice

- **WHEN** scope is **Step** and `m_editStep == 4`
- **THEN** step 4 receives a fresh randomized scene snapshot and `hasData = true`
- **THEN** all other step indices are unchanged

#### Scenario: All steps mode overwrites entire pattern

- **WHEN** scope is **All steps**, pattern length is 16, and steps 2 and 9 already have `hasData == true`
- **THEN** all 16 steps each receive independent randomized scene snapshots
- **THEN** previously stored data in steps 2 and 9 is replaced

#### Scenario: Rand-seq does not skip crispy rows in scene loop

- **WHEN** Rand-seq randomizes scene slots into a step buffer
- **THEN** crispy rows are skipped in the scene-slot loop per Rand All policy
- **THEN** Crunchy `sceneCenter[0..2]` in the step snapshot are randomized

#### Scenario: Rand-seq updates live L/R indicators

- **WHEN** Rand-seq dice completes in Step or All steps scope
- **THEN** performance band S1/S2/S3 suffixes and blend slider reflect `randomizeSceneEndpointsAndBlend()` output

## MODIFIED Requirements

### Requirement: v2-full-sequencer

Desktop v2 and VST v2 SHALL include a **full step sequencer** integrated with transport, scenes, and the control core.

#### Scenario: Sequencer UI visible

- **WHEN** desktop v2 or VST v2 editor is open
- **THEN** a sequencer panel shows step grid, pattern length, BPM, and playhead position in a toolbar adjacent to the grid
- **THEN** the panel is reachable from the global strip (Sequencer toggle or dedicated tab)

#### Scenario: Clock from transport

- **WHEN** **Start Sequence** is active and BPM is set in the sequencer toolbar
- **THEN** the sequencer advances steps on beat boundaries at the configured BPM
- **THEN** step changes publish `MessageIn::Clock` to the control core

#### Scenario: Per-step scene capture (Write Seq. playing)

- **WHEN** **Write Seq.** is armed, **Start Sequence** is pressed at playhead 0, and the operator tweaks knobs
- **THEN** step 0 receives a snapshot immediately on start and again when the first beat leaves step 0
- **WHEN** the playhead advances from step N to step N+1 on a beat
- **THEN** step N receives the live snapshot from that beat

#### Scenario: Per-step scene capture (Write Seq. stopped)

- **WHEN** **Write Seq.** is armed, edit step is 3, and the operator moves edit step to 4
- **THEN** step 3 stores the live snapshot before edit step becomes 4
- **THEN** live controls recall step 4

#### Scenario: Pattern length

- **WHEN** the user sets pattern length to 16 in the sequencer toolbar
- **THEN** the playhead cycles steps 0–15
- **THEN** supported lengths include at least 4, 8, 16, 32, and 64 steps

#### Scenario: Sequencer and scenes

- **WHEN** a step fires during playback
- **THEN** scene blend and endpoints update to the stored step state without corrupting unstored scene metadata outside the sequencer record buffer

#### Scenario: VST sequencer host parameters

- **WHEN** FroggersTigaPluginV2 is hosted
- **THEN** BPM, pattern length, **Write Seq.** arm, and current step are exposed as host parameters with flat stable IDs
- **THEN** grouped display names appear in DAW trees per `vst-v2-midi-modulation` dual-ID rules

#### Scenario: MIDI clock sync optional input

- **WHEN** external MIDI clock is enabled in v2 MIDI settings
- **THEN** sequencer step advance may follow incoming MIDI clock instead of internal BPM when sync mode is External

#### Scenario: Edit step always selected

- **WHEN** the sequencer panel is visible
- **THEN** exactly one edit step index in `0 .. patternLength-1` is selected at all times per `v2-sequencer-edit-step-always-selected`
