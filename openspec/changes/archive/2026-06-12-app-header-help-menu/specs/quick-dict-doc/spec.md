## ADDED Requirements

> **Format superseded by `desktop-compact-layout` / `quick-dict-format`:** table-based entries replaced with `PRMT : Parameter Name` lines; sim UI labels (`VCO1` not `V1VO` on Audio). Document existence requirement below still applies.

### Requirement: Quick Dict document exists at repo root

The repository SHALL contain `QUICK_DICT.md` at the project root. It SHALL list abbreviated parameter and control meanings for FroggersTiga, including sim-host labels (six pages, Delay overlay, sim mod sources) in addition to Field hardware abbreviations referenced from `MANUAL.md`.

#### Scenario: Audio page abbreviations

- **WHEN** a user reads the Audio section of `QUICK_DICT.md`
- **THEN** entries exist for VCO1, VCO2, VCO3, XCPL, PM1A, PM2A, OLVL, and FUEG with one-line descriptions (`PRMT : Name` format after compact-layout)

#### Scenario: Delay sim page

- **WHEN** a user reads the Delay section
- **THEN** entries exist for DTIM, DSND, DFBK, DWID, DTON, DMOD, DMIX, and FUEG

#### Scenario: Sim mod sources

- **WHEN** a user reads the mod sources section
- **THEN** MIDI, VCO level, Marbles 1, and Marbles 2 are defined for sim hosts

#### Scenario: Field depth deferred to Manual

- **WHEN** a user needs pickup badges, M1–M7 assignment, or full signal-flow detail
- **THEN** `QUICK_DICT.md` directs them to **Manual** rather than duplicating full `MANUAL.md` sections
