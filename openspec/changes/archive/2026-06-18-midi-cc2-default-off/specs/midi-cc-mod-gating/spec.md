## MODIFIED Requirements

### Requirement: Per-pair MIDI CC enable flags

The sim host SHALL maintain independent enable flags for MIDI CC 1 (mod index 0) and MIDI CC 2 (mod index 1). Defaults: MIDI CC 1 enabled and MIDI CC 2 disabled on desktop, VST, and VCV; on web both disabled until External MIDI is turned on (then CC 1 enabled, CC 2 disabled unless the operator enables it).

#### Scenario: Desktop defaults

- **WHEN** the desktop app starts with no persisted override
- **THEN** MIDI CC 1 enable flag is true and MIDI CC 2 enable flag is false

#### Scenario: Web defaults before External MIDI

- **WHEN** the web sim loads and External MIDI is Off
- **THEN** MIDI CC 1 and MIDI CC 2 enable flags are false

#### Scenario: Web External MIDI on

- **WHEN** the operator turns External MIDI on
- **THEN** MIDI CC 1 enable flag is true and MIDI CC 2 enable flag remains false
