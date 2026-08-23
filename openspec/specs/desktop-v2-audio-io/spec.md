# desktop-v2-audio-io Specification

## Purpose
VST v2 defaults to stereo audio output with mono input and shared mono-core rendering/downmix behavior.
## Requirements
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

