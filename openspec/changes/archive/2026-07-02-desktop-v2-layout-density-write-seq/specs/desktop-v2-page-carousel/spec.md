## MODIFIED Requirements

### Requirement: v2-carousel-vertical-budget

The standalone carousel SHALL receive vertical space previously consumed by the bottom global strip. Sequencer panel height SHALL increase by **5u** (`kGlobalStripH` + one `kSectionGap`). Hosted editor layout SHALL follow the same center-cluster and strip-removal rules.

#### Scenario: Sequencer taller after strip removal

- **WHEN** `MainComponent` lays out at default 1280×920
- **THEN** `kSequencerH` is **18u** (180px)
- **THEN** `m_globalStrip` is not given bounds in standalone layout

#### Scenario: Center cluster sibling to submodule viewport

- **WHEN** carousel renders active submodule page
- **THEN** `CenterGlobalClusterV2` is visible beside the encoder viewport within the carousel content area
- **THEN** center cluster height spans the submodule encoder viewport height
