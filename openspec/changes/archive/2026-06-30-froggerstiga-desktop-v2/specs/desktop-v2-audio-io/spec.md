## ADDED Requirements

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
