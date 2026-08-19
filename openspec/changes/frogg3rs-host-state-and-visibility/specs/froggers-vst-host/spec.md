# Delta — `froggers-vst-host`

**Added 2026-08-19**, superseding the remaining VST-host scope of
`frogg3rs-browser-and-vst-hosts` (which delivered the plugin, its
transport, tempo, parameter surface, and editor; these two requirements
were deliberately outside its automation-surface scope).

## ADDED Requirements

### Requirement: Session state survives the host project
THE plugin SHALL persist its full user-visible state through the host's
own state calls and restore it on reload, so that a project saved and
reopened presents the instrument exactly as it was left — including
parameter values the operator changed by hand, which no automation lane
would rewrite. Restoration SHALL go through the app's single parameter
authority, and SHALL NOT mutate the standalone application's own saved
patches as a side effect.

#### Scenario: A saved project reopens unchanged
- **WHEN** the operator edits parameters by hand, saves the DAW project,
  closes it, and reopens it
- **THEN** the instrument's parameter values are the edited ones
- **AND** the host parameters read back those same values

#### Scenario: Project state is not the standalone's patch store
- **WHEN** a project restores plugin state
- **THEN** the standalone application's saved patches are unmodified

### Requirement: Automation does not steal the operator's view
WHEN the DAW automates parameters, THE plugin editor's visible page
SHALL behave per the operator's chosen policy for view-follows-
automation, consistently for a single lane and for simultaneous lanes
across different banks; whatever the policy, the automated parameter's
value SHALL reach the correct bank and slot.

#### Scenario: Simultaneous cross-bank lanes
- **WHEN** two automation lanes drive parameters in two different banks
  at once
- **THEN** each value lands on its own bank's parameter
- **AND** the editor's visible page follows the chosen policy without
  oscillating between banks
