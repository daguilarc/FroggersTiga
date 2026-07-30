## ADDED Requirements

### Requirement: UI mutations apply when DAW transport is stopped

When the plugin is hosted, pending `DesktopHostIO` mutations from UI controls SHALL drain on the editor timer even if `AudioEngine::isAudioRunning()` is true.

#### Scenario: Randmod with transport stopped

- **WHEN** the user presses Randmod on the Audio column while the DAW transport is stopped
- **THEN** mod source and depth for that page update within one editor timer period
- **THEN** audio reflects the new routes when playback resumes

#### Scenario: No duplicate mutation apply

- **WHEN** mutations drain on both timer and audio `tickControls`
- **THEN** each enqueued mutation applies exactly once

### Requirement: Patch cable overlay reflects modIndex after bulk state changes

`PatchCableOverlay` SHALL redraw cables from host `modIndex` state after preset load and after Randmod / Rand Mods mutations.

#### Scenario: Randmod reroutes visible cables

- **WHEN** the user presses Randmod on a column with existing mod cables
- **THEN** cables on the overlay connect mod rack outputs to knob mod inputs matching the new `modIndex` assignments
- **THEN** connections with `modIndex == 255` show no cable

#### Scenario: Preset reload restores cable view

- **WHEN** DAW reloads a saved project with mod routes
- **THEN** overlay cables match restored `modIndex` without manual repatching

#### Scenario: Manual patch unchanged

- **WHEN** the user drags a cable on the overlay in VST
- **THEN** behavior matches standalone (sets `modIndex` via host enqueue)

### Requirement: VST mod routing uses closed desktop model

VST mod routing SHALL use the five-cell mod rack as the sole mod source outputs and per-knob mod inputs on submodule columns — not DAW-exposed CV outputs for CC modulation.

#### Scenario: MIDI CC mod without hardware MIDI Out

- **WHEN** the plugin is hosted and receives MIDI CC via `processBlock`
- **THEN** CC1/CC2 mod sources update via `CvMidiBridge` the same as standalone
- **THEN** user may patch MIDI CC mod rack cells to knob mod inputs on the overlay

### Requirement: Cross-host parity reference

VST Randmod behavior SHALL match standalone desktop DSP and serve as the reference for VCV Randmod implementation (VCV uses Rack `Cable` objects instead of `PatchCableOverlay`).

#### Scenario: Same engine call

- **WHEN** Randmod fires on VST Audio page
- **THEN** `RandomizePageModWithExtras` runs with the same `CvMidiBridge` gating as standalone
