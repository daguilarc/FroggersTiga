## ADDED Requirements

### Requirement: Sim hosts expose dedicated PM3 knob on Audio row 6

When `SetSimDedicatedPm3Knob(true)` is active, Audio page parameter index 6 SHALL control PM3 phase-mod depth (VCO2 → VCO3 when cross-coupler 2→3 coupling is active) using the same `ZeroedExp` curve as PM1 and PM2. Crunch (parameter index 7) SHALL NOT contribute to PM3 depth on sim hosts.

#### Scenario: Row 6 drives PM3 with coupling

- **WHEN** sim host is playing on the Audio page, cross-coupler is turned toward 2→3, and the user raises knob 7 (row index 6)
- **THEN** VCO3 phase modulation from VCO2 increases audibly
- **AND** PM3 depth does not depend on the Crunch knob position

#### Scenario: Crunch at minimum does not gate PM3

- **WHEN** Crunch is at minimum and row 6 is above minimum with 2→3 coupling active
- **THEN** PM3 depth follows row 6 only
- **AND** Crunch still controls fuegoizer amount for knobs 1–7

#### Scenario: Firmware path unchanged

- **WHEN** `SetSimDedicatedPm3Knob` was never called (Field firmware build)
- **THEN** row 6 still drives OLVL and PM3 depth still follows stored Audio-page FUEG value
- **AND** behavior matches `MANUAL.md` Audio page table

### Requirement: Sim hosts use fixed OLVL default

When `SetSimDedicatedPm3Knob(true)` is active, oscillator-only mix level (`OLVL`) SHALL use the firmware init default (0.4 mapped through existing `ExpMap(0.01f, 1.0f, …)`) regardless of the stored value at Audio parameter index 6.

#### Scenario: External off default path

- **WHEN** sim host plays with External off and no external gate
- **THEN** output uses fixed OLVL × oscillator mix
- **AND** row 6 knob does not trim overall VCO level

#### Scenario: External on mix topology unchanged

- **WHEN** external gate is high and External is on
- **THEN** mix topology blend still follows Crunch (`FUEG`) with product ↔ parallel ring mod
- **AND** row 6 does not affect mix topology

### Requirement: Sim host Init enables dedicated PM3 knob

Desktop and WASM sim host adapters SHALL call `SetSimDedicatedPm3Knob(true)` during engine `Init()`. The firmware shim SHALL NOT call it.

#### Scenario: Desktop sim Init

- **WHEN** `DesktopHostIO` initializes the engine for JUCE sim
- **THEN** `SetSimDedicatedPm3Knob(true)` was invoked before audio processing

#### Scenario: Web WASM Init

- **WHEN** `PagedHostIO` initializes the engine for WASM
- **THEN** `SetSimDedicatedPm3Knob(true)` was invoked before audio processing
