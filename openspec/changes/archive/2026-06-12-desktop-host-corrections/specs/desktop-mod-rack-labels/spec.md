## ADDED Requirements

### Requirement: VCO mod source is labeled VCO level

The desktop mod rack box for mod index `4` SHALL display **VCO level** (not "VCO feat") with subtitle **Mod out**.

#### Scenario: Mod rack readability

- **WHEN** the user views the mod rack
- **THEN** the second box title is **VCO level**
- **AND** a subtitle or tooltip explains it is the slow envelope from combined VCO amplitudes (hardware M5)

### Requirement: Marbles are documented as manual random CV

Marbles 1 and Marbles 2 boxes SHALL indicate they are **stepped random** sources clocked by the **Marbles** global strip button (hardware B5), not free-running LFOs.

#### Scenario: Marbles meter idle

- **WHEN** the user has not pressed **Marbles** and nothing is patched
- **THEN** Marbles meters may be static
- **AND** UI text explains stepping is manual

### Requirement: No patchable LFO mod bus in v1

The desktop sim SHALL NOT claim or imply patchable sine LFO mod outputs. Reverb LFO (`RMOD`/`RRAT`) and delay LFO (`DMOD`) remain internal to their effects.

#### Scenario: User searches for LFO patch source

- **WHEN** the user inspects the mod rack
- **THEN** only MIDI, VCO level, Marbles 1, and Marbles 2 are available patch sources
- **AND** no LFO-labeled jack is shown
