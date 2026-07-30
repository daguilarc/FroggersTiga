## MODIFIED Requirements

### Requirement: Crispy does not control PM3 on sim

On sim hosts, Audio row 8 (Crispy / FUEG) SHALL be the fuegoizer for knobs 1–7 on fuego-enabled pages; it SHALL NOT be the primary PM3 depth control when `SetSimDedicatedPm3Knob(true)` and SHALL NOT control external ring-mod mix.

#### Scenario: Crispy vs PM3 independence

- **WHEN** user sets Crispy to minimum and Phase mod 3 to maximum with 2→3 coupling active
- **THEN** PM3 effect is present while fuegoizer/scramble effect on knobs 1–7 stays at minimum

#### Scenario: Crispy vs external mix independence

- **WHEN** user enables external input and sweeps Crispy while gate is open
- **THEN** pre-drive ring-mod timbre does not change with Crispy; only fuegoizer-scrambled parameter values change
