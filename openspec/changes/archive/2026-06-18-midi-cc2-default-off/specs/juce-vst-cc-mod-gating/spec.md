## MODIFIED Requirements

### Requirement: VST CC enable toggles

The VST/AU plugin SHALL expose MIDI CC 1 and MIDI CC 2 enable toggles when plugin-hosted (MIDI CC 1 default On, MIDI CC 2 default Off), wired to `DesktopHostIO::SetMidiCcPairEnabled`.

#### Scenario: CC 2 default off

- **WHEN** the plugin loads with factory defaults and no saved state
- **THEN** MIDI CC 2 is disabled and the mod rack greys the CC 2 column

#### Scenario: CC 2 disabled ignores DAW CC

- **WHEN** MIDI CC 2 is disabled in the plugin and the DAW sends a matching CC 2 message
- **THEN** `mods[1]` remains 0.0
