# desktop-v2-center-global-cluster Specification

## Purpose
**Superseded by `froggers-v2-sheaf-runtime-convergence`.** Global randomize, Crunchy, Shift, waveform-randomize, Marbles/Rand Resample, and scene/step scope controls moved to the top chrome stack global-command band. Do not implement center-column `CenterGlobalClusterV2` as the authority for global controls.

## Requirements
### Requirement: v2-center-global-cluster-column (RETIRED)
Standalone desktop v2 previously placed global randomize actions, **Crunchy**, and **Shift** in a center column within the module/carousel area. That layout SHALL NOT be reintroduced: standalone desktop v2 MUST own global randomize, Crunchy, Shift, waveform-randomize, Marbles/Rand Resample, and scene/step scope controls through the top chrome stack global-command band, not a center-column `CenterGlobalClusterV2` authority.

#### Scenario: Center cluster replaced by top global-command band (RETIRED)
- **WHEN** standalone desktop v2 renders any module page at 1280×920
- **THEN** Rand All, Rand Mods, Rand waveforms, Rand Resample, Crunchy, Shift, and scene/step scope pairs appear in the top chrome stack global-command band
- **THEN** no center-column overlay receives pointer events over module parameter cells, parameter-detail cells, sequencer controls, or mod-column cells

#### Scenario: Center cluster host callbacks preserved through top band (RETIRED)
- **WHEN** the operator invokes global controls from the global-command band
- **THEN** the same control-core and `DesktopHostIO` mutations fire as the prior center-cluster projection
