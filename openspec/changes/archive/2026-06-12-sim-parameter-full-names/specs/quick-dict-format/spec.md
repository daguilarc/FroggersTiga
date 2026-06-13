## MODIFIED Requirements

### Requirement: Quick Dict uses PRMT colon Name lines

`QUICK_DICT.md` at the repo root SHALL list parameters as **`Display name : short gloss`** (one entry per line). The left token SHALL be the **sim UI row label** (`ParamDisplayNames` / `getRowName()`), not the 4-character firmware OLED name. Parameter sections SHALL NOT use markdown tables with Role or description columns.

#### Scenario: Audio page uses full display labels

- **WHEN** a user reads the Audio section of Quick Dict
- **THEN** rows include `VCO1 : …`, `Cross-coupler : …`, `Crunch : …`
- **AND** entries do **not** use firmware-only labels `V1VO`, `XCPL`, or `FUEG` as the left token

#### Scenario: Reverb page entry format

- **WHEN** a user reads the Reverb section of Quick Dict
- **THEN** lines appear as `Wet/dry : …`, `Room size : …`, `Crunch : …`
- **AND** no table headers like `| ID | Role |` are present in parameter sections

#### Scenario: All six sim pages covered

- **WHEN** a user reads Quick Dict
- **THEN** entries exist for Audio, Marbles, Reverb, Filter, Drive, and Delay pages
- **AND** each line’s left token matches the label shown on that sim page row

### Requirement: Depth deferred to Manual

Quick Dict SHALL include a single upfront note that full signal-flow and Field hardware detail (including OLED symbols like `FUEG`) live in **Manual**. Quick Dict SHALL NOT duplicate multi-sentence knob descriptions from `MANUAL.md`.

#### Scenario: User needs decay time explanation

- **WHEN** a user needs to know what decay does in depth
- **THEN** Quick Dict shows only `Decay : …` with a short gloss
- **AND** the document directs the user to **Manual** for full behavior
