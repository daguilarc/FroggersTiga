# desktop-v2-midi-cv-input Specification

## Purpose
Desktop v2 exposes one primary MIDI input device for user-assignable pitch, gate, and mod-CV routing, replacing v1's dual CC-pair MIDI Settings UI, with assignments stored and applied through the controller configuration model using manifest target IDs.
## Requirements
### Requirement: v2-single-midi-cv-input
Desktop v2 SHALL expose one primary MIDI input device selector for user-assignable CV routing (pitch, gate, and mod CV targets).

#### Scenario: Pitch assignment from MIDI input
- **WHEN** the user maps incoming MIDI note messages to Pitch CV on VCO1
- **THEN** note number and velocity (per design) update the configured pitch target through `DesktopHostIO`
- **THEN** v1 dual CC-pair MIDI Settings UI is not shown in v2

#### Scenario: Gate assignment from MIDI input
- **WHEN** the user maps incoming note-on/off to an external gate target
- **THEN** gate state follows MIDI note lifecycle on the audio thread

#### Scenario: CC to mod source assignment
- **WHEN** the user maps MIDI CC 74 to feed mod source slot configured as "external CC A"
- **THEN** that CC level is available as a modulator input to the control core at control rate
- **THEN** the assignment is persisted in v2 session state

#### Scenario: One input device at a time
- **WHEN** the user selects a MIDI input device in v2 settings
- **THEN** only that device's messages are consumed for CV assignment
- **THEN** QWERTY keyboard MAY still drive a configurable virtual MIDI channel when enabled

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

