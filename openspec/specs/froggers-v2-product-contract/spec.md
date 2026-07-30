# froggers-v2-product-contract Specification

## Purpose
Froggers v2 product contract: three cross-coupled VCOs, Envelope page, 15-lane modulation rack, fixed 16-step sequencer, global randomization scope pairs, and no general held-gesture model.

## Requirements
### Requirement: Froggers v2 synth product contract
Froggers v2 SHALL preserve the Froggers synth shape: three cross-coupled VCOs, external-input ring modulation or continuous oscillator operation, module pages for Audio/VCO, Envelope, Filter, Distortion, Random/Marbles, Reverb, and Delay, per-parameter modulation/randomization/scene behavior, and sequencer-owned parameter locks on a fixed 16-step sequencer.

#### Scenario: Audio/VCO page is the default module
- **WHEN** desktop v2 launches
- **THEN** the Audio/VCO module page is selected
- **THEN** the page exposes cross-coupler controls for VCO 1/2 and VCO 2/3

#### Scenario: External input and continuous oscillator modes remain available
- **WHEN** the user configures the VCO/audio mode
- **THEN** each VCO can participate in the Froggers external-input ring-modulation path
- **THEN** each VCO can also run as a continuous oscillator according to the v2 audio mode contract

### Requirement: Envelope and waveform morph controls participate in v2 modulation behavior
Froggers v2 SHALL add an Envelope page after Audio/VCO with attack/release pairs for VCO 1, VCO 2, and VCO 3, and SHALL expose a continuous waveform morph control for each VCO.

#### Scenario: Envelope page appears after Audio/VCO
- **WHEN** the user advances one carousel page from Audio/VCO
- **THEN** the Envelope page is selected
- **THEN** it exposes VCO 1 attack/release, VCO 2 attack/release, and VCO 3 attack/release controls

#### Scenario: Waveform morph controls are full v2 targets
- **WHEN** the user selects a VCO waveform morph control
- **THEN** it can receive modulation assignments declared by the manifest
- **THEN** it can be randomized
- **THEN** it can participate in Crispy/Crunchy actions
- **THEN** it can be captured, locked, and recalled by scenes and sequencer snapshots when marked eligible

### Requirement: LFO module participates in source and target modulation
Froggers v2 SHALL expose LFO 1, LFO 2, and LFO 3 as first-class module outputs in the permanent modulation source rack. The LFO module SHALL expose manifest-owned parameters that can themselves receive modulation when eligible.

#### Scenario: LFO output is available as a source
- **WHEN** the user opens modulation detail for an eligible parameter
- **THEN** LFO 1, LFO 2, and LFO 3 appear as permanent source lanes
- **THEN** each LFO lane contributes no modulation while its depth is zero

#### Scenario: LFO parameters can be modulated
- **WHEN** the user opens modulation detail for an eligible LFO module parameter
- **THEN** VCO, EF, LFO, Random/Marbles, and available external-audio source lanes can be used according to manifest eligibility

### Requirement: Held gestures are not part of Froggers v2
Froggers v2 SHALL NOT implement press-and-hold gesture routing for Randomize, Randomize Mod, Crunchy, Crispy, MIDI/controller input, or parameter targeting. Randomize All and Randomize Mod SHALL act as explicit commands using their visible scope controls. Locked parameter values SHALL live in the clocked sequencer snapshot/lock model.

#### Scenario: Randomization does not wait for a later gesture target
- **WHEN** the user triggers Randomize All or Randomize Mod
- **THEN** the command resolves its target set from the visible scene and step scope controls
- **THEN** it does not enter a held state waiting for the next parameter, source lane, or Crunchy click

#### Scenario: Locked values are sequencer-owned
- **WHEN** a parameter value is locked for a step
- **THEN** the step is marked written
- **THEN** the lock is stored in the sequencer snapshot/lock fields declared by the manifest
- **THEN** the clocked sequencer applies that locked value when the owning step is active
- **THEN** no MIDI/controller gesture route is required to hold or recall the locked value

#### Scenario: MIDI clock can drive sequencer timing without gesture routing
- **WHEN** a MIDI clock source is configured for the sequencer
- **THEN** the sequencer can advance according to that clock source
- **THEN** MIDI clock does not create parameter gestures, held randomization state, or a private modulation route

### Requirement: Sequencer length and performance controls are fixed
Froggers v2 SHALL expose exactly 16 sequencer steps. The user SHALL NOT be able to change the sequencer step count or pattern length. Direction and speed SHALL be selected through a compact two-row icon strip above the sequencer: direction icons `<`, `>`, and `RND`; speed icons `/2`, `/1.5`, `1`, `x1.5`, and `x2`.

