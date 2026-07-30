## MODIFIED Requirements

### Requirement: v2-seven-module-carousel
Desktop v2 SHALL render all seven module sections (Audio/VCO, Envelope, Filter, Distortion, Random S&H, Reverb, Delay) **at once** on a single unified parameter surface. The prev/next page arrows and the single-active-page model are retired: there is no carousel paging, and no module section is hidden behind navigation. Each section is titled and shows every one of its manifest-visible parameter rows. Module sections carry **no** per-row modulation dropdown column (modulation is edited only in the parameter-detail grid, per `desktop-v2-mod-source-grid`).

#### Scenario: All module sections visible without paging
- **WHEN** desktop v2 renders at 1280×920
- **THEN** every module section (Audio/VCO, Envelope, Filter, Distortion, Random S&H, Reverb, Delay) is visible simultaneously
- **THEN** there are no prev/next carousel arrows and no single active-page selection
- **THEN** each section shows all of its manifest-visible parameter rows

#### Scenario: Per-module Randomize on each section
- **WHEN** the operator uses a module section's header Randomize control
- **THEN** the control dispatches the control-core `RandPage` message for that section's page index
- **THEN** only that module's parameters randomize, through the existing randomization authority (no parallel randomize path)

### Requirement: v2-carousel-vertical-budget
At 1280×920 the unified parameter surface SHALL fit every module section's parameters without a vertical scrollbar. The transport and global-command chrome relocate beside the oscilloscope so the vertical space formerly used by the carousel header and arrows is available to the parameter surface.

#### Scenario: Unified surface fits without scroll
- **WHEN** desktop v2 renders the unified parameter surface at 1280×920
- **THEN** all module-section parameter rows are visible without vertical scrolling
- **THEN** the transport and global-command controls render to the right of the oscilloscope with no dead horizontal margin
