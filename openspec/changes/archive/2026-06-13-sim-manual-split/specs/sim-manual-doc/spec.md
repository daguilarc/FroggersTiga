## ADDED Requirements

### Requirement: Sim operator manual exists

The repository SHALL contain `SIM_MANUAL.md` at the project root documenting sim host operation for desktop and web. It SHALL use sim display names from `ParamDisplayNames.hpp` for all per-page knob tables.

#### Scenario: Filter page uses sim names

- **WHEN** a reader opens `SIM_MANUAL.md` Filter section
- **THEN** row 0 is documented as **Comb offset**, not Pure delay or `DELF` as the primary label

#### Scenario: Firmware manual not duplicated

- **WHEN** reviewing `SIM_MANUAL.md`
- **THEN** it does not contain Daisy flash procedures, `SW1`/`SW2` hardware button workflow, or full Field signal-flow as the primary guide
- **AND** it includes a footer pointer to repository `MANUAL.md` for Field hardware

### Requirement: Sim manual covers host-specific behavior

`SIM_MANUAL.md` SHALL document sim transport (Play, Stop, External/Ext. In.), mod bay, page navigation, global randomize strip, and desktop-vs-web differences in clearly marked subsections.

#### Scenario: Web external audio

- **WHEN** the Web subsection is read
- **THEN** it explains External mic permission and default External off

#### Scenario: Desktop patch bay

- **WHEN** the Desktop subsection is read
- **THEN** it explains mod rack patch cables and MIDI at a high level
