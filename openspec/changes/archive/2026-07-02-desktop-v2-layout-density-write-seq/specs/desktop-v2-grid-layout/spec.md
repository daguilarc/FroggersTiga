## ADDED Requirements

### Requirement: v2-layout-density-constants

`DesktopV2ChromeLayout.hpp` SHALL define center-cluster and density constants:

| Constant | Value |
|----------|-------|
| `kModuleRowCenterClusterX` | 15u (150px) |
| `kCenterGlobalClusterW` | 15u (150px) |
| `kModuleRowModGap` | 1u (10px) — existing `kSectionGap` |
| `kModuleRowModX` | 31u (310px) — `kModuleRowCenterClusterX + kCenterGlobalClusterW + kModuleRowModGap` |
| `kModCellW` | 18u (180px) |
| `kSequencerH` | 18u (180px) |
| `kSequencerStepCellSize` | 3u (30px) |
| `kPerfMarblesColW` | 6u (60px) |

#### Scenario: Constants consumed by layout code

- **WHEN** submodule row layout computes mod cell bounds
- **THEN** it uses `kModuleRowModX` and `kModCellW` from the chrome header
- **THEN** `moduleRowModX(rowWidth)` right-flush helper is removed or deprecated

#### Scenario: Standalone no longer uses kGlobalStripH

- **WHEN** standalone `MainComponent` computes vertical chrome budget
- **THEN** bottom global strip height is not subtracted from flex area
