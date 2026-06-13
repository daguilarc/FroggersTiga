## MODIFIED Requirements

### Requirement: Quick Dict document exists at repo root

The repository SHALL contain `QUICK_DICT.md` at the project root. It SHALL list abbreviated parameter and control meanings for FroggersTiga, including sim-host labels (six pages, Delay overlay, sim mod sources) in addition to Field hardware abbreviations where still useful as cross-reference.

#### Scenario: Audio page abbreviations

- **WHEN** a user reads the Audio section of `QUICK_DICT.md`
- **THEN** entries exist for VCO1, VCO2, VCO3, XCPL, PM1A, PM2A, OLVL, and FUEG with one-line descriptions (`Display name : gloss` format per `quick-dict-format`)

#### Scenario: Delay sim page

- **WHEN** a user reads the Delay section
- **THEN** entries exist for DTIM, DSND, DFBK, DWID, DTON, DMOD, DMIX, and FUEG

#### Scenario: Sim mod sources

- **WHEN** a user reads the mod sources section
- **THEN** MIDI, VCO level, Marbles 1, and Marbles 2 are defined for sim hosts

#### Scenario: Sim depth deferred to in-app Manual

- **WHEN** a user needs full sim signal-flow or per-page knob behavior
- **THEN** `QUICK_DICT.md` directs them to in-app **Manual** (`SIM_MANUAL.md`)
- **AND** does not duplicate multi-paragraph sections from the sim manual

#### Scenario: Field depth deferred to repo manual

- **WHEN** a user needs pickup badges, M1–M7 assignment, or full Field signal-flow detail
- **THEN** `QUICK_DICT.md` directs them to repository `MANUAL.md`
- **AND** does not imply the in-app Manual is the Field operator guide

### Requirement: Quick Dict points to sim Manual

`QUICK_DICT.md` SHALL state that the in-app **Manual** is the full sim operator guide. It SHALL direct readers to repository `MANUAL.md` only for Daisy Field hardware detail.

#### Scenario: Intro blurb

- **WHEN** a reader opens `QUICK_DICT.md`
- **THEN** the header does not imply the in-app Manual is the firmware Field manual
- **AND** it references in-app **Manual** for sim operation
