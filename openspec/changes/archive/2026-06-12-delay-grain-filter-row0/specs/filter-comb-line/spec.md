## ADDED Requirements

### Requirement: Filter row 0 sim display name is Comb line

Sim hosts SHALL display **Comb line** for Filter page row 0 (`DELF`). **Pure delay** and **Filter delay** SHALL NOT appear in sim UI.

#### Scenario: Desktop Filter panel

- **WHEN** the Filter submodule panel is visible
- **THEN** row 0 label reads **Comb line**

#### Scenario: Web Filter page

- **WHEN** host page is Filter (3)
- **THEN** knob column 0 and OLED row 0 show **Comb line**

### Requirement: Core PureDelay chain unchanged

`ApplyOutputFx` SHALL keep `PureDelay` before the comb filter. This rename is display-only for sim hosts.

#### Scenario: Firmware

- **WHEN** running on Daisy Field hardware
- **THEN** OLED still shows `DELF` as documented

### Requirement: Quick Dict

`QUICK_DICT.md` SHALL list **`Comb line : Short delay line before comb`**.
