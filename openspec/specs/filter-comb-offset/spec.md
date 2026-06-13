# filter-comb-offset Specification

## Purpose

Filter row 0 (`DELF`) pre-comb pure delay: sim label **Comb offset**, monotonic 1 ms–100 ms mapping. Supersedes archived **Comb line** (display-only) from `delay-grain-filter-row0`.

## Requirements

### Requirement: Filter row 0 sim label is Comb offset

Sim hosts SHALL display **Comb offset** for Filter page row 0 (`DELF`). **Comb line**, **Pure delay**, and **Filter delay** SHALL NOT appear in sim UI.

#### Scenario: Desktop Filter panel

- **WHEN** the Filter submodule panel is visible
- **THEN** row 0 label reads **Comb offset**

#### Scenario: Web Filter page

- **WHEN** host page is Filter (3)
- **THEN** knob column 0 shows **Comb offset**

### Requirement: Comb offset maps 1 ms to 100 ms monotonically

Filter row 0 knob SHALL map exponentially from **0.001 s** at knob minimum to **0.1 s** at knob maximum. Higher knob position SHALL produce a longer pure-delay line before the comb.

#### Scenario: Knob sweep direction

- **WHEN** the user increases row 0 from minimum to maximum while listening
- **THEN** pre-comb smear increases (longer offset)

#### Scenario: Endpoints

- **WHEN** row 0 is at minimum
- **THEN** delay time is approximately **1 ms**
- **WHEN** row 0 is at maximum
- **THEN** delay time is approximately **100 ms**

### Requirement: PureDelay remains before comb

`ApplyOutputFx` SHALL keep `PureDelay` immediately before the comb filter. Row 4 **Comb delay** controls comb pitch separately.

#### Scenario: Firmware OLED

- **WHEN** running on Daisy Field hardware
- **THEN** OLED still shows `DELF` as documented

### Requirement: Quick Dict

`QUICK_DICT.md` SHALL list **`Comb offset : Short line before comb — smears strike, not pitch`**.
