# juce-vst-cc-mod-gating Specification

## Purpose
**Historical — superseded by `froggers-host-master`.** Pre-omni JUCE VST/AU fixed CC-pair ingest for mod indices 0 and 1. Current VST contract: 107 DAW host parameters; `acceptsMidi()` false.
## Requirements
### Requirement: Plugin-hosted CC enable controls

The VST/AU plugin SHALL expose MIDI CC 1 and MIDI CC 2 enable toggles (MIDI CC 1 default On, MIDI CC 2 default Off) when plugin-hosted, wired to `DesktopHostIO::SetMidiCcPairEnabled`.

#### Scenario: CC 2 default off

- **WHEN** the plugin loads with factory defaults and no saved state
- **THEN** MIDI CC 2 is disabled and the mod rack greys the CC 2 column

#### Scenario: MIDI Settings opens in plugin mode

- **WHEN** the user clicks MIDI Settings in the plugin UI
- **THEN** a dialog shows CC1/CC2 channel, CC number, and enable toggles

#### Scenario: Hardware device pickers hidden

- **WHEN** the MIDI Settings dialog opens in plugin-hosted mode
- **THEN** MIDI In device picker, computer keyboard option, and MIDI Out section are not shown

### Requirement: DAW CC respects enable flags

DAW-routed MIDI CC ingest SHALL pass through `PushMidiCc` and honor enable flags identically to desktop standalone.

#### Scenario: Disabled pair ignored from DAW

- **WHEN** MIDI CC 2 is disabled in the plugin and the DAW sends a matching CC 2 message
- **THEN** `mods[1]` is 0.0 after block processing

#### Scenario: Mod rack greys disabled column

- **WHEN** MIDI CC 1 is disabled in the plugin
- **THEN** the desktop mod rack MIDI CC 1 column renders greyed and rejects new patch assignments

### Requirement: Toolbar MIDI button functional

The plugin main toolbar MIDI Settings control SHALL open the CC enable dialog when plugin-hosted.

#### Scenario: Button not a no-op

- **WHEN** the user clicks MIDI Settings in plugin mode
- **THEN** the CC enable dialog appears (the click does not silently return)

### Requirement: vst-v2-midi-gating-policy
`juce-vst-cc-mod-gating` rules apply to FroggersTigaPlugin (v1) only. FroggersTigaPluginV2 SHALL enable MIDI input and SHALL NOT apply v1 `acceptsMidi() false` gating.

#### Scenario: v1 plugin still rejects raw MIDI buffer for mod
- **WHEN** MIDI is sent to FroggersTigaPlugin v1
- **THEN** existing v1 gating scenarios remain unchanged

#### Scenario: v2 plugin accepts MIDI flag
- **WHEN** a host queries FroggersTigaPluginV2 bus capabilities
- **THEN** MIDI input is advertised as supported for DAW routing to parameters

