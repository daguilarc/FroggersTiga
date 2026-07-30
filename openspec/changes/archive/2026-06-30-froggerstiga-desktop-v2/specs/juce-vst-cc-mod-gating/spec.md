## ADDED Requirements

### Requirement: vst-v2-midi-gating-policy
`juce-vst-cc-mod-gating` rules apply to FroggersTigaPlugin (v1) only. FroggersTigaPluginV2 SHALL enable MIDI input and SHALL NOT apply v1 `acceptsMidi() false` gating.

#### Scenario: v1 plugin still rejects raw MIDI buffer for mod
- **WHEN** MIDI is sent to FroggersTigaPlugin v1
- **THEN** existing v1 gating scenarios remain unchanged

#### Scenario: v2 plugin accepts MIDI flag
- **WHEN** a host queries FroggersTigaPluginV2 bus capabilities
- **THEN** MIDI input is advertised as supported for DAW routing to parameters
