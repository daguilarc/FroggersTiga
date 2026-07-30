## MODIFIED Requirements

### Requirement: v2-layout-density-constants

`DesktopV2ChromeLayout.hpp` SHALL define center-cluster and density constants:

| Constant | Value |
|----------|-------|
| `kModuleRowCenterClusterX` | 15u (150px) — encoder column end + gap |
| `kCenterGlobalClusterW` | 15u (150px) |
| `kModuleRowModGap` | 1u (10px) — existing `kSectionGap` |
| `kModuleRowModX` | **derived** via `moduleRowColumns().modX` — not an independent magic offset |
| `kModCellW` | 18u (180px) minimum mod column width |
| `kSequencerH` | 18u (180px) |
| `kSequencerStepCellSize` | 3u (30px) |
| `kPerfMarblesColW` | 6u (60px) |
| `kPerfMarblesLabelH` | **2u** (20px) |

#### Scenario: Constants consumed by layout code

- **WHEN** submodule row layout computes mod cell bounds
- **THEN** it uses `moduleRowColumns(rowWidth).modX` and mod column width from the same struct
- **THEN** mod cells are placed at x=0 inside the dedicated mod column content rather than inside encoder viewport bounds at x=310 on a full-width content surface

#### Scenario: Standalone no longer uses kGlobalStripH

- **WHEN** standalone `MainComponent` computes vertical chrome budget
- **THEN** bottom global strip height is not subtracted from flex area
