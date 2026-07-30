## MODIFIED Requirements

### Requirement: Sequencer toolbar omits duplicate global randomization
The sequencer panel SHALL NOT duplicate global Rand Mods or global step-scope radios. Sequencer-local controls are limited to transport, BPM, direction/speed icons, Write Seq., step navigation, and Rand-seq scene-slot dice.

#### Scenario: No second All Steps toggle
- **WHEN** the sequencer toolbar renders at 1280×920
- **THEN** All Steps / Current Step radios appear only in the global-command band
- **THEN** the sequencer toolbar does not render a second All steps control
