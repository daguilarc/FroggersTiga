## ADDED Requirements

### Requirement: v2-mod-assignment-without-patch-cables
Desktop v2 SHALL assign modulation routes through lit source cells and dropdown menus per knob row; patch-cable drag routing SHALL NOT appear in v2.

#### Scenario: Lit cell shows active mod source
- **WHEN** a knob row has mod index 7 (VCO1 EF) assigned
- **THEN** the row's mod-source cell displays the VCO1 EF color and short label
- **THEN** the cell appears visually active (lit)

#### Scenario: Dropdown assigns mod source
- **WHEN** the user opens the mod-source dropdown on an unassigned row and selects VCO2 EF
- **THEN** `DesktopHostIO` records the assignment for that page row
- **THEN** the lit cell updates without patch-cable interaction

#### Scenario: Randomize mod respects v2 source catalog
- **WHEN** the user triggers per-panel Randmod or global Rand Mods
- **THEN** random assignments draw only from the v2 internal mod-source catalog (indices 7–14)
- **THEN** None (255) remains a valid cleared state

#### Scenario: Mod depth editing on assigned row
- **WHEN** a row has an active mod assignment
- **THEN** dragging the knob control edits mod depth (blend amount)
- **WHEN** no assignment is active
- **THEN** dragging edits the base parameter value

### Requirement: v2-eight-internal-mod-sources
Desktop v2 SHALL expose eight internal modulation sources with stable indices defined in `sim/V2ModSourceCatalog.hpp`.

| Index | Source |
|-------|--------|
| 7 | VCO1 envelope follower |
| 8 | VCO2 envelope follower |
| 9 | VCO3 envelope follower |
| 10 | VCO1+VCO2 envelope follower |
| 11 | VCO2+VCO3 envelope follower |
| 12 | VCO1+VCO2+VCO3 envelope follower |
| 13 | Random S&H 1 |
| 14 | Random S&H 2 |

#### Scenario: EF sources are modulatable
- **WHEN** any EF source index 7–12 is selected as a mod target in the control core
- **THEN** that EF tap MAY itself receive modulation depth from another v2 source per control-core rules

#### Scenario: Random S&H indices preserve engine wiring
- **WHEN** Random S&H 1 or 2 is assigned
- **THEN** modulation values SHALL come from the Random page S&H channels mapped to indices 13 and 14
