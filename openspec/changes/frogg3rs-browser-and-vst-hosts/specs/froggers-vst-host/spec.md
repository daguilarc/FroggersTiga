# Delta — `froggers-vst-host`

**Added 2026-08-18, at the operator's instruction.** New capability: a
plugin host for the current Sheaf-hosted app whose DAW-facing surface is
external by design. The retired wrappers' specs (`juce-vst-cc-mod-gating`,
`vst-v2-midi-modulation`) are REMOVED by this same change.

## ADDED Requirements

### Requirement: Plugin host wraps the JUCE-free core
THE VST host SHALL be a JUCE audio plugin (VST3 and AU, instrument) that
wraps the app core without modifying it: all JUCE code SHALL live in the
plugin host layer, and the core's no-JUCE gate SHALL remain in force and
green, unmodified.

#### Scenario: Core stays JUCE-free
- **WHEN** the app test suite runs after the plugin host lands
- **THEN** the no-JUCE gate passes with no changes to the gate or the core

### Requirement: Transport is external via the DAW
WHEN hosted as a plugin, THE transport SHALL be driven exclusively by the
DAW: host play-state transitions from the plugin playhead SHALL produce the
same transport messages the standalone UI produces, edge-triggered (exactly
one start or stop per host transition), and the plugin editor SHALL NOT
render internal transport controls. Freeze is a musical control, not
transport: it SHALL remain available as an automatable parameter with the
standalone latch semantics preserved.

#### Scenario: Host play drives the instrument
- **WHEN** the DAW transport starts and later stops
- **THEN** the instrument starts and stops with it, one transport message
  per transition, with no internal play/stop control involved

#### Scenario: Freeze automatable, not transport-coupled
- **WHEN** the DAW automates the Freeze parameter while its transport runs
- **THEN** the freeze latch engages and releases per the standalone
  semantics, independent of the host transport state

### Requirement: Parameters are external via a stable automation surface
THE plugin SHALL expose every user parameter of the six-bank model through
host parameters carrying a flat stable ID (for DAW automation and DAW-side
MIDI mapping) plus a grouped display name, bridged bidirectionally to the
app's single parameter authority; stable IDs SHALL be stable across
sessions and releases. THE plugin SHALL NOT implement internal MIDI
mapping or MIDI learn.

#### Scenario: DAW automation round-trip
- **WHEN** the DAW writes a host parameter and later reads it back
- **THEN** the app's parameter value follows the write and the readback
  matches, under the same value semantics the standalone surface uses

#### Scenario: MIDI mapping lives in the DAW
- **WHEN** the operator maps a MIDI controller to a plugin parameter in
  the DAW
- **THEN** the mapping works through the host parameter alone, with no
  plugin-side MIDI configuration

### Requirement: Editor hosts the portable surface
THE plugin editor SHALL render the same portable app surface the
standalone launcher renders (minus DAW-owned chrome: no internal
transport controls, no audio-device page), through the same portable
renderer, so surface improvements reach the plugin without a parallel UI.

#### Scenario: One surface, both hosts
- **WHEN** a change lands in the portable surface
- **THEN** the plugin editor renders it without plugin-specific UI work
  beyond the DAW-owned exclusions

## REMOVED Requirements

The capabilities `juce-vst-cc-mod-gating` and `vst-v2-midi-modulation` are
removed in their entirety: they specify the deleted `desktop/` /
`desktop-v2/` plugin wrappers (never tested at the AudioProcessor layer,
never built by CI). Their spec directories are deleted at archive-time
sync. The dual-identity parameter idea they contained is re-specified
above against the current model.
