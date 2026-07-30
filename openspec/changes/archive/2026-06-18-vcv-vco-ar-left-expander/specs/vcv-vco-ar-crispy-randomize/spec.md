## ADDED Requirements

### Requirement: Dedicated VCO Crispy on left expander

The VCO AR left expander SHALL provide a **Crispy** knob independent of the Audio column row-7 Crispy on the right voicing expander.

That knob SHALL apply fuego/scramble semantics to **Audio page rows 0–2 only** (VCO1, VCO2, VCO3 frequency parameters) on the linked main/right modules.

#### Scenario: VCO Crispy does not fuegoize PM rows

- **WHEN** VCO Crispy on the left expander is at maximum
- **THEN** Audio rows 3–6 (cross-coupler, phase mods) are not fuegoized by this knob

#### Scenario: Independent from Audio column Crispy

- **WHEN** VCO Crispy is at 0 and Audio column Crispy (row 7) is at maximum
- **THEN** rows 0–6 on the Audio page are fuegoized by column Crispy only

### Requirement: Dedicated randomize buttons on left expander

The VCO AR left expander SHALL provide **Randomize** and **Randmod** momentary buttons that affect **only** the six VCO A/R knobs (and their mod depths if mod inputs exist).

#### Scenario: Randomize hits A/R knobs only

- **WHEN** the user presses Randomize on the left expander
- **THEN** the six Att./Rel. values change
- **THEN** voicing-column Randomize on the right expander is not invoked

#### Scenario: Randmod hits A/R mod routing only

- **WHEN** the user presses Randmod on the left expander
- **THEN** mod source/depth for the six A/R params randomize
- **THEN** page-level Randmod on the right expander is not invoked

### Requirement: Randomize uses shared rising-edge dispatch

Left-expander Randomize and Randmod SHALL register in the same momentary action dispatch table pattern as `vcv-randomize-controls` — one rising-edge handler, no copy-paste per button.

#### Scenario: Single fire per press

- **WHEN** Randomize transitions low→high for one sample block
- **THEN** `VcoArState::randomizeKnobs()` is called exactly once
