## ADDED Requirements

### Requirement: v2-full-sequencer
Desktop v2 and VST v2 SHALL include a **full step sequencer** integrated with transport, scenes, and the control core.

#### Scenario: Sequencer UI visible
- **WHEN** desktop v2 or VST v2 editor is open
- **THEN** a sequencer panel shows step grid, pattern length, and playhead position
- **THEN** the panel is reachable from the global strip (Sequencer toggle or dedicated tab)

#### Scenario: Clock from transport
- **WHEN** transport Play is active and BPM is set
- **THEN** the sequencer advances steps on beat boundaries at the configured BPM
- **THEN** step changes publish `MessageIn::Clock` (or equivalent) to the control core

#### Scenario: Per-step scene capture
- **WHEN** the user records into step N while sequencer record mode is armed
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
- **THEN** sequencer step advance may follow incoming MIDI clock instead of internal BPM when sync mode is External

### Requirement: v2-sequencer-gate-for-adsr
The sequencer SHALL provide a per-step or global gate signal usable by `VcoAdsrState` gated ADSR envelopes.

#### Scenario: Step gate for ADSR
- **WHEN** sequencer playback is active and a step defines gate on
- **THEN** ADSR envelopes receive gate high for the step duration
- **WHEN** gate is off for a step
- **THEN** ADSR envelopes receive gate low and release per release times
