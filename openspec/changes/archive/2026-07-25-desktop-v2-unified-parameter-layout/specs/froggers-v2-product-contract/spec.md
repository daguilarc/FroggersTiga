## MODIFIED Requirements

### Requirement: Froggers v2 synth product contract
Froggers v2 SHALL preserve the Froggers synth shape: three cross-coupled VCOs, external-input ring modulation or continuous oscillator operation, module sections for Audio/VCO, Envelope, Filter, Distortion, Random S&H, Reverb, and Delay, per-parameter modulation/randomization/scene behavior, and sequencer-owned parameter locks on a fixed 16-step sequencer. The Audio section SHALL expose cross-coupler controls for **all three** VCO pairings — VCO 1/2, VCO 2/3, and VCO 1/3.

#### Scenario: Audio section exposes three cross-couplers
- **WHEN** the operator views the Audio/VCO section
- **THEN** the section exposes cross-coupler controls for VCO 1/2, VCO 2/3, and VCO 1/3
- **THEN** each cross-coupler is a manifest-owned parameter row (single authority, same pattern for all three pairings)

### Requirement: Envelope and waveform morph controls participate in v2 modulation behavior
Froggers v2 SHALL provide an **Envelope** section positioned immediately after Audio/VCO, exposing per-VCO **Attack**, **Sustain**, and **Release** controls (ASR) for VCO 1, VCO 2, and VCO 3, and SHALL expose a continuous waveform morph control for each VCO. Operator-visible labels use full words (Attack, Sustain, Release); the section is titled **Envelope** (no "Pair-AR"). Sustain is a level held while the gate is high; Release is the fall time after gate-off; there is no decay-to-sustain knee (ASR, not ADSR).

#### Scenario: Envelope section exposes ASR per VCO
- **WHEN** the operator views the Envelope section
- **THEN** it exposes VCO 1, VCO 2, and VCO 3 Attack, Sustain, and Release controls with full-word labels
- **THEN** the section is titled **Envelope** and appears immediately right of Audio/VCO
- **THEN** holding the gate sustains at the Sustain level and releasing falls over the Release time
