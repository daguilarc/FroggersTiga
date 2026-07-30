## ADDED Requirements

### Requirement: v2-seven-module-carousel
Desktop v2 SHALL present one **module** at a time via a carousel labeled **Module** (not Scene), with seven host pages: Audio (0), Random (1), Reverb (2), Filter (3), Drive (4), Delay (5), ADSR (6).

#### Scenario: Default module on launch
- **WHEN** desktop v2 launches
- **THEN** module Audio (index 0) is selected
- **THEN** the carousel title reads **Module: Audio** (or equivalent from `V2ParamDisplayNames`)

#### Scenario: Carousel wraps seven modules
- **WHEN** the user advances right from ADSR (index 6)
- **THEN** the carousel wraps to Audio (index 0)
- **WHEN** the user advances left from Audio
- **THEN** the carousel wraps to ADSR (index 6)

#### Scenario: Module pages exclude pair-AR band
- **WHEN** any module page is visible
- **THEN** the v1 Audio pair-AR four-knob band is not shown
- **THEN** per-VCO envelope shaping is edited only on the ADSR module page

### Requirement: v2-module-row-counts
Module pages SHALL use ten-row layouts where specified in `desktop-v2-module-expansion`. **Audio** is excluded from row additions.

#### Scenario: Audio unchanged
- **WHEN** the Audio module is visible
- **THEN** the Audio page shows its existing row layout with no v2 expansion rows

#### Scenario: Expanded FX pages show ten rows
- **WHEN** any module page 1–5 is visible
- **THEN** exactly ten encoder rows are shown
- **THEN** Crispy is row 9

#### Scenario: ADSR ten rows
- **WHEN** the ADSR module is visible
- **THEN** exactly ten encoder rows are shown (rows 0–8 A/S/R, row 9 Crispy)

#### Scenario: Labels from display authority
- **WHEN** any module row is rendered
- **THEN** labels come from `V2ParamDisplayNames` without duplicate tables in UI code
