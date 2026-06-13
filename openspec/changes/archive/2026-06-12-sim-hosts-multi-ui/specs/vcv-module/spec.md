## ADDED Requirements

### Requirement: Paged Rack module

The VCV Rack 2 plugin SHALL present one page at a time with eight knobs mapping to the current page parameters and controls to switch among all five pages.

#### Scenario: Page switch changes knob labels

- **WHEN** the user switches from Audio to Drive page
- **THEN** knob labels update to Drive parameter names

### Requirement: Shared core linkage

The plugin SHALL link the same `FroggersEngine` and `PagedHostIO` (or equivalent paged adapter) as the web simulator.

#### Scenario: FUEG on fuegoized pages

- **WHEN** the Drive page is active and FUEG is swept
- **THEN** drive character changes consistent with web sim at same parameter values

### Requirement: License gate

VCV module publication SHALL NOT proceed until GPL distribution strategy for MIT core is documented and accepted.

#### Scenario: Blocked without license decision

- **WHEN** no license decision is recorded in project docs
- **THEN** VCV plugin is not published to the Rack library

### Requirement: Rack sample rate

The plugin SHALL call `SetSampleRate` with the Rack engine sample rate each process cycle or when the rate changes.

#### Scenario: Rate matches Rack

- **WHEN** Rack runs at 48 kHz
- **THEN** engine internal rate is 48000 Hz

### Requirement: Sim-only VCO morph on Audio page

When the Audio page is active, the VCV plugin SHALL show morph knobs (or compact params) for VCO1–3 with linear 0–1 morph read and CV inputs routable as mod attenuators. Wave shape SHALL be indicated by icons beside VCO labels, not separate panel rows. B′ randomize-all-morphs optional. Daisy firmware unchanged.

#### Scenario: CV attenuates VCO morph in Rack

- **WHEN** CV1 is patched to VCO2 morph mod and depth is non-zero
- **THEN** morph follows ModMgr attenuation semantics

#### Scenario: Firmware unaffected

- **WHEN** the Rack plugin is unloaded and Daisy device runs
- **THEN** device VCO behavior matches pre-sim hardware

### Requirement: Field-parity Rack I/O

The VCV plugin SHALL expose Rack jacks matching Daisy Field roles. MIDI complements these jacks — it does not replace them.

| Jack | Role |
|------|------|
| Audio in | External ring-mod / envelope source → `ProcessSample` input |
| Audio out | Stereo or mono main output |
| CV1–CV4 in | 0–1V maps to `m_modMgr.m_mods[0..3]` via `PagedHostIO::SetCv` |
| Gate in | Gate → `ButtonCallback` |
| CV out 1–2 | `m_modMgr.m_mods[4]` / `m_mods[5]` (M5/M6) |
| MIDI in / out | `CvMidiBridge` CC ↔ CV1–4 + envelope CC out |

#### Scenario: CV1 modulates like Field

- **WHEN** 5 V is patched to CV1 input (Rack 0–10 V scaled to 0–1)
- **THEN** `m_mods[0]` and `m_externalCvActive[0]` follow Field CV presence semantics

#### Scenario: Gate triggers button callback

- **WHEN** a gate rising edge arrives at the gate input
- **THEN** the same `ButtonCallback` path as hardware gate fires

#### Scenario: CV outs carry M5/M6

- **WHEN** M5/M6 mod sources are active on the current page
- **THEN** CV out 1–2 voltages reflect `m_mods[4]` and `m_mods[5]`

### Requirement: MIDI in and out on module

The VCV plugin SHALL provide MIDI input and output ports in addition to Field-parity CV/gate/audio jacks. MIDI out SHALL transmit envelope tracker level as CC via `CvMidiBridge::tickMidiOut()` after each `process()` block. MIDI in SHALL be drained in `tickControls()` before `ProcessBlock` and map CC to CV1–CV4 mod inputs.

#### Scenario: MIDI out in Rack

- **WHEN** audio input drives the envelope follower
- **THEN** a MIDI-CV module downstream receives CC values proportional to envelope

#### Scenario: MIDI in modulates via CV path

- **WHEN** MIDI CC is patched to the module MIDI input
- **THEN** assigned CC maps to `m_mods[0..3]` like Field CV inputs before audio processing in that block
