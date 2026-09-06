# Delta — `external-ring-mod-mix`

## MODIFIED Requirements

### Requirement: Parallel ring mod formula

When `hasExternal` is true, `MixExternalAndOsc` SHALL return:

`(extIn × VCO1 + extIn × VCO2 + extIn × VCO3) / 3`

There SHALL be no product term `extIn × VCO1 × VCO2 × VCO3` and no blend/morph between mix shapes.

#### Scenario: All hosts share one formula

- **WHEN** any host (Daisy Field, desktop, web WASM) processes a sample with external gate open
- **THEN** the pre-drive mix matches the parallel formula above

#### Scenario: FUEG knob position with external present

- **WHEN** external gate is open and Audio knob 8 is at any position
- **THEN** pre-drive mix is unchanged by knob 8; knob 8 still affects fuegoizer (and PM3 per host rules) only
