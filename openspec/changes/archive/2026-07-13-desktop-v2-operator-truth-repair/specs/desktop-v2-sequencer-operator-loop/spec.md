## ADDED Requirements

### Requirement: Write Seq. captures audible state to steps
Desktop v2 SHALL let operators write sequencer step snapshots from live synth state using Write Seq. while transport runs, and by explicit step selection when armed.

#### Scenario: Write while playing advances capture
- **WHEN** Write Seq. is armed and Start Sequence is active
- **THEN** each sequencer clock advance captures the prior step's live state into that step snapshot
- **THEN** captured steps become written and visible in the 16-step grid

#### Scenario: Click step while armed writes snapshot
- **WHEN** Write Seq. is armed and transport is stopped
- **THEN** clicking a step button writes the current live state into that step snapshot
- **THEN** the step becomes written

### Requirement: All Steps randomization writes all written steps
When All Steps scope is active, randomization commands that target sequencer storage SHALL affect every written step among the fixed 16 slots.

#### Scenario: Rand-seq dice with All Steps
- **WHEN** All Steps is selected and the operator triggers Rand-seq
- **THEN** scene-slot values randomize into every written step snapshot

#### Scenario: Rand Mods with All Steps
- **WHEN** All Steps is selected and the operator clicks Rand Mods
- **THEN** mod snapshots in every written step update to match the randomization policy

### Requirement: Sequencer scope UI is not duplicated
Step scope selection SHALL have a single authority. Sequencer toolbar MUST NOT host a second All Steps / Current Step radio group when the global-command band already provides step scope.

#### Scenario: Single step scope authority
- **WHEN** the operator toggles All Steps in the global-command band
- **THEN** the sequencer Rand-seq dice uses All Steps scope
- **THEN** no duplicate step scope control appears in the sequencer toolbar
