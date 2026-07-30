## ADDED Requirements

### Requirement: Single module grid layout authority
Desktop v2 SHALL compute carousel module-page, parameter-detail, and sequencer-region geometry through shared `DesktopV2ChromeLayout` helpers rather than independent hardcoded X/Y constants. The helpers SHALL define the center grid body, compact encoder-cell footprint, top/global chrome exclusion zones, the fixed 16-step sequencer region with its direction/speed icon strip, and any legacy column projections still needed during migration.

#### Scenario: Grid body derived from shared helper
- **WHEN** layout code queries column geometry at default 1280px carousel width
- **THEN** carousel module pages and parameter-detail pages derive their grid body from the same layout helper
- **THEN** the 16-step sequencer region and its two-row icon strip derive their bounds from the same layout helper
- **THEN** legacy `kModuleRowModX` placement is not duplicated in panel layout code

#### Scenario: Parameter cells use container-local placement
- **WHEN** `SubmodulePagePanel` or `AdsrPagePanel` places parameter encoder cells
- **THEN** cell origins are local to the manifest-declared grid container
- **THEN** the grid container origin is derived from the shared layout helper

### Requirement: Exclusive grid bounds at default window size
At default standalone size 1280x920, global controls and sequencer controls SHALL NOT intersect any visible module parameter cell or parameter-detail cell.

#### Scenario: No global-control/grid overlap on Audio page
- **WHEN** the app lays out module Audio at 1280x920
- **THEN** global controls intersect no visible module parameter cell bounds
- **THEN** the sequencer direction/speed icon strip and all 16 step cells intersect no visible module parameter cell bounds

#### Scenario: Layout bounds test gate
- **WHEN** `LayoutBounds_test` runs in desktop-v2 ctest
- **THEN** it fails if any module parameter cell or parameter-detail cell intersects global controls at default layout size

### Requirement: Default-size module pages avoid vertical scroll
Carousel module pages, parameter-detail pages, and the sequencer region SHALL avoid vertical scrolling at 1280x920. When a manifest module cannot fit in the default-size grid, it SHALL be split into named groups/subpages instead of hiding controls behind a default-size scrollbar.

#### Scenario: Audio module fits without scroll
- **WHEN** module Audio is shown at default 1280x920
- **THEN** the module page does not show a vertical scrollbar
- **THEN** all manifest-visible Audio parameter cells are visible without scrolling

#### Scenario: Sequencer strip fits without scroll
- **WHEN** the fixed 16-step sequencer is shown at default 1280x920
- **THEN** all step cells and both direction/speed icon rows are visible without scrolling
- **THEN** no sequencer icon label is truncated
