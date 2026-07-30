## ADDED Requirements

### Requirement: Public manuals document only launched hosts

Public operator manuals SHALL describe only host surfaces that are launched and intended for users. VST/AU and VCV Rack SHALL remain absent from `SIM_MANUAL.md`, `docs/sim-manual.md`, and `web/public/sim-manual.md` until a later launch/documentation change explicitly adds them back.

#### Scenario: SIM manual excludes unreleased hosts

- **WHEN** a reader opens `SIM_MANUAL.md`
- **THEN** it documents desktop standalone and web sim behavior
- **THEN** it does not present VST/AU, plugin, VCV, or Rack as supported user surfaces

#### Scenario: Published mirrors exclude unreleased hosts

- **WHEN** docs sync has run
- **THEN** `docs/sim-manual.md` and `web/public/sim-manual.md` match the launch-gated host scope of root `SIM_MANUAL.md`

### Requirement: Internal host specs may track pre-launch hosts

Internal OpenSpec host contracts, test plans, and development docs MAY continue to describe VST/AU and VCV behavior before launch. Public operator manuals SHALL NOT use those internal contracts as user-facing availability claims.

#### Scenario: Internal VST and VCV specs remain valid

- **WHEN** VST/AU or VCV implementation/testing work continues
- **THEN** OpenSpec host specs may retain VST/AU and VCV requirements
- **THEN** public SIM manual mirrors remain desktop/web-only until launch

### Requirement: Launch change restores host documentation deliberately

VST/AU or VCV documentation SHALL return to the public SIM manual only through a later explicit launch or documentation change that names the host, its supported status, and its verified user-facing behavior.

#### Scenario: Future VST launch updates docs

- **WHEN** VST/AU testing is complete and the host is ready to launch
- **THEN** a new change may add VST/AU sections to the SIM manual with verified behavior

#### Scenario: Future VCV launch updates docs

- **WHEN** VCV testing is complete and the module is ready to launch
- **THEN** a new change may add VCV sections to the SIM manual with verified behavior
