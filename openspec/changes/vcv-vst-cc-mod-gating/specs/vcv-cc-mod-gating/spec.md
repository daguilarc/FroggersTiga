## ADDED Requirements

### Requirement: VCV CC ingest through CvMidiBridge

The VCV module SHALL route inbound MIDI CC through `CvMidiBridge.PushMidiCc` and `drainMidiIn`, not direct `mods[]` assignment.

#### Scenario: Enabled pair latches CV

- **WHEN** MIDI CC 1 is enabled and a matching CC message arrives
- **THEN** `mods[0]` reflects the normalized CC value after block drain

#### Scenario: Disabled pair ignored

- **WHEN** MIDI CC 2 is disabled and a matching CC message arrives
- **THEN** `mods[1]` is 0.0

### Requirement: VCV CC enable toggles

The VCV field-parity panel SHALL provide independent enable toggles for MIDI CC 1 and MIDI CC 2 (default On).

#### Scenario: Disable clears routes

- **WHEN** the user disables MIDI CC 1 on the VCV panel while a knob uses mod index 0
- **THEN** that route becomes None with zero depth

### Requirement: VCV grey disabled mod columns

When a MIDI CC pair is disabled, the corresponding mod rack column on the VCV panel SHALL render greyed out and SHALL not accept new patch assignments.

#### Scenario: Grey CC2 column

- **WHEN** MIDI CC 2 is disabled
- **THEN** the MIDI CC 2 mod column is visually greyed and patch assignment to mod index 1 is rejected
