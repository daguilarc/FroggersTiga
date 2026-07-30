## ADDED Requirements

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
