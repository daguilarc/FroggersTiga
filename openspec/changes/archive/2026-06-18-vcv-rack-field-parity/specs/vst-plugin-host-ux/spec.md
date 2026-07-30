## ADDED Requirements

### Requirement: VST visual field parity matches desktop reference

The local JUCE VST/AU plugin SHALL expose the same readable labels, randomize controls, VCO morph buttons, and five-cell mod rack with CV scopes as the standalone desktop app, without requiring hover as the primary identification mechanism.

#### Scenario: Labels visible in DAW editor

- **WHEN** the VST editor opens at default size (1440×720)
- **THEN** all six submodule columns show row labels, column titles, and mod rack cell names
- **THEN** no SVG or nanosvg rendering path is involved

#### Scenario: Randomize surface present

- **WHEN** the user inspects the VST editor
- **THEN** global strip exposes Rand All, Rand Mods, Rand waveforms, and Random (marbles)
- **THEN** each submodule column exposes Randomize and Randmod buttons

#### Scenario: Mod rack scopes active during playback

- **WHEN** the DAW transports audio through the plugin
- **THEN** MIDI CC 1, MIDI CC 2, and VCO Envelope cells display live CV traces
- **THEN** Random 1 and Random 2 cells display green LEDs at threshold 0.55

### Requirement: VST plugin-host UX delegated to spin-off change

Plugin-hosted UX gaps (Record cluster, QWERTY MIDI, snapshot v2, DAW manual gates) SHALL be implemented in **`openspec/changes/vst-plugin-host-ux/`**, not in this change.

#### Scenario: VCV apply not blocked

- **WHEN** VCV field-parity tasks proceed
- **THEN** VST hosting fixes are out of scope for this change
- **THEN** VST remains reference host for visual A/B only
