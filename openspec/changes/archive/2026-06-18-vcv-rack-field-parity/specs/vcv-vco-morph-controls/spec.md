## ADDED Requirements

### Requirement: VCO morph buttons on Audio column

The VCV main module SHALL expose clickable wave-morph controls on Audio page rows 0–2 (VCO1, VCO2, VCO3), matching desktop `SubModulePanel` and web `vco-morph-btn` placement beside each knob/mod-jack cluster.

#### Scenario: Morph display matches engine state

- **WHEN** the Audio column is visible at 100% rack zoom
- **THEN** three wave-morph widgets appear on rows 0–2 only
- **THEN** each widget draws a wave stroke reflecting `GetVcoMorph(index)`

#### Scenario: Click cycles morph

- **WHEN** the user clicks VCO2 wave-morph widget
- **THEN** `CycleVcoMorph(1)` runs
- **THEN** the drawn wave updates to the new morph value

#### Scenario: No morph buttons on non-Audio columns

- **WHEN** Random S&H, Filter, or Drive columns are inspected
- **THEN** no wave-morph widgets appear on those columns

#### Scenario: Morph icon is static wave preview

- **WHEN** a wave-morph widget is displayed
- **THEN** it draws a one-cycle morph shape from `VcoWaveEval` (same as web `waveSvg` and desktop `WaveMorphButton`)
- **THEN** it is not a time-series CV or audio trace (not a mod-rack oscilloscope)

#### Scenario: Rand waveforms global action

- **WHEN** the user triggers **Rand waveforms** on the global strip
- **THEN** all three VCO morph widgets update to new random morph values
- **THEN** behavior matches desktop `GlobalStrip` Rand waveforms
