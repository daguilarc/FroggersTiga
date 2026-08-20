# Delta — `froggers-vst-host`

**Added 2026-08-19.** Session persistence for the plugin, which the
predecessor change left as no-op host state calls.

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
