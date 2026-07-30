## ADDED Requirements

### Requirement: vst-v2-full-parameter-surface
FroggersTigaPluginV2 SHALL expose every v2-desktop-persistent control as a JUCE `AudioProcessorParameter`, including module knobs, ADSR rows, modulation depths, global Crunchy, scene/gesture values, delay sidecar, and VCO morph.

#### Scenario: Parameter count exceeds v1 inventory
- **WHEN** `HostParameterInventoryV2.hpp` is generated
- **THEN** the count is greater than 107 and documented with stable grouped IDs (e.g. `Module/Audio/VCO1`, `Global/Crunchy`, `ADSR/AtkVCO1`)
- **THEN** `HostParameterProcessorV2_test` asserts the exact count

#### Scenario: v1 plugin remains unchanged
- **WHEN** `BUILD_VST=ON` builds FroggersTigaPlugin (v1)
- **THEN** parameter count remains 107 and `acceptsMidi()` remains false

### Requirement: vst-v2-dual-parameter-ids
Each FroggersTigaPluginV2 host parameter SHALL have a **flat stable ID** for automation/MIDI mapping and a **grouped display name** for DAW browser trees.

#### Scenario: Flat ID for automation
- **WHEN** a host queries parameter ID for `Global/Crunchy`
- **THEN** the stable ID is a single kebab-case or snake identifier (e.g. `global_crunchy`) unchanged across versions within v2 major line

#### Scenario: Grouped display name
- **WHEN** the DAW shows the parameter list
- **THEN** display name uses slash grouping (e.g. `Global/Crunchy`, `Module/Audio/VCO1`, `ADSR/AtkVCO1`, `Sequencer/BPM`)

#### Scenario: Both IDs in manifest
- **WHEN** `HostParameterInventoryV2.hpp` is generated
- **THEN** every entry includes `stableId` and `displayName` fields
- **THEN** `HostParameterProcessorV2_test` validates uniqueness of both

### Requirement: vst-v2-daw-midi-to-any-parameter
FroggersTigaPluginV2 SHALL accept MIDI input from the DAW and SHALL allow any MIDI message type the DAW maps to modulate any exposed parameter through host parameter routing.

#### Scenario: MIDI CC modulates global Crunchy
- **WHEN** the DAW maps CC 1 to `Global/Crunchy`
- **THEN** global fuegoization follows CC at block boundaries

#### Scenario: No duplicate raw MIDI mod path
- **WHEN** MIDI arrives on the plugin bus
- **THEN** modulation is applied only through host parameter mapping

### Requirement: vst-v2-editor-parity-with-desktop-v2
FroggersTigaPluginV2 editor SHALL embed v2 chrome including Crunchy encoder, ADSR module carousel page, and encoder rings minus standalone transport/record/device settings.

#### Scenario: Hosted editor hides device settings
- **WHEN** the plugin editor opens in a DAW
- **THEN** audio device and record/export clusters are hidden
- **THEN** module carousel, Crunchy, mod grid, and scopes remain functional

### Requirement: vst-v2-stereo-output-default
FroggersTigaPluginV2 SHALL default to stereo output and mono input buses per `desktop-v2-audio-io`.

#### Scenario: Mono output host downmix
- **WHEN** the host provides only one output channel
- **THEN** the plugin processes without error and downmixes stereo spread to mono
