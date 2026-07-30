## MODIFIED Requirements

### Requirement: v2-carousel-vertical-budget

The standalone carousel SHALL receive vertical space previously consumed by the bottom global strip. Sequencer panel height SHALL remain **18u**. Carousel body SHALL use available height for module rows before enabling scroll.

#### Scenario: Center cluster column sibling with exclusive bounds

- **WHEN** carousel renders active submodule page
- **THEN** `CenterGlobalClusterV2` occupies the center column rectangle from `moduleRowColumns`
- **THEN** center cluster is not `setBounds` over the full submodule panel area at a fixed X offset

#### Scenario: Sequencer taller after strip removal

- **WHEN** `MainComponent` lays out at default 1280×920
- **THEN** `kSequencerH` is **18u** (180px)
- **THEN** `m_globalStrip` is not given bounds in standalone layout
