# audio-pair-ar-engine Specification

## Purpose
Define the four Audio-only pair-sum attack/release parameters, shared label authority, and envelope behavior consumed by all sim hosts.
## Requirements
### Requirement: Pair-sum AR parameters exist on Audio page only

The simulator engine SHALL expose four Audio-only parameters with stable indices 0–3:

| Index | Label |
|-------|-------|
| 0 | Attack 1+2 |
| 1 | Release 1+2 |
| 2 | Attack 2+3 |
| 3 | Release 2+3 |

Labels SHALL be defined in `ParamDisplayNames::forAudioPairAr` and SHALL NOT be duplicated in host UI source strings.

#### Scenario: Labels readable on desktop

- **WHEN** the user views the Audio submodule on desktop
- **THEN** the bottom band shows exactly these four labels in order

#### Scenario: Labels readable on web

- **WHEN** the user selects the Audio page in the browser simulator
- **THEN** the third row of knobs shows exactly these four labels in order

### Requirement: AR envelope shapes pair-sum contributions

For each audio sample, the engine SHALL apply an attack–release envelope to the VCO1+VCO2 and VCO2+VCO3 pair-sum magnitudes before they contribute to the oscillator mix.

- **Attack** knobs set rise time when the pair-sum target increases
- **Release** knobs set fall time when the pair-sum target decreases (combined decay and release — no separate sustain stage)

#### Scenario: Attack 1+2 shortens rise time

- **WHEN** Attack 1+2 is at minimum and VCO1+VCO2 level increases abruptly
- **THEN** the pair-12 contribution reaches target faster than when Attack 1+2 is at maximum

#### Scenario: Release 2+3 lengthens fall time

- **WHEN** Release 2+3 is at maximum and VCO2+VCO3 level decreases
- **THEN** the pair-23 contribution falls more slowly than when Release 2+3 is at minimum

### Requirement: Mod routing per pair-AR param

Each pair-AR parameter SHALL support mod source assignment and mod depth using the same assignable mod index set as Audio page rows 0–7.

#### Scenario: Mod depth scales effective attack

- **WHEN** mod source 0 (MIDI CC 1) is patched to Attack 1+2 with depth > 0
- **THEN** CC 1 level affects the effective attack time for the VCO1+VCO2 pair envelope

### Requirement: Preset persistence

`SimPresetSnapshot` SHALL store knob value, mod source, and mod depth for all four pair-AR parameters and restore them on load.

#### Scenario: Reload preserves pair-AR settings

- **WHEN** the user sets non-default Attack 2+3 and Release 2+3 values, saves snapshot, and reloads
- **THEN** restored values match the saved state

