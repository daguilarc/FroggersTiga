# Delta — `froggers-sheaf-runtime-app`

## ADDED Requirements

### Requirement: The MIDI configuration page offers this instrument's controls

The MIDI configuration page SHALL offer exactly the controls this
application registers — its parameter banks and named actions — and none
of any other application's. Resetting the mappings SHALL install this
application's own registered defaults. Every offered target SHALL dispatch
into a real action of this application, verified by a check that walks the
offered list.

#### Scenario: The offered targets are this app's

- **WHEN** the operator opens the MIDI configuration page
- **THEN** every mappable target names a control this application actually
  has, and no sample-application target appears

#### Scenario: Reset lands on this app's defaults

- **WHEN** the operator resets the MIDI mappings
- **THEN** the installed profile is the one this application registered as
  its default for the connected device

#### Scenario: No offered target is a dead end

- **WHEN** the offered-target list is enumerated by the check
- **THEN** each target dispatches into a routed action, and a target that
  routes nowhere fails the check by name
