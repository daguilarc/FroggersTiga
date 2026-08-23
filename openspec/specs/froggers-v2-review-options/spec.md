# froggers-v2-review-options Specification

## Purpose
Product-owner decisions for the Froggers v2 / Sheaf convergence are recorded before dependent implementation locks behavior.

## Requirements
### Requirement: Successor review options are explicit
The Froggers v2 / Sheaf convergence proposal SHALL record product-owner decisions before code locks behavior, and SHALL preserve bounded option sets only for implementation choices that genuinely remain open.

#### Scenario: Review options are available before implementation
- **WHEN** a successor begins implementation of this change
- **THEN** the design artifact records selected decisions for C++-authoritative manifest storage, Sheaf-style parameter/modulation management behind the Froggers facade after manifest validation, controller target readback using product display format, collapsible hosted runtime status UI, VST/AU host-state-only File/Patch behavior with no plugin preset browser/import-export workflow, no MIDI learn mode/recent-event list, multi-target controller mapping behavior, three-VCO global oscilloscope source mode, and one top chrome stack with transport/signal and global-command bands
- **THEN** any remaining option set is limited to implementation sequencing or verification evidence

#### Scenario: Product contract is not optional
- **WHEN** the successor reviews the option sets
- **THEN** the options do not weaken the Froggers product contract for three VCOs, Audio/VCO default page, two cross-couplers, Envelope page, waveform morph controls, global oscilloscope, modulation assignment, MIDI mapping, global randomization scope, fixed 16-step sequencer, clocked sequencer locks, or desktop/VST behavior

### Requirement: Option decisions are recorded before dependent implementation
Implementation SHALL record the selected option before completing tasks that depend on that option.

#### Scenario: Manifest storage selected before manifest implementation
- **WHEN** task 1.2 begins manifest-family schema implementation
- **THEN** the implementation uses the recorded C++-authoritative manifest decision unless the product owner explicitly reopens it

#### Scenario: Hosted UI selected before plugin editor implementation
- **WHEN** hosted editor runtime projection work begins
- **THEN** the implementation uses the recorded collapsible hosted runtime status decision
- **THEN** the implementation uses the recorded VST/AU File/Patch decision: host state and DAW preset mechanisms only, with no plugin preset browser, direct plugin file-system preset save/load, or plugin import/export workflow in this change

#### Scenario: MIDI behavior selected before controller UI implementation
- **WHEN** MIDI/Controllers page implementation begins
- **THEN** the implementation uses the recorded no-learn-mode, no-recent-event-list, and multi-target physical input mapping decisions
- **THEN** target readback uses product display formatting rather than raw normalized values in the normal Controllers page

#### Scenario: Oscilloscope behavior selected before shell layout implementation
- **WHEN** global oscilloscope shell layout begins
- **THEN** the implementation uses the recorded three-VCO default source-mode decision and Sheaf-style modulation-aware visualization
