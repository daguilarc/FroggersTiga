## ADDED Requirements

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
