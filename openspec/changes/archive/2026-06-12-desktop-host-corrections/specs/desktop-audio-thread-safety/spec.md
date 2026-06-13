## ADDED Requirements

### Requirement: VCO morph changes are applied on the audio thread

All VCO waveform morph mutations (`NudgeVcoMorph`, `SetVcoMorph`, `RandomizeVcoMorphs`) initiated from desktop UI SHALL be queued from the message thread and applied inside `DesktopHostIO::tickControls()` (or equivalent) before `ProcessBlock`. The audio thread SHALL NOT read `m_vcoMorph[]` while the message thread writes it.

#### Scenario: Wave button during Play

- **WHEN** the user clicks a VCO wave cycle button while audio is playing
- **THEN** morph updates apply on the audio thread
- **AND** audio continues without permanent silence

#### Scenario: Randomize waves during Play

- **WHEN** the user clicks **Randomize waves** on the global strip while audio is playing
- **THEN** all three morph values update safely
- **AND** audio continues without permanent silence

### Requirement: Morph values are finite before DSP use

`ModulatedMorph` or `EvalWaveMorph` SHALL treat non-finite morph values as `0.f` before waveform evaluation.

#### Scenario: Corrupted morph guard

- **WHEN** a morph value is NaN or infinite
- **THEN** the engine uses morph `0.f` for that sample
- **AND** oscillator output remains finite

### Requirement: Transport recovers after device stop

`AudioEngine::audioDeviceStopped()` SHALL set internal running state to false and notify the UI to refresh Play/Stop buttons.

#### Scenario: CoreAudio stops stream

- **WHEN** the OS stops the audio device while the app shows playing
- **THEN** Play becomes enabled and Stop becomes disabled
- **AND** clicking Play restarts audio

### Requirement: Transport recovers after device error

`AudioEngine` SHALL override `audioDeviceError`, log the message, clear running state, and notify the UI.

#### Scenario: Device error

- **WHEN** JUCE reports an audio device error
- **THEN** the user can click Play again without restarting the app

### Requirement: Optional DSP soft-reset on Stop after non-finite output

When the user clicks Stop, if the engine detected non-finite samples in the last processed block, the host SHALL reset volatile DSP state (reverb/delay lines minimum) before the next Play.

#### Scenario: Recovery after NaN poison

- **WHEN** audio was poisoned by a prior bug and the user clicks Stop then Play
- **THEN** audible output returns without quitting the application
