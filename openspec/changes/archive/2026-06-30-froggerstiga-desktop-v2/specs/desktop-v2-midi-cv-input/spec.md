## ADDED Requirements

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
