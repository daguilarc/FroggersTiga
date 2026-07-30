## MODIFIED Requirements

### Requirement: Mod source cell width is fixed on module pages
Module row layout SHALL assign mod picker cells a fixed width of `DesktopV2ChromeLayout::kModCellW`. Mod cells MUST NOT expand to consume all remaining horizontal space in the module row.

#### Scenario: Mod dropdown is not full-row width
- **WHEN** the Audio module page renders at 1280×920
- **THEN** each mod lane picker cell width equals `kModCellW`
- **THEN** the dropdown is not wider than the mod cell footprint

### Requirement: Default standalone layout fits without module-page scroll
At 1280×920 with default chrome visible, carousel module pages whose manifest-visible rows fit the center grid SHALL NOT display vertical scrollbars.

#### Scenario: Audio page fits
- **WHEN** the operator opens the Audio module at 1280×920
- **THEN** all manifest-visible rows and mod cells are visible without scrolling the module viewport
- **THEN** the full 16-step sequencer region remains visible below the carousel

### Requirement: Global command band uses a readable grid at 1280px
The global-command band SHALL lay out controls on the shared 10px grid so scope radio labels, randomize buttons, and Crunchy are fully readable without ellipsis. Scope radio pairs SHALL NOT overlap buttons on the row above. Unused horizontal space right of the last control SHALL be eliminated by reflow or width fill, not left as a dead margin.

#### Scenario: Scope radios readable
- **WHEN** the global-command band renders at 1280px width
- **THEN** **All Scenes**, **Current Scene**, **All Steps**, and **Current Step** labels are not truncated to `"..."`
- **THEN** scope radios do not overlap Randomize All or Rand Mods buttons

#### Scenario: Shift is not shown as dead chrome
- **WHEN** the global-command band renders on desktop v2
- **THEN** an on-screen **Shift** toggle is absent (v2 has no held-gesture model)
