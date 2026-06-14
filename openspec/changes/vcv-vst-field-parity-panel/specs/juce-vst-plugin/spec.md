## ADDED Requirements

### Requirement: VST3 and AU plugin targets

The build system SHALL produce VST3 and AU (macOS) plugin formats from the same JUCE UI sources as the desktop standalone app.

#### Scenario: Plugin loads in DAW

- **WHEN** user scans for plugins in a supported DAW
- **THEN** FroggersTiga appears as an effect/instrument plugin and loads without missing-symbol errors

#### Scenario: Shared UI parity

- **WHEN** the plugin editor opens
- **THEN** the user sees six submodule panels, mod rack (including VCO Envelope CV scope), patch overlay, and global strip matching the standalone desktop layout; the editor is user-resizable

#### Scenario: Audio row 7 label parity

- **WHEN** the plugin Audio panel renders row 7
- **THEN** the label is **Phase mod 3** from `ParamDisplayNames`, matching website and standalone desktop

#### Scenario: Delay via DelayState

- **WHEN** the plugin Delay panel knobs are adjusted
- **THEN** `DelayState` drives FX insert behavior identically to standalone desktop

### Requirement: Resizable plugin editor

The VST/AU editor SHALL be resizable with a minimum size that preserves mod rack scope readability (default design: 1440×720 minimum, matching desktop default).

#### Scenario: User resizes plugin window

- **WHEN** user drags the plugin editor larger in the DAW
- **THEN** the full panel layout scales or reflows per existing `MainComponent` resize behavior without clipping the VCO Envelope scope

### Requirement: Plugin audio path

The plugin SHALL process stereo (or mono) audio through `DesktopHostIO` + `FroggersEngine` at the DAW sample rate.

#### Scenario: Sample rate change

- **WHEN** DAW changes project sample rate
- **THEN** `DesktopHostIO::SetSampleRate` is called and audio continues without crash

#### Scenario: Transport sync

- **WHEN** DAW transport is stopped
- **THEN** plugin audio output is silent or bypassed per design default documented in plugin manual

### Requirement: Standalone app unchanged

Enabling the plugin target SHALL NOT remove or break the existing `FroggersTigaDesktop` standalone application target.

#### Scenario: Dual target build

- **WHEN** developer runs desktop release build
- **THEN** both standalone `.app` / installer and plugin binaries build from one CMake configure (flags permitting)

### Requirement: MIT licensing for plugin

The plugin target SHALL link only MIT-licensed engine code and JUCE; it SHALL NOT link Rack SDK or GPL VCV code.

#### Scenario: License separation

- **WHEN** plugin binary is distributed
- **THEN** license bundle lists MIT for engine + JUCE; no GPL Rack SDK artifacts are included
