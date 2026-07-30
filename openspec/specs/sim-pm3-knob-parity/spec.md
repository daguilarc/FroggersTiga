# sim-pm3-knob-parity Specification

## Purpose
Keep sim host knob labels and control ranges aligned with PM3 hardware field parity across desktop, web, and VCV surfaces.
## Requirements
### Requirement: Sim Audio row 7 is Phase mod 3

On sim hosts (desktop standalone, web WASM, VCV `PagedHostIO`), Audio page row 7 (parameter index 6) SHALL display the label **Phase mod 3** from `ParamDisplayNames::forHostPageRow(0, 6)` and SHALL control dedicated PM3 depth (VCO2 → VCO3 phase modulation when cross-coupler is toward 2→3).

#### Scenario: Desktop panel label

- **WHEN** the desktop Audio submodule panel renders row 7
- **THEN** the label text is **Phase mod 3**, not OLVL, VCO level, or VCO Envelope

#### Scenario: Web panel label

- **WHEN** the web sim renders Audio page row 7
- **THEN** the label text is **Phase mod 3**, matching `ParamDisplayNames`

#### Scenario: PM3 DSP routing on sim

- **WHEN** sim host initializes with `SetSimDedicatedPm3Knob(true)` and user raises Audio row 7 with cross-coupler CW (2→3)
- **THEN** audible VCO3 phase modulation increases; PM3 depth is not sourced from Crispy (row 8 / FUEG)

#### Scenario: Osc level fixed on sim

- **WHEN** sim host runs with dedicated PM3 knob enabled
- **THEN** internal oscillator level (`m_oscLvl`) uses the sim fixed target (0.4) and is not controlled by row 7

### Requirement: Crispy does not control PM3 on sim

On sim hosts, Audio row 8 (Crispy / FUEG) SHALL be the fuegoizer for knobs 1–7 on fuego-enabled pages; it SHALL NOT be the primary PM3 depth control when `SetSimDedicatedPm3Knob(true)` and SHALL NOT control external ring-mod mix.

#### Scenario: Crispy vs PM3 independence

- **WHEN** user sets Crispy to minimum and Phase mod 3 to maximum with 2→3 coupling active
- **THEN** PM3 effect is present while fuegoizer/scramble effect on knobs 1–7 stays at minimum

#### Scenario: Crispy vs external mix independence

- **WHEN** user enables external input and sweeps Crispy while gate is open
- **THEN** pre-drive ring-mod timbre does not change with Crispy; only fuegoizer-scrambled parameter values change

### Requirement: VCO Envelope is mod source only

Mod source index 4 SHALL remain labeled **VCO Envelope** (via `ParamDisplayNames::forModSource(4)`) and SHALL NOT be used as the Audio page row 7 knob label.

#### Scenario: Mod rack vs submodule label separation

- **WHEN** user views the mod rack and Audio submodule simultaneously
- **THEN** mod rack shows **VCO Envelope** for mod index 4 and Audio row 7 shows **Phase mod 3** as distinct controls

