# Delta — `mod-blend-semantics`

## MODIFIED Requirements

### Requirement: Modulation applied before fuegoization

For fuego-enabled parameters (page rows 1–7, Delay rows 1–7), the pipeline SHALL be:

1. Mod crossfade on stored base
2. Fuego bit-scramble using effective Crispy amount (mod crossfade on Crispy when assigned)

Fuego SHALL NOT apply to Crispy itself.

#### Scenario: Mod then fuego on page row

- **WHEN** Audio row 1 has LFO mod at depth 1.0 and Crispy > 0
- **THEN** DSP receives fuego-scrambled LFO value, not fuego-scrambled base then modulated

#### Scenario: Modulated Crispy drives scramble intensity

- **WHEN** Crispy has VCO Envelope mod at depth 1.0 on Audio page
- **THEN** fuego mask on rows 1–7 follows the modulated Crispy level
