## ADDED Requirements

### Requirement: Filter peaking EQ uses Peak labels

Filter page rows 1–3 (`BUPF`, `BUPR`, `BUPW`) SHALL display **Peak freq**, **Peak gain**, and **Peak Q** in sim UI. **Bump center**, **Bump gain**, and **Bump width** SHALL NOT appear.

#### Scenario: Desktop Filter panel

- **WHEN** the Filter submodule panel is visible
- **THEN** rows 1–3 read **Peak freq**, **Peak gain**, **Peak Q**

#### Scenario: Quick Dict

- **WHEN** the user reads Filter rows in Quick Dict
- **THEN** entries use Peak labels with glosses describing peaking EQ frequency, gain, and Q

### Requirement: Drive digital grit uses XOR and Bit depth

Drive page row 4 (`DIGR`) SHALL display **XOR**. Row 5 (`HASH`) SHALL display **Bit depth**. **Reorganizer** SHALL NOT appear in sim UI.

#### Scenario: Desktop Drive panel

- **WHEN** the Drive submodule panel is visible
- **THEN** row 4 is **XOR** and row 5 is **Bit depth**

#### Scenario: Quick Dict

- **WHEN** the user reads Drive rows in Quick Dict
- **THEN** `XOR : XOR bit mask on samples` and `Bit depth : Low-bit scramble depth`

### Requirement: Firmware OLED unchanged

Hardware Field OLED 4-character names (`BUPF`, `DIGR`, `HASH`, …) SHALL remain unchanged.
