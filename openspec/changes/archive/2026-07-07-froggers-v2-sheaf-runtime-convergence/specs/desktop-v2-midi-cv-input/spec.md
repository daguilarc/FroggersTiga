## ADDED Requirements

### Requirement: MIDI CV input uses controller configuration model
Desktop v2 MIDI pitch, gate, CC, and modulation assignments SHALL be stored and applied through the controller configuration model using manifest target IDs.

#### Scenario: Pitch assignment persists by target ID
- **WHEN** the user maps MIDI note messages to VCO1 pitch
- **THEN** the saved mapping references the manifest target ID for VCO1 pitch
- **THEN** reloading the session restores the mapping even if the display label changes

#### Scenario: QWERTY virtual MIDI remains explicit
- **WHEN** QWERTY keyboard input is enabled as virtual MIDI
- **THEN** it appears as an explicit virtual controller source in the controller configuration model
- **THEN** disabling it stops QWERTY from mutating MIDI CV targets
