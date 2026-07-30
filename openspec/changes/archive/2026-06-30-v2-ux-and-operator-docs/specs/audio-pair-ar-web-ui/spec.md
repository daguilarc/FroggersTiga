## MODIFIED Requirements

### Requirement: Third row pair-AR controls on Audio page

The web Audio knob grid SHALL place pair-AR controls in columns starting at index `EXPANDED_CORE_KNOB_COUNT` (10), after the eight core Audio knobs, without column collision.

Cells 0–7 SHALL remain the existing Audio parameters (VCO1 through Crispy). Pair-AR cells SHALL use indices 10–13 in order: Attack 1+2, Release 1+2, Attack 2+3, Release 2+3.

Each pair-AR cell SHALL use the same column layout as existing cells: label, knob, mod-source select.

#### Scenario: Pair-AR labels from authority

- **WHEN** the web Audio page renders pair-AR knobs
- **THEN** labels match `ParamDisplayNames::forAudioPairAr` via generated `hostDisplay` constants
- **THEN** labels read Attack 1+2, Release 1+2, Attack 2+3, Release 2+3 without abbreviation

#### Scenario: Pair-AR columns do not overlap expansion rows

- **WHEN** the web Audio page layout is computed
- **THEN** pair-AR column indices are >= 10
- **THEN** no pair-AR knob shares a column index with core rows 0–7
