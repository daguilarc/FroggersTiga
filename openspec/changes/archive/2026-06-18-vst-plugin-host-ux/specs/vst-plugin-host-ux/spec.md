## ADDED Requirements

### Requirement: Visual field parity with desktop (baseline — already met)

The local JUCE VST/AU plugin SHALL expose the same readable labels, randomize controls, VCO morph buttons, five-cell mod rack with CV scopes, and `PatchCableOverlay` drag-patch UI as the standalone desktop app.

#### Scenario: Labels visible in DAW editor

- **WHEN** the VST editor opens at default size (1440×720)
- **THEN** all six submodule columns show row labels, column titles, and mod rack cell names

#### Scenario: Randomize surface present

- **WHEN** the user inspects the VST editor
- **THEN** global strip exposes Rand All, Rand Mods, Rand waveforms, and Random (marbles)
- **THEN** each submodule column exposes Randomize and Randmod buttons

### Requirement: Hosted chrome hides standalone transport artifacts

When `AudioEngine` is constructed with plugin-hosted mode, the editor SHALL NOT show standalone-only transport UI.

#### Scenario: Record cluster hidden in DAW

- **WHEN** the plugin runs inside a DAW
- **THEN** Play, Stop, and Audio settings are not visible
- **THEN** Record/Export cluster is not visible

### Requirement: QWERTY MIDI disabled when plugin-hosted

Computer-keyboard MIDI capture SHALL NOT run when the plugin is hosted in a DAW.

#### Scenario: DAW typing not captured as MIDI

- **WHEN** the VST editor has focus, the plugin is hosted, and the user types in the DAW
- **THEN** QWERTY key events do not update MIDI CC 1 mod level — including when Computer Keyboard is enabled in MIDI settings

#### Scenario: Standalone unchanged

- **WHEN** the standalone desktop app runs
- **THEN** QWERTY MIDI behavior remains as today

### Requirement: Preset snapshot completeness (v3)

`getStateInformation` / `setStateInformation` SHALL persist and restore knob values, mod routing, delay sidecar, pair-AR state, VCO morph indices, and MIDI CC pair enable/channel/number configuration.

#### Scenario: Morph recall

- **WHEN** user cycles VCO1 morph in plugin, saves DAW project, and reloads
- **THEN** VCO1 morph index restores to the saved value

#### Scenario: CC config recall

- **WHEN** user disables CC2 or changes CC channel/number, saves, and reloads
- **THEN** CC gating and routing config restores

#### Scenario: v1 and v2 snapshot backward compatibility

- **WHEN** DAW loads a project saved with snapshot version 1 or 2
- **THEN** knob and mod state restore; newer fields use documented defaults

### Requirement: Plugin editor minimum size

The plugin editor minimum size SHALL be 1440×720 pixels, matching `HostPanelLayout` defaults.

#### Scenario: Minimum resize

- **WHEN** user resizes editor to minimum
- **THEN** mod rack scopes and column labels remain readable without clipping

### Requirement: Manual DAW verification gate

The change SHALL NOT be marked complete until documented manual DAW tests pass.

#### Scenario: DAW load smoke test

- **WHEN** reviewer loads VST3 in one macOS DAW
- **THEN** six submodule panels render, audio passes, mod rack scopes animate during playback

#### Scenario: VST vs desktop A/B

- **WHEN** the same snapshot state is used in standalone and VST
- **THEN** audio output matches within expected float tolerance for a documented spot-check patch set

### Requirement: Local-only build policy

VST sources and binaries SHALL NOT be published on public GitHub `main`. `BUILD_VST` SHALL default OFF in `desktop/CMakeLists.txt`.

#### Scenario: Public CI

- **WHEN** GitHub Actions runs on `main`
- **THEN** no VST3/AU artifact is produced
