# desktop-v2-audio-io Specification

## Purpose
Desktop v2 and VST v2 default to stereo audio output with mono input and shared mono-core rendering/downmix behavior, and that state feeds the runtime Audio page projection without altering the underlying stereo/mono contract.
## Requirements
### Requirement: v2-stereo-default-desktop
Desktop v2 SHALL initialize audio output as **stereo by default** using two active output channels.

#### Scenario: Default device setup
- **WHEN** desktop v2 starts audio without user configuration
- **THEN** `AudioDeviceManager` is initialized with two output channels active (equivalent to v1 `initialiseWithDefaultDevices(0, 2)` with output bits 0 and 1 set)

#### Scenario: Mono output device downmix
- **WHEN** the user selects a mono output device or only one output channel is active
- **THEN** `applyStereoBus` writes a downmixed mono signal to the single output channel
- **THEN** audio remains glitch-free and finite

### Requirement: v2-vst-bus-layout
FroggersTigaPluginV2 SHALL declare mono audio input and stereo audio output as the default bus layout, matching v1 plugin conventions.

#### Scenario: Default VST buses
- **WHEN** a host instantiates FroggersTigaPluginV2 with default layout
- **THEN** input bus is mono (external audio optional)
- **THEN** output bus is stereo

#### Scenario: Mono output bus supported
- **WHEN** a host requests mono output only
- **THEN** the plugin accepts the layout if supported by `isBusesLayoutSupported`
- **THEN** processing downmixes via the same `applyStereoBus` mono path as desktop

#### Scenario: Internal mono core unchanged
- **WHEN** either stereo or mono output is used
- **THEN** `FroggersEngine::ProcessBlock` still renders a mono core buffer before stereo spread

### Requirement: Audio IO state feeds runtime audio projection
Desktop v2 audio IO state SHALL feed the runtime Audio page projection without changing existing stereo default, mono downmix, optional mono input, or mono-core rendering behavior.

#### Scenario: Runtime page reflects stereo default
- **WHEN** desktop v2 initializes default audio output
- **THEN** the runtime Audio page reports two active output channels
- **THEN** stereo processing behavior matches the existing audio IO contract

#### Scenario: Mono downmix remains shared
- **WHEN** standalone or hosted output has one channel
- **THEN** the runtime Audio page reports mono output
- **THEN** audio downmix uses the existing shared mono path

