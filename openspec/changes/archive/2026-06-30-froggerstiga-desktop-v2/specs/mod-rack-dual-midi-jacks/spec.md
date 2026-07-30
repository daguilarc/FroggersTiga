## ADDED Requirements

### Requirement: v2-excludes-dual-midi-jack-mod-rack
Desktop v2 SHALL NOT render dual MIDI CC scope jacks from `mod-rack-dual-midi-jacks`. External MIDI is configured only through `desktop-v2-midi-cv-input`.

#### Scenario: v1 desktop dual jacks unchanged
- **WHEN** v1 desktop renders mod rack indices 0 and 1
- **THEN** dual MIDI jack presentation remains per existing spec

#### Scenario: v2 has no CC1/CC2 scope cells
- **WHEN** desktop v2 renders mod source UI
- **THEN** no CC1 or CC2 scope cells appear in the mod grid
