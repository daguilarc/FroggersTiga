# quick-dict-format Specification

## Purpose

Quick Dict line format and coverage. Left token = sim UI label (`ParamDisplayNames`); short gloss on the right.

## Requirements

### Requirement: Quick Dict uses Display name colon gloss lines

`QUICK_DICT.md` at the repo root SHALL list parameters as **`Display name : short gloss`** (one entry per line). The left token SHALL be the **sim UI row label** (`ParamDisplayNames` / `getRowName()`), not the 4-character firmware OLED name. Parameter sections SHALL NOT use markdown tables with Role or description columns.

#### Scenario: Audio page uses full display labels

- **WHEN** a user reads the Audio section of Quick Dict
- **THEN** rows include `VCO1 : …`, `Cross-coupler : …`, `Crunch : …`
- **AND** entries do **not** use firmware-only labels `V1VO`, `XCPL`, or `FUEG` as the left token

#### Scenario: Filter row 0

- **WHEN** a user reads Filter row 0 in Quick Dict
- **THEN** the line reads `Comb offset : Short line before comb — smears strike, not pitch`

#### Scenario: All six sim pages covered

- **WHEN** a user reads Quick Dict
- **THEN** entries exist for Audio, Marbles, Reverb, Filter, Drive, and Delay pages
- **AND** each line's left token matches the label shown on that sim page row

### Requirement: Transport and mod sources use same line format

Sections for transport controls and sim mod sources SHALL use **`LABEL : gloss`** lines, not markdown tables.

#### Scenario: Transport section format

- **WHEN** a user reads the transport section of Quick Dict
- **THEN** lines appear as `Play : Audio on/off`, `Rand Mods : All mod routes`, etc.

### Requirement: Depth deferred to Manual

Quick Dict SHALL include a single upfront note that full signal-flow and Field hardware detail (including OLED symbols like `FUEG`) live in **Manual**.

### Requirement: Web and desktop share the same Quick Dict text

`web/public/quick-dict.md` SHALL match the root `QUICK_DICT.md` after sync/build.
