# froggers-v2-runtime-audio-configuration Specification

## Purpose
Desktop standalone v2 runtime Audio page and hosted read-only audio status projection without altering DSP bus semantics or duplicating the global oscilloscope.

## Requirements
### Requirement: Labeled runtime audio page
Desktop standalone v2 SHALL expose a runtime Audio page where every setup field has a visible label. The page SHALL show selected input/output devices, active channel masks, negotiated sample rate, block size, current bus layout, external-input state, input meter state, and output meter state.

#### Scenario: Negotiated device state visible
- **WHEN** desktop standalone starts audio
- **THEN** the Audio page shows the active output device, active output channels, negotiated sample rate, and negotiated block size

#### Scenario: Input state visible
- **WHEN** external audio input is configured or unavailable
- **THEN** the Audio page shows input device state, active input channels, and one of these external-input states: `active`, `unavailable`, `muted`, or `clipped`
- **THEN** the state is derived from the same audio input path used by Froggers v1/v2 external audio processing

#### Scenario: Output state visible
- **WHEN** desktop standalone audio is running
- **THEN** the Audio page shows one of these output states: `active`, `muted`, `clipped`, or `unavailable`

#### Scenario: Signal shape remains on scopes
- **WHEN** the user wants to inspect audio or modulation signal shape
- **THEN** the global top-chrome oscilloscope provides the signal view
- **THEN** the runtime Audio page does not add a duplicate oscilloscope

### Requirement: Hosted audio projection is read-only
VST/AU v2 SHALL expose hosted bus/status information without presenting standalone hardware device selection.

#### Scenario: Hosted editor omits hardware selectors
- **WHEN** FroggersTigaPluginV2 opens in a DAW
- **THEN** hardware input and output device selectors are absent
- **THEN** the hosted audio status panel, when enabled by the plugin overlay, shows host input bus count, host output bus count, active channel layout, sample rate, block size if reported by the host, input present/unavailable, and output active/clipped/muted status
- **THEN** all displayed hosted audio fields are read-only

### Requirement: Audio page does not alter DSP bus semantics
Runtime audio configuration visibility SHALL NOT change Froggers v2 stereo output, mono downmix, optional mono input, or internal mono-core rendering contracts.

#### Scenario: Stereo default preserved
- **WHEN** desktop standalone starts with default devices
- **THEN** the runtime Audio page reports two active output channels
- **THEN** the audio engine still follows the `desktop-v2-audio-io` stereo default contract
