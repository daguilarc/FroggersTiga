# froggers-vco-topology Specification

## Purpose
The Froggers three-oscillator topology (exponential pitch, continuous Shape morph, self-contained phase modulation) with no hardcoded cross-VCO coupling, exposing Sheaf's VCO scope UI-state and envelope followers, and gated by the master clock's quarter-note pulse so pitch and amplitude gating stay independent.

## Requirements
### Requirement: Froggers oscillator topology is preserved
The app SHALL implement the Froggers three-oscillator topology: per-VCO pitch on an exponential map spanning roughly 20 Hz to 20 kHz; a continuous waveform **Shape** morph crossfading sine to saw over the lower half of its range and saw to square over the upper half; and per-VCO phase modulation driven by that VCO's **own** dedicated sine LFO whose frequency is an exponential function of the PM knob.

#### Scenario: Shape morph sweeps continuously
- **WHEN** a VCO's Shape control is swept from minimum to maximum
- **THEN** the waveform morphs continuously from sine through saw to square without discontinuity

#### Scenario: Phase modulation is self-contained
- **WHEN** any VCO's phase-modulation control is raised
- **THEN** only that VCO's phase is modulated
- **THEN** no other VCO's output changes as a result

### Requirement: No hardcoded cross-VCO coupling
The oscillator section SHALL contain no hardcoded VCO-to-VCO coupling terms. All inter-oscillator routing SHALL be expressed only through the modulation matrix.

#### Scenario: Cross-VCO routing requires an explicit assignment
- **WHEN** no modulation assignment links two VCOs
- **THEN** changing one VCO's parameters does not alter another VCO's output

#### Scenario: Default patch ships ordinary modulation assignments, not topology
- **WHEN** the app starts for the first time
- **THEN** the initial patch includes cross-oscillator modulation assignments at minimal depth
- **THEN** those assignments are ordinary modulation-matrix entries the operator can remove like any other
- **THEN** removing them restores full oscillator independence, with changing one oscillator no longer altering another

### Requirement: Phase modulation has a true zero position
The phase-modulation control SHALL be fully inert at its minimum position, with a smooth ramp from that floor into its active range.

#### Scenario: Minimum position is silent modulation
- **WHEN** a VCO's phase-modulation control is at minimum
- **THEN** that VCO's phase receives zero modulation depth

### Requirement: Oscillators expose Sheaf scope state
Each VCO SHALL expose the oscillator UI-state shape Sheaf's scope visualizer consumes — connection flag, scope writer reference, scope channel, and scope color — and SHALL accept a scope writer holder and color, publishing its UI state each block. The app SHALL NOT implement its own waveform drawing for VCO scopes.

#### Scenario: Scope visualizer binds without app drawing code
- **WHEN** the surface is built with VCO scope panels
- **THEN** each panel is driven by the standard Sheaf scope visualizer over the VCO's UI state
- **THEN** no bespoke waveform rasterization exists in the app

#### Scenario: Scope reports connected after processing
- **WHEN** the app has processed at least one block
- **THEN** each VCO's scope state reports connected
- **THEN** the panel renders a live waveform trace

### Requirement: Envelope followers on each oscillator
The app SHALL provide an envelope follower per VCO with fast attack and release characteristics matching the Froggers follower, feeding the three VCO envelope-follower modulation sources.

#### Scenario: Follower tracks oscillator level
- **WHEN** a VCO's output level rises and then falls
- **THEN** its envelope follower value rises quickly and decays more slowly
- **THEN** that value is available as a modulation source

### Requirement: The transport pulse gates the instrument; pitch stays on the pitch controls
The running transport's quarter-note pulse SHALL be the sole trigger for sound: while the transport runs, the amplitude envelope opens and closes in time with the quarter-note pulse. Oscillator pitch SHALL come solely from the pitch controls — there is no other source of transposition. While the transport is stopped, the instrument SHALL be silent regardless of any other parameter setting.

#### Scenario: A running transport gates the amplitude envelope
- **WHEN** the transport is running
- **THEN** the amplitude envelope opens and closes in time with the transport's quarter-note pulse
- **THEN** oscillator pitch is determined solely by the pitch controls, unaffected by the pulse

#### Scenario: Transport stopped is silence
- **WHEN** the transport is not running
- **THEN** the instrument produces no audible output, regardless of any other parameter setting
