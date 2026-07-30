## MODIFIED Requirements

### Requirement: v2-mod-source-cell-grid

Each module row SHALL lay out mod source cells at X = `kModuleRowCenterClusterX + kCenterGlobalClusterW + kModuleRowModGap` (**31u** from row left at default constants), with width `kModCellW` (**18u**). Mod cells SHALL NOT use `rowWidth - kModCellW` right-flush placement.

#### Scenario: Mod column adjacent to center cluster

- **WHEN** submodule page lays out a row at 1280px content width
- **THEN** mod dropdown left edge is **310px** from the row left (31u × 10px)
- **THEN** no horizontal void wider than **1u** appears between encoder column and mod column

#### Scenario: Mod label readable

- **WHEN** a mod source with a long catalog label is assigned (e.g. combined VCO source)
- **THEN** the dropdown shows full text without ellipsis at 1280×920 default size
