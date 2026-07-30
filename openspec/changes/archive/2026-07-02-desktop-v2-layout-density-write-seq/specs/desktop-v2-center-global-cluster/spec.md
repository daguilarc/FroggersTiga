## ADDED Requirements

### Requirement: v2-center-global-cluster-column

Standalone desktop v2 SHALL place global randomize actions, **Crunchy**, and **Shift** in a **center column** within the module/carousel area at `DesktopV2ChromeLayout::kModuleRowCenterClusterX`, with width `kCenterGlobalClusterW`. The column SHALL sit between encoder rings and mod source dropdowns — not in the bottom chrome strip.

#### Scenario: Center cluster visible in module area

- **WHEN** standalone desktop v2 renders any submodule page at 1280×920
- **THEN** Rand All, Rand Mods, Rand waveforms, Rand Resample, Crunchy, and Shift appear in the center column
- **THEN** no bottom global strip row is allocated in `MainComponent`

#### Scenario: Center cluster uses grid constants

- **WHEN** layout computes center column bounds
- **THEN** X offset is **15u** from the module row left edge (after 9u label + 5u encoder + 1u gap)
- **THEN** column width is **15u**

#### Scenario: Page-local randomize stays left

- **WHEN** submodule page renders Randomize and Randmod buttons
- **THEN** those buttons remain above the encoder viewport on the left
- **THEN** they are not duplicated in the center cluster

### Requirement: v2-center-cluster-host-callbacks

`CenterGlobalClusterV2` SHALL invoke the same `DesktopHostIO` / control-core mutations as today's `GlobalStripV2` for each control.

#### Scenario: Rand All parity

- **WHEN** the operator clicks Rand All in the center cluster
- **THEN** the same Rand All scope and exclusions apply as `desktop-v2-global-controls` v2-rand-all-scope

#### Scenario: Crunchy and Shift parity

- **WHEN** the operator turns Crunchy or toggles Shift in the center cluster
- **THEN** behavior matches prior global strip semantics (encoder turn, shift-held message bus)
