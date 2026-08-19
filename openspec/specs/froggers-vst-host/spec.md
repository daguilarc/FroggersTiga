# froggers-vst-host Specification

## Purpose
A JUCE plugin host (VST3 and AU) for the Sheaf-hosted app whose entire DAW-facing surface — transport, tempo, parameters, and editor chrome — is external by design: the DAW is the authority for everything a DAW touches, and the app core stays JUCE-free.

## Requirements
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
render internal transport controls — Play, Stop, and Record (recording is
the DAW's job: the host records the plugin's output natively). Freeze is a
musical control, not transport: it SHALL remain available as an automatable
parameter with the standalone latch semantics preserved, its surface button
SHALL stay rendered, and it SHALL gain a "FREEZE" text label beside it in
the row space the suppressed controls free (operator instruction
2026-08-18).

#### Scenario: Host play drives the instrument
- **WHEN** the DAW transport starts and later stops
- **THEN** the instrument starts and stops with it, one transport message
  per transition, with no internal play/stop control involved

#### Scenario: Freeze automatable, not transport-coupled
- **WHEN** the DAW automates the Freeze parameter while its transport runs
- **THEN** the freeze latch engages and releases per the standalone
  semantics, independent of the host transport state

#### Scenario: Freeze button labeled in the thinned row
- **WHEN** the plugin editor renders the transport row
- **THEN** Play, Stop, and Record are absent, the Freeze button renders,
  and a "FREEZE" text label sits beside it

### Requirement: Tempo is external via the DAW
WHEN hosted as a plugin, THE master clock SHALL follow the host tempo
through the core's existing external-clock slaving, and the BPM control
SHALL behave exactly as it does when slaved to external MIDI clock:
display-direction only, with user tempo requests suppressed.

#### Scenario: Host tempo drives the clock
- **WHEN** the DAW tempo changes while the plugin runs
- **THEN** the instrument's clock follows the host tempo
- **THEN** the BPM control displays the host tempo and does not accept a
  user tempo change

### Requirement: Bus and MIDI posture match the core's real I/O
THE plugin SHALL present a stereo output bus and no audio input bus (the
core requests zero input channels; external audio routing is the DAW's
job and arrives only if the core itself gains inputs, a separate change),
and SHALL NOT consume MIDI note input (the core exposes no note-input
seam): DAW MIDI reaches the instrument exclusively through host-parameter
mappings.

#### Scenario: Instrument scans with the declared layout
- **WHEN** the DAW scans and instantiates the plugin
- **THEN** it presents as an instrument with a stereo output and no audio
  input bus
- **THEN** incoming MIDI notes are not required and produce no core-side
  effect

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

