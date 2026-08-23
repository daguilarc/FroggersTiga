# external-ring-mod-mix Specification

## Purpose

Normative external-input gate and parallel ring-mod mix formula in the shared `FroggersEngine` across all hosts.

## Requirements

### Requirement: External gate selects VCO-only vs ring mod

`FroggersEngine` SHALL use the existing external-input Schmidt gate (`m_extGate` on smoothed `|extIn|`) to choose the pre-drive mix:

- Gate **closed** (`hasExternal == false`): `OLVL × average(VCO1, VCO2, VCO3)`.
- Gate **open** (`hasExternal == true`): parallel ring mod only (see next requirement).

FUEG/Crispy (Audio page knob 8) SHALL NOT influence the external ring-mod mix.

#### Scenario: Silent external input

- **WHEN** smoothed external level is below the gate close threshold
- **THEN** engine output mix is VCO-only at `OLVL`; external sample is not multiplied into the chain input

#### Scenario: Active external input

- **WHEN** smoothed external level is above the gate open threshold
- **THEN** engine uses parallel ring mod; product ring mod is not computed

### Requirement: Parallel ring mod formula

When `hasExternal` is true, `MixExternalAndOsc` SHALL return:

`(extIn × VCO1 + extIn × VCO2 + extIn × VCO3) / 3`

There SHALL be no product term `extIn × VCO1 × VCO2 × VCO3` and no blend/morph between mix shapes.

The parallel formula SHALL NOT route through `MixOscVoices` (pair-AR VCO sum). Pair-AR dynamics apply to the gate-closed VCO-only path only.

#### Scenario: All hosts share one formula

- **WHEN** any host (Daisy Field, desktop, web WASM) processes a sample with external gate open
- **THEN** the pre-drive mix matches the parallel formula above

#### Scenario: FUEG knob position with external present

- **WHEN** external gate is open and Audio knob 8 is at any position
- **THEN** pre-drive mix is unchanged by knob 8; knob 8 still affects fuegoizer (and PM3 per host rules) only

#### Scenario: Pair-AR active with external gate open

- **WHEN** pair-AR is enabled on the Audio page and external gate is open
- **THEN** pre-drive mix is still `(extIn × VCO1 + extIn × VCO2 + extIn × VCO3) / 3` on raw per-VCO samples, not `extIn × MixOscVoices(VCO1, VCO2, VCO3)`
