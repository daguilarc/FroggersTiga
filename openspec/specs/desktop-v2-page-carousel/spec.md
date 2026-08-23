# desktop-v2-page-carousel Specification

## Purpose
Desktop v2 presents one module at a time through a seven-page "Module" carousel (Audio, Random, Reverb, Filter, Drive, Delay, ADSR), owns the active module body's geometry through shared `DesktopV2ChromeLayout` helpers, and excludes the v1 pair-AR band in favor of the dedicated ADSR page.
## Requirements
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

### Requirement: v2-carousel-vertical-budget

The standalone carousel SHALL receive vertical space previously consumed by the bottom global strip. Sequencer panel height SHALL increase by **5u** (`kGlobalStripH` + one `kSectionGap`). Hosted editor layout SHALL follow the same center-cluster and strip-removal rules.

#### Scenario: Sequencer taller after strip removal

- **WHEN** `MainComponent` lays out at default 1280×920
- **THEN** `kSequencerH` is **18u** (180px)
- **THEN** `m_globalStrip` is not given bounds in standalone layout

#### Scenario: Global controls render from top chrome stack

- **WHEN** carousel renders active submodule page
- **THEN** global controls render from the top chrome stack global-command band
- **THEN** any legacy `CenterGlobalClusterV2` transition component is hidden or bounded outside the active module body

### Requirement: Carousel owns module body projection
`PageCarouselComponent` SHALL own the active module body projection and derive its geometry from shared `DesktopV2ChromeLayout` helpers. During migration, legacy row-based pages SHALL split the body into label+encoder, center, and mod column regions from `moduleRowColumns`; compact module pages and parameter-detail pages SHALL use the manifest-backed grid body from the same layout authority.

#### Scenario: Center cluster is not overlayed in legacy column projection
- **WHEN** carousel renders an active legacy row-based submodule page
- **THEN** global controls are rendered from the top chrome stack's global-command band rather than from a center-column overlay
- **THEN** any legacy `CenterGlobalClusterV2` transition component is hidden or bounded outside the active module body

#### Scenario: Compact grid projection uses shared body
- **WHEN** carousel renders a compact module page or parameter-detail page
- **THEN** the page grid bounds come from the shared manifest-era layout helper
- **THEN** the page does not also allocate a hidden legacy center cluster over the grid body

#### Scenario: Hosted carousel uses same projection authority
- **WHEN** hosted editor renders the v2 carousel
- **THEN** carousel column geometry and compact grid geometry come from the same shared layout authority as standalone desktop
