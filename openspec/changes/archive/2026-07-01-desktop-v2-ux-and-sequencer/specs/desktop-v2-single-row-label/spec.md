## ADDED Requirements

### Requirement: one-label-per-encoder-row

Each module encoder row SHALL display exactly one parameter name from `V2ParamDisplayNames::forHostPageRow`. The name SHALL NOT be duplicated on the encoder ring component. This applies to `SubmodulePagePanel` and `AdsrPagePanel` (Pair-AR page).

#### Scenario: No duplicate VCO1 label

- **WHEN** Audio page row 0 renders
- **THEN** exactly one visible text label reads the authority string for row 0
- **THEN** the encoder ring draws arcs and badges only — no parameter name text in the ring paint path

#### Scenario: No duplicate Pair-AR row label

- **WHEN** Pair-AR page row 0 (Atk1) renders
- **THEN** exactly one visible text label reads the authority string for row 0
- **THEN** `AdsrPagePanel` does not call `EncoderRingComponent::setLabel` with the same string as the left column
