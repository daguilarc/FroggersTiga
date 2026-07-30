## ADDED Requirements

### Requirement: v2-rand-seq-pattern-all-steps

`FroggersV2ControlCore::onRandSequencerStep` with scope `kRandSeqScopePattern` SHALL iterate `i = 0 .. m_patternLength-1` and randomize scene slots into `m_steps[i]` for **every** `i`, without skipping steps where `hasData == true`. Each written step SHALL set `hasData = true` and zero gestures in the snapshot per Rand-seq policy.

#### Scenario: Pattern scope no blank-only skip

- **WHEN** `onRandSequencerStep` runs with `kRandSeqScopePattern`, pattern length 4, and step 3 has `hasData == true` with `sceneCenter[0][0][0] == 0.42f`
- **THEN** step 3 `sceneCenter` values change after the call
- **THEN** steps 0, 1, and 2 are also randomized

#### Scenario: Pattern scope calls endpoint randomize once

- **WHEN** `onRandSequencerStep` runs with `kRandSeqScopePattern`
- **THEN** `randomizeSceneEndpointsAndBlend()` runs exactly once before the step loop
- **THEN** live S1/S2/S3 ordinals and blend update

#### Scenario: Step scope unchanged target

- **WHEN** `onRandSequencerStep` runs with `kRandSeqScopeStep`
- **THEN** only `m_steps[m_editStep]` is written
- **THEN** `message.slot` is ignored for step-index selection (edit step comes from `m_sequencer->m_editStep`)

### Requirement: v2-sequencer-write-seq-capture

`FroggersV2HostBridge` and `SequencerPanelComponent` SHALL implement Write Seq. capture per `desktop-v2-sequencer-toolbar`:

- Stopped: capture on edit-step change (previous step) and on disarm (current step)
- Playing: capture on Start Sequence to `m_playhead`; on advance capture to step left `(m_playhead + patternLength - 1) % patternLength`
- Always recall landed/current edit step after navigation or advance

#### Scenario: Step-left capture after advance

- **WHEN** playhead advances from 0 to 1 with Write Seq. armed
- **THEN** `captureSequencerStepSnapshot` writes into `m_steps[0]`, not `m_steps[1]`

#### Scenario: Start Sequence immediate capture

- **WHEN** playhead is 0, Write Seq. is armed, and Start Sequence begins
- **THEN** `m_steps[0]` receives a snapshot before the first beat advance

## MODIFIED Requirements

### Requirement: v2-sequencer-control-core

The control core SHALL accept sequencer clock and step messages and apply per-step scene snapshots during playback.

#### Scenario: Clock advances steps

- **WHEN** `MessageIn::Clock` arrives at a beat boundary
- **THEN** the sequencer playhead advances and recalls the step's stored scene snapshot

#### Scenario: Rand-seq pattern overwrites all pattern steps

- **WHEN** `MessageIn::RandSequencerStep` arrives with scope `kRandSeqScopePattern`
- **THEN** all steps in `0 .. patternLength-1` are randomized per `v2-rand-seq-pattern-all-steps`
