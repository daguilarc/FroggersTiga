# Delta — `froggers-vst-host`

**Added 2026-08-19**, superseding the remaining VST-host scope of
`frogg3rs-browser-and-vst-hosts` (which delivered the plugin, its
transport, tempo, parameter surface, and editor; the two ADDED
requirements were deliberately outside its automation-surface scope,
and the MODIFIED bus requirement is the "separate change" its own text
reserved for the day the core gained inputs — which this change is).

## ADDED Requirements

### Requirement: Session state survives the host project
THE plugin SHALL persist its full user-visible state through the host's
own state calls and restore it on reload, so that a project saved and
reopened presents the instrument exactly as it was left — including
parameter values the operator changed by hand, which no automation lane
would rewrite. Restoration SHALL go through the app's single parameter
authority, and SHALL NOT mutate the standalone application's own saved
patches as a side effect. The stored representation SHALL survive
parameter-model growth: a session saved before a bank or slot is added
SHALL still restore, without corruption, after it is.

#### Scenario: A saved project reopens unchanged
- **WHEN** the operator edits parameters by hand, saves the DAW project,
  closes it, and reopens it
- **THEN** the instrument's parameter values are the edited ones
- **AND** the host parameters read back those same values

#### Scenario: Project state is not the standalone's patch store
- **WHEN** a project restores plugin state
- **THEN** the standalone application's saved patches are unmodified

#### Scenario: An old session outlives model growth
- **WHEN** a session stored against a smaller parameter model is
  restored into a build whose model has grown
- **THEN** every stored parameter restores to its saved value
- **AND** parameters the stored session never knew keep their defaults

### Requirement: Automation does not steal the operator's view
THE plugin SHALL deliver an automated parameter's value to that
parameter's own bank and slot regardless of which bank the editor is
currently showing, and SHALL NOT leave the editor's visible page
oscillating when lanes in different banks are automated at once.
Whether automation moves the visible page at all is this change's
operator decision; whichever policy is chosen SHALL apply identically
to a single lane and to simultaneous lanes.

#### Scenario: Simultaneous cross-bank lanes
- **WHEN** two automation lanes drive parameters in two different banks
  at once
- **THEN** each value lands on its own bank's parameter
- **AND** the editor's visible page follows the chosen policy without
  oscillating between banks

## MODIFIED Requirements

### Requirement: Bus and MIDI posture match the core's real I/O
THE plugin SHALL present a stereo output bus and one optional audio
input bus that feeds the external-audio modulation sources: the bus
SHALL NOT be required for the plugin to instantiate or make sound, and
the external-audio sources SHALL connect when, and only when, the DAW
user has actually routed audio into it — an explicit act in the host,
which is the plugin's form of the affirmative routing the standalone
derives from device selection. The plugin SHALL NOT consume MIDI note
input (the core exposes no note-input seam): DAW MIDI reaches the
instrument exclusively through host-parameter mappings.

#### Scenario: Instrument scans with the declared layout
- **WHEN** the DAW scans and instantiates the plugin
- **THEN** it presents as an instrument with a stereo output and an
  optional audio input bus, and instantiates whether or not the host
  connects that bus
- **THEN** incoming MIDI notes are not required and produce no core-side
  effect

#### Scenario: Routing into the input bus connects the sources
- **WHEN** the DAW user routes a signal into the plugin's input bus
- **THEN** External Audio and External EF present as connected and
  modulate from that signal
- **WHEN** the bus is disconnected again
- **THEN** both sources return to their inert, disconnected state
