## ADDED Requirements

### Requirement: Single module row column authority

Desktop v2 SHALL compute module row column geometry through one function `DesktopV2ChromeLayout::moduleRowColumns(int rowWidth)` returning label width, encoder width, gap width, label+encoder width, center column X/width, mod column X/width, and content width. Submodule panel, ADSR panel, and page carousel SHALL consume this function — not independent hardcoded X constants.

#### Scenario: Mod X derived from column sum

- **WHEN** layout code queries column geometry at default 1280px carousel width
- **THEN** mod column X equals label width + encoder width + section gap + center cluster width + section gap
- **THEN** `kModuleRowModX` literal is not duplicated in panel layout code

#### Scenario: Three callers share one layout

- **WHEN** `PageCarouselComponent`, `SubmodulePagePanel`, and `AdsrPagePanel` lay out at the same window width
- **THEN** all three derive bounds from `moduleRowColumns`

#### Scenario: Container-local mod placement

- **WHEN** `SubmodulePagePanel` or `AdsrPagePanel` places a `ModSourceCell`
- **THEN** the cell X origin is 0 inside mod column content
- **THEN** the mod column container X origin is derived from `moduleRowColumns(rowWidth).modX`

### Requirement: Exclusive column bounds at default window size

At default standalone size 1280×920, center global cluster bounds SHALL NOT intersect any visible mod source cell bounds on the Audio module page. Center cluster and mod column SHALL occupy disjoint horizontal bands.

#### Scenario: No cluster-mod overlap on Audio page

- **WHEN** the app lays out module Audio at 1280×920
- **THEN** `CenterGlobalClusterV2::getBounds()` intersects no visible `ModSourceCell` bounds

#### Scenario: Layout bounds test gate

- **WHEN** `LayoutBounds_test` runs in desktop-v2 ctest
- **THEN** it fails if any mod cell intersects the center cluster at default layout size

### Requirement: Vertical scroll only when document exceeds viewport

Encoder row viewports SHALL enable vertical scrolling only when `encoderDocumentHeight(rowCount)` exceeds available viewport height. When the document fits, scrollbars SHALL be hidden and view position reset to top.

#### Scenario: Audio module fits without scroll

- **WHEN** module Audio is shown at default 1280×920 with eight visible rows
- **THEN** the encoder viewport does not show a vertical scrollbar
- **THEN** all eight rows are visible without scrolling

#### Scenario: Ten-row module scrolls when needed

- **WHEN** a ten-row expanded module page exceeds available viewport height at default window size
- **THEN** vertical scrolling is enabled
- **THEN** encoder and mod columns stay vertically aligned during scroll
