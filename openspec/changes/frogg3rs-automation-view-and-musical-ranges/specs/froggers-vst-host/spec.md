# Delta — `froggers-vst-host`

**Added 2026-08-20.** Both requirements were written for the predecessor
change and never satisfied: cross-bank automation was decided but not
implemented, and the input bus was reserved for the day the core gained
inputs, which has now happened. They move here rather than syncing as
promises the code does not keep.

## ADDED Requirements

### Requirement: Automation does not steal the operator's view
THE plugin SHALL deliver an automated parameter's value to that
parameter's own bank and slot regardless of which bank the editor is
currently showing, and regardless of what page that bank is displaying.
The editor's visible page SHALL follow operator selection only:
automation SHALL NOT move it, for a single lane or for simultaneous
lanes, and SHALL NOT close a modulation view the operator has open.

#### Scenario: Simultaneous cross-bank lanes
- **WHEN** two automation lanes drive parameters in two different banks
  at once
- **THEN** each value lands on its own bank's parameter
- **AND** the editor's visible page does not move

#### Scenario: The operator is drilled into an automated bank
- **WHEN** the operator has a modulation view open on a bank and a lane
  automates a parameter in that same bank
- **THEN** the value lands on that parameter
- **AND** the open modulation view is neither closed nor written to

#### Scenario: Operator selection still moves the page
- **WHEN** the operator selects a different bank
- **THEN** the editor's visible page follows that selection

## MODIFIED Requirements

### Requirement: Bus and MIDI posture match the core's real I/O
THE plugin SHALL present a stereo output bus and one optional audio
input bus that feeds the external-audio modulation sources: the bus
SHALL NOT be required for the plugin to instantiate or make sound. The
external-audio sources SHALL start disconnected and SHALL connect when,
and only when, the operator has explicitly opted the input in AND that
bus carries at least one channel. An enabled bus alone SHALL NOT count
as consent: a host may enable an optional input bus with nothing routed
into it, and a channel existing has never meant the operator asked for
it. This is the same contract the standalone applies through device
selection, whose default is likewise no device. The plugin SHALL NOT consume MIDI note
input (the core exposes no note-input seam): DAW MIDI reaches the
instrument exclusively through host-parameter mappings.

#### Scenario: Instrument scans with the declared layout
- **WHEN** the DAW scans and instantiates the plugin
- **THEN** it presents as an instrument with a stereo output and an
  optional audio input bus, and instantiates whether or not the host
  connects that bus
- **THEN** incoming MIDI notes are not required and produce no core-side
  effect

#### Scenario: Opting the input in connects the sources
- **WHEN** the DAW user routes a signal into the plugin's input bus and
  the operator opts that input in
- **THEN** External Audio and External EF present as connected and
  modulate from that signal
- **WHEN** either the opt-in is withdrawn or the bus is disconnected
- **THEN** both sources return to their inert, disconnected state

#### Scenario: A host-enabled bus nobody asked for stays inert
- **WHEN** the host instantiates the plugin with the input bus enabled
  but the operator has not opted the input in
- **THEN** External Audio and External EF remain disconnected and take
  no part in randomization

#### Scenario: The host constrains what can be selected
- **WHEN** the host provides the input bus with a given channel layout
- **THEN** the input selection offers only None and the channels that
  layout actually provides
- **WHEN** the bus is disabled or provides no channels
- **THEN** the only available selection is None

#### Scenario: A selection the host takes away
- **WHEN** a channel is selected and the host then changes the bus
  layout so that channel no longer exists
- **THEN** the selection falls back to None and both sources go inert,
  rather than reading a channel that is gone

#### Scenario: The opt-in survives the project
- **WHEN** a project saved with the input opted in is reopened
- **THEN** the opt-in is restored
- **WHEN** a project predating the setting is reopened
- **THEN** the input is off
