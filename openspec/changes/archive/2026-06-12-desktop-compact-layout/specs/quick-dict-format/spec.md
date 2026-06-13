## ADDED Requirements

### Requirement: Quick Dict uses PRMT colon Name lines

`QUICK_DICT.md` at the repo root SHALL list parameters as **`PRMT : Parameter Name`** (one entry per line). **PRMT** SHALL be the **sim UI row label** (desktop `getRowName()` / web equivalent), not internal firmware OLED names where they differ. Parameter sections SHALL NOT use markdown tables with Role or description columns.

#### Scenario: Audio page uses sim display labels

- **WHEN** a user reads the Audio section of Quick Dict
- **THEN** rows 0–2 appear as `VCO1 : …`, `VCO2 : …`, `VCO3 : …`
- **AND** entries do **not** use firmware-only labels `V1VO`, `V2VO`, or `V3VO`

#### Scenario: Reverb page entry format

- **WHEN** a user reads the Reverb section of Quick Dict
- **THEN** lines appear as `RVMX : Wet/dry`, `RSIZ : Room size`, etc.
- **AND** no table headers like `| ID | Role |` are present in parameter sections

#### Scenario: All six sim pages covered

- **WHEN** a user reads Quick Dict
- **THEN** entries exist for Audio, Marbles, Reverb, Filter, Drive, and Delay pages
- **AND** each line’s left token matches the label shown on that sim page row

### Requirement: Transport and mod sources use same line format

Sections for transport controls and sim mod sources SHALL use **`LABEL : Name`** lines, not markdown tables. Examples: `Rand waves : Randomize VCO morph`, `MIDI : QWERTY piano or hardware notes → pitch CV`.

#### Scenario: Transport section format

- **WHEN** a user reads the transport section of Quick Dict
- **THEN** lines appear as `Play : Audio on/off`, `Randmod all : All mod routes`, etc.
- **AND** no `| Control | Action |` table headers are present

### Requirement: Depth deferred to Manual

Quick Dict SHALL include a single upfront note that full signal-flow and Field hardware detail live in **Manual**. Quick Dict SHALL NOT duplicate multi-sentence knob descriptions from `MANUAL.md`.

#### Scenario: User needs decay time explanation

- **WHEN** a user needs to know what `RDEC` does in depth
- **THEN** Quick Dict shows only `RDEC : Decay` (or equivalent short name)
- **AND** the document directs the user to **Manual** for full behavior

### Requirement: Web and desktop share the same Quick Dict text

`web/public/quick-dict.md` SHALL match the root `QUICK_DICT.md` after sync/build.

#### Scenario: Help menu Quick Dict

- **WHEN** the user opens Quick Dict from the desktop Help menu
- **THEN** the embedded text matches the `PRMT : Name` format
