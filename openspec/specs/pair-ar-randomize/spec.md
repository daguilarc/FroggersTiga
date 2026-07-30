# pair-ar-randomize Specification

## Purpose
Include all four pair-AR knobs in Audio page randomize, global Rand Mods, and host randomize parity across desktop, web, WASM, and VCV.
## Requirements
### Requirement: Pair-AR knobs randomize with Audio page Randomize

When the operator triggers **page-level knob randomize** on the Audio host page, the system SHALL randomize all four pair-sum A/R knob values (Attack 1+2, Release 1+2, Attack 2+3, Release 2+3) in addition to the eight vertical page rows.

#### Scenario: Desktop Audio panel Randomize

- **WHEN** the operator clicks **Randomize** on the Audio `SubModulePanel` (host page 0)
- **THEN** all four pair-AR knobs receive new values in the range 0.0–1.0
- **AND** the horizontal band sliders reflect the new values on the next UI refresh

#### Scenario: Web Audio page Rand

- **WHEN** the operator clicks the page **Rand** control while on the Audio page with the engine running
- **THEN** all four pair-AR knobs receive new values in the range 0.0–1.0
- **AND** the web knob row displays the updated values after `postScreen`

### Requirement: Pair-AR mod routes randomize with Audio page Randmod

When the operator triggers **page-level mod randomize** on the Audio host page, the system SHALL randomize mod source and depth for all four pair-AR parameters in addition to the eight vertical page rows.

#### Scenario: Desktop Audio panel Randmod

- **WHEN** the operator clicks **Randmod** on the Audio `SubModulePanel`
- **THEN** each pair-AR parameter receives a new mod depth in 0.0–1.0 and a valid random mod source assignment

#### Scenario: Web Audio page Rand Mod

- **WHEN** the operator clicks the page **Rand Mod** control on the Audio page
- **THEN** pair-AR mod sources and depths are randomized consistently with vertical row mod randomization

### Requirement: Pair-AR knobs included in global Rand All

When the operator triggers **global Rand All**, the system SHALL randomize pair-AR knobs together with all page knobs and Delay FX knobs on every host (desktop and WASM/web).

#### Scenario: Desktop global Rand All

- **WHEN** the operator clicks **Rand All** on the global strip
- **THEN** pair-AR knobs are randomized along with all page knobs and Delay knobs

#### Scenario: Web global Rand All

- **WHEN** the operator clicks **Rand All** in the web UI
- **THEN** pair-AR knobs are randomized along with all page knobs and Delay knobs

### Requirement: Shared host orchestration

Pair-AR randomize behavior SHALL be implemented once in shared host orchestration consumed by both `PagedHostIO` and `DesktopHostIO`; platform-specific duplicate randomize logic for pair-AR SHALL NOT exist.

#### Scenario: Single orchestration entry point

- **WHEN** either host performs page or global randomize that affects pair-AR
- **THEN** the mutation is routed through the shared helper that gates on `AudioPairArLayout::kAudioHostPage`

### Requirement: Unit test coverage

The sim test suite SHALL include tests that verify pair-AR knob values change after Audio page randomize and global Rand All via `PagedHostIO`.

#### Scenario: Page randomize test

- **WHEN** a unit test calls page randomize on host page 0 with known initial pair-AR knob values
- **THEN** at least one pair-AR knob value differs after the call

#### Scenario: Global randomize test

- **WHEN** a unit test calls global randomize all pages on a host with pair-AR state
- **THEN** pair-AR knob values differ from their pre-randomize values

