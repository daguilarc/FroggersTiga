## ADDED Requirements

### Requirement: pair-ar-global-crunchy-on-v2-fuego-hosts

On hosts where `UsesV2Fuego(hostKind)` is true, the engine SHALL apply `V2FuegoStack::ApplyMusicalRow` (global Crunchy then Audio-page Crispy) to each pair-AR parameter's modulated knob value before the pair-AR runtime smoother and envelope step.

Pair-AR parameters SHALL use the Audio page Crispy row (row 7) for the Crispy fuego pass, matching musical rows on that page.

#### Scenario: Web Crunchy affects Attack 1+2 effective value

- **WHEN** `SimHostKind::Web` has global Crunchy at maximum and Attack 1+2 knob at 0.5 with no active mod
- **THEN** `getEffectiveSmoothed(0)` differs from the raw knob 0.5

#### Scenario: Crunchy at zero leaves pair-AR unchanged

- **WHEN** global Crunchy is 0.0 and no mod is active on Release 2+3
- **THEN** effective pair-AR value equals stored knob value

#### Scenario: Web Crispy affects Attack 1+2 effective value

- **WHEN** `SimHostKind::Web` has global Crunchy at 0.0, Audio Crispy at maximum, and Attack 1+2 knob at 0.5 with no active mod
- **THEN** `getEffectiveKnob(0)` differs from the raw knob 0.5

#### Scenario: v1 desktop without global Crunchy unchanged

- **WHEN** `SimHostKind::Desktop` processes pair-AR
- **THEN** pair-AR effective values exclude global Crunchy (v1 has no global Crunchy control)

## MODIFIED Requirements

### Requirement: Pair-sum AR parameters exist on Audio page only

The simulator engine SHALL expose four Audio-only parameters with stable indices 0–3:

| Index | Label |
|-------|-------|
| 0 | Attack 1+2 |
| 1 | Release 1+2 |
| 2 | Attack 2+3 |
| 3 | Release 2+3 |

Labels SHALL be defined in `ParamDisplayNames::forAudioPairAr` and SHALL NOT be duplicated in host UI source strings. Labels SHALL NOT use abbreviated forms such as `Att.` or `Rel.`

#### Scenario: Labels readable on desktop

- **WHEN** the user views the Audio submodule on desktop
- **THEN** the bottom band shows exactly these four labels in order

#### Scenario: Labels readable on web

- **WHEN** the user selects the Audio page in the browser simulator
- **THEN** the pair-AR knob row shows exactly these four labels in order

#### Scenario: Authority matches generated host display

- **WHEN** `scripts/generate-host-display.mjs` runs
- **THEN** `PAIR_AR_KNOB_LABELS` in generated TypeScript matches `ParamDisplayNames::forAudioPairAr`
