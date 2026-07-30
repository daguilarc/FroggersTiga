## MODIFIED Requirements

### Requirement: VCV CC enable toggles on primary panel

The VCV primary panel SHALL provide independent enable toggles for MIDI CC 1 and MIDI CC 2 (MIDI CC 1 default On, MIDI CC 2 default Off), wired to `PagedHostIO::SetMidiCcPairEnabled`.

#### Scenario: CC 2 default off

- **WHEN** a new VCV module is placed with factory defaults
- **THEN** the MIDI CC 2 enable control is Off and mod index 1 is gated off

#### Scenario: CC 2 disabled dims affordance

- **WHEN** MIDI CC 2 is disabled
- **THEN** the CC 2 enable control renders at reduced brightness
