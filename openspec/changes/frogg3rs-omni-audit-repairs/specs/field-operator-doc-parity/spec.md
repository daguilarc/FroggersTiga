# Delta — `field-operator-doc-parity`

## MODIFIED Requirements

### Requirement: Field manual external mix documented plainly

`DAISY_MANUAL.md` SHALL describe external-input mix without product ring mod or FUEG/Crispy mix-topology language:

- Signal-flow ascii: parallel ring mod when external audio is present above the gate; VCO-only at `OLVL` when silent.
- No mix-topology table or FUEG morph between product and parallel.
- Audio page: external gate open → `(ext × VCO1 + ext × VCO2 + ext × VCO3) / 3`; FUEG does not shape external mix.
- Audio page exception: FUEG is fuegoizer plus PM3 depth only (no mix-topology bullet).

#### Scenario: Field signal flow

- **WHEN** reader opens `DAISY_MANUAL.md` signal-flow section
- **THEN** text does not mention FUEG continuum, product ring mod, or mix topology morph

#### Scenario: Field Audio FUEG row

- **WHEN** reader opens Audio page knob table row 8
- **THEN** FUEG is fuegoizer and PM3 depth only