#### Scenario: Sequencer exposes sixteen immutable steps
- **WHEN** the sequencer state is initialized or loaded from a patch
- **THEN** exactly 16 step snapshots exist
- **THEN** no user-facing pattern-length or step-count control is available
- **THEN** `All Steps` resolves to all 16 steps
- **THEN** `Current Step` resolves to one step index in the range 0..15

#### Scenario: Direction and speed defaults are selected
- **WHEN** desktop v2 starts without a saved sequencer direction or speed override
- **THEN** direction `>` is selected
- **THEN** speed `1` is selected

#### Scenario: Direction and speed controls do not change step count
- **WHEN** the user selects direction `<`, `>`, or `RND`
- **THEN** the sequencer changes step traversal direction without changing the 16-step set
- **WHEN** the user selects speed `/2`, `/1.5`, `1`, `x1.5`, or `x2`
- **THEN** the sequencer changes clock multiplier without changing the 16-step set

### Requirement: Cleared sequencer steps are skipped without stopping audio
Each of the 16 sequencer steps SHALL have a written/unwritten state. A written step contains a saved step state. A cleared step is unwritten and SHALL be skipped by sequencer playback. A device-neutral long press on a written step SHALL directly clear that step after the long-press threshold. Mouse press-and-hold, touch press-and-hold, and holding a mapped MIDI/controller step control SHALL emit the same step-local clear command, with no second click, menu selection, or controller confirm. Releasing before the threshold SHALL cancel the clear. The step-local long press SHALL NOT create a held modifier for future parameter targets, randomization actions, Crunchy, Crispy, or MIDI/controller routing.

#### Scenario: Clear Step marks a step unwritten
- **WHEN** the user long-presses a written sequencer step by mouse, touch, or mapped MIDI/controller step control
- **AND** the hold reaches the long-press threshold
- **THEN** that step's saved snapshot/lock state is cleared
- **THEN** that step is marked unwritten
- **THEN** the sequencer still contains exactly 16 step slots

#### Scenario: Playback skips cleared steps
- **WHEN** sequencer playback advances
- **THEN** it applies only written steps according to the selected direction and speed
- **THEN** it skips unwritten steps without applying silence, reset, or default parameter values
- **THEN** odd effective sequence lengths are produced by the number and placement of written steps

#### Scenario: Clearing the final written step produces no-op sequencer playback
- **WHEN** only step 0 is written
- **AND** the user long-presses step 0 until the clear threshold is reached
- **THEN** all 16 steps are unwritten
- **THEN** sequencer transport and clock state remain able to continue running
- **THEN** the sequencer emits no step snapshot, lock recall, gate, silence, reset, or default-value event
- **THEN** audio continues from the current live synth state, matching playback when no sequencer steps have ever been recorded

### Requirement: Global randomization actions expose scene and step scope
Randomize All and Randomize Mod SHALL use two visible scope pairs: `All Scenes` / `Current Scene` and `All Steps` / `Current Step`. `Current Scene` SHALL mean the active scene edit-target endpoint whose stored parameter/modulation contents scene writes affect, not a blended or crossfaded scene result. Scene-scoped randomization SHALL NOT randomize which scene ordinals are selected for the left or right side of the scene slider, and SHALL NOT randomize the scene-slider position. `All Steps` SHALL mean all 16 sequencer steps. `Current Step` SHALL mean the sequencer playhead step while playback is running and the edit step while playback is stopped.

#### Scenario: Current scene and current step are always defined
- **WHEN** the global randomization controls render
- **THEN** exactly one scene scope is selected: `All Scenes` or `Current Scene`
- **THEN** exactly one step scope is selected: `All Steps` or `Current Step`
- **THEN** `Current Scene` resolves to the active scene edit-target endpoint
- **THEN** `Current Step` resolves to the playhead step while playing and the edit step while stopped

#### Scenario: Randomize All respects scene and step scope
- **WHEN** the user triggers Randomize All
- **THEN** eligible parameter values are randomized only for the selected scene scope
- **THEN** eligible sequencer snapshot values are randomized only for written steps in the selected step scope
- **THEN** unselected scene endpoints and unselected sequencer steps are preserved
- **THEN** the left scene selection, right scene selection, and scene-slider position are preserved

#### Scenario: Randomize Mod respects scene and step scope
- **WHEN** the user triggers Randomize Mod
- **THEN** eligible modulation source/depth values are randomized only for the selected scene scope
- **THEN** eligible modulation sequencer snapshot values are randomized only for written steps in the selected step scope
- **THEN** unavailable external-audio source lanes remain unavailable and are not chosen
- **THEN** the left scene selection, right scene selection, and scene-slider position are preserved
