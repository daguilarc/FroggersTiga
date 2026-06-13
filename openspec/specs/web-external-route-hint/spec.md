# web-external-route-hint Specification

## Purpose
TBD - created by archiving change web-ext-in-meter. Update Purpose after archive.
## Requirements
### Requirement: Silent capture status hint

When **External** is on and audio is playing but peak external input stays below the silence threshold for at least one second, the web sim SHALL show a short diagnostic in the `#status` line indicating input is silent and suggesting mic permission or level checks.

#### Scenario: Sustained silence

- **WHEN** **External** is on, **Play** is running, and `inputPeak` from `screen` stays near zero for ~1 s
- **THEN** status includes a silent-input hint
- **AND** the hint mentions browser mic permission and/or signal level

#### Scenario: Signal returns

- **WHEN** peak rises above the silence threshold after a silent period
- **THEN** the silent-input hint is removed from status
- **AND** the meter fill reflects the new peak

#### Scenario: External off

- **WHEN** the user turns **External** off
- **THEN** no silent-input hint is shown
- **AND** the meter returns to idle

