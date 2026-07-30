## MODIFIED Requirements

### Requirement: Rotated pair-AR labels use display authority

On host page 0 (Audio) only, each pair-AR column label SHALL render the full string from `ParamDisplayNames::forAudioPairAr(index)` rotated **90 degrees clockwise** beneath the knob.

#### Scenario: Full Attack and Release strings legible

- **WHEN** the desktop Audio panel is shown at default width
- **THEN** the bottom band shows **Attack 1+2**, **Release 1+2**, **Attack 2+3**, and **Release 2+3** fully legible
- **THEN** no label uses abbreviated **Att.** or **Rel.** forms

#### Scenario: Authority string used verbatim

- **WHEN** `ParamDisplayNames::forAudioPairAr(0)` returns **Attack 1+2**
- **THEN** the first pair-AR column displays exactly that string (rotated), not a hardcoded alias
