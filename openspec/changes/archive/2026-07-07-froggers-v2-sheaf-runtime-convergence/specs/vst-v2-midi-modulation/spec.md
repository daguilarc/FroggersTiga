## ADDED Requirements

### Requirement: VST v2 MIDI maps through host parameters
FroggersTigaPluginV2 SHALL treat DAW MIDI mapping as host-parameter control over manifest-declared parameters. Raw MIDI SHALL NOT mutate a second private modulation or controller assignment table.

#### Scenario: DAW CC controls host parameter
- **WHEN** a DAW maps CC 1 to the `Global/Crunchy` host parameter
- **THEN** Crunchy follows the host parameter value delivered to the plugin
- **THEN** no raw CC slot in Froggers modulation state is updated

#### Scenario: Parameter inventory derives from manifest
- **WHEN** `HostParameterInventoryV2` is generated or validated
- **THEN** every plugin parameter corresponds to a manifest entry exposed to hosted automation
- **THEN** every manifest hosted parameter has one stable plugin parameter ID

### Requirement: Hosted runtime projection excludes hardware configuration
FroggersTigaPluginV2 editor SHALL use the VST/AU projection overlay and SHALL NOT show standalone hardware audio or MIDI device configuration.

#### Scenario: Device pages hidden in plugin
- **WHEN** the plugin editor opens
- **THEN** standalone audio device selectors are absent
- **THEN** standalone MIDI input device selectors are absent
- **THEN** carousel, Crunchy, ADSR, mod grid, scopes, and DAW-parameter-backed controls remain available
