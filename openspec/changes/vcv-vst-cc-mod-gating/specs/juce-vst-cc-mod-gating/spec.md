## ADDED Requirements

### Requirement: Plugin-hosted CC enable controls

The VST/AU plugin SHALL expose MIDI CC 1 and MIDI CC 2 enable toggles (default On) when plugin-hosted, wired to `DesktopHostIO::SetMidiCcPairEnabled`.

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
