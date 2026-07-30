## ADDED Requirements

### Requirement: Pair-AR labels rotated on desktop

On host page 0 (Audio) only, each pair-AR column label SHALL render the full string from `ParamDisplayNames::forAudioPairAr(index)` rotated **90 degrees clockwise** beneath the knob.

The label SHALL NOT truncate with ellipsis or horizontal clip under default panel width (300 px module width).

Label zone height and band layout SHALL be defined only in `AudioPairArLayout.hpp` (no duplicate magic numbers in `SubModulePanel.cpp`).

#### Scenario: All four labels readable

- **WHEN** the user views the Audio submodule on desktop at default panel size
- **THEN** the bottom band shows **Attack 1+2**, **Release 1+2**, **Attack 2+3**, and **Release 2+3** fully legible

#### Scenario: Label authority unchanged

- **WHEN** `ParamDisplayNames::forAudioPairAr(0)` returns **Attack 1+2**
- **THEN** the first pair-AR column displays exactly that string (rotated), not a hardcoded alias

#### Scenario: Non-Audio panels unchanged

- **WHEN** the user views Filter or any non-Audio submodule
- **THEN** no rotated pair-AR labels appear
