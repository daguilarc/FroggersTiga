## ADDED Requirements

### Requirement: VST preset serialization

`FroggersTigaPlugin` SHALL implement `getStateInformation` and `setStateInformation` to save and restore sim knob values, mod routing, and delay sidecar state sufficient to reproduce timbre after DAW reload.

#### Scenario: DAW session reload

- **WHEN** user saves a DAW project with FroggersTiga loaded and reopens it
- **THEN** knob positions and mod assignments match the saved session

### Requirement: DAW bypass honored

When the host bypasses the plugin, `processBlock` SHALL output silence (or host-defined bypass behavior) without advancing sim state that affects post-bypass output.

#### Scenario: Bypass silences output

- **WHEN** host sets plugin bypass on
- **THEN** audio output is silent until bypass is released

### Requirement: Sample rate follows host

Plugin `prepareToPlay` SHALL call `AudioEngine::setHostSampleRate` with the host sample rate; sim DSP SHALL not assume 44100 Hz when hosted.

#### Scenario: 48 kHz host session

- **WHEN** DAW opens plugin at 48000 Hz
- **THEN** engine and delay sidecar run at 48000 Hz
