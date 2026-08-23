# froggers-fuegoization Specification

## Purpose
Global Crunchy and per-bank Crispy are applied on Sheaf's `ReplaceCachedKnobValue` parameter-filtering seam (between phase-1 and phase-2), preserving the Froggers Crunchy/Crispy cascade semantics and the Daisy firmware's defined transform rather than the divide-by-zero variant the retired simulator carried in its own fuegoize copy.

## Requirements
### Requirement: Fuegoization is applied on the parameter filtering seam
Global **Crunchy** and per-bank **Crispy** SHALL be applied by overwriting each parameter's cached knob value between the parameter processing phase-1 and phase-2 steps, using the same seam Sheaf exposes for parameter filtering. Fuegoization SHALL NOT be applied inside the DSP modules and SHALL NOT be applied twice.

#### Scenario: Fuego reaches the DSP
- **WHEN** global Crunchy is raised from zero
- **THEN** the value consumed by the DSP for an affected parameter differs from its unfuegoized value
- **THEN** the parameter is warped exactly once per processing block

#### Scenario: Single application point
- **WHEN** the fuego application sites are enumerated
- **THEN** exactly one application point exists, on the phase-1 to phase-2 seam

### Requirement: Froggers Crunchy/Crispy cascade semantics preserved
The transform SHALL be the Froggers fuegoize function: a knob-controlled bit mask, quantization to eight bits with a preserved fractional remainder, XOR-shift scrambling of the masked low bits, and a row-keyed final shift. The cascade SHALL be: the value is first warped by global Crunchy; the bank's Crispy control is itself warped by global Crunchy; the Crunchy-warped Crispy value is then applied to the already-warped parameter value.

#### Scenario: Crispy is scoped to its own bank
- **WHEN** a bank's Crispy control is swept
- **THEN** parameters on that bank change
- **THEN** parameters on other banks do not change

#### Scenario: Crispy control receives only the global stage
- **WHEN** global Crunchy is non-zero
- **THEN** the Crispy control's own displayed and consumed value is warped by Crunchy only
- **THEN** the Crispy control is not fuegoized by itself

#### Scenario: Zero knobs are transparent
- **WHEN** both global Crunchy and the bank's Crispy are at zero
- **THEN** every parameter value passes through unchanged

#### Scenario: Maximum knobs are well-defined
- **WHEN** global Crunchy or a bank's Crispy is at or near its maximum, where the bit mask covers all eight bits
- **THEN** the transform produces a finite, deterministic value
- **THEN** that value does not depend on the optimization level the app was built with

Note: the previous product's transform is undefined behavior in this range — it divides by zero once the mask reaches its full width. This app implements the field firmware's defined formulation instead, which is the parity reference (design D6).

### Requirement: Global Crunchy receives no fuegoization
Ordinary parameters SHALL receive both the global Crunchy stage and their bank's Crispy stage. A bank's Crispy control SHALL receive only the global Crunchy stage. The global Crunchy control itself, being the source of the warp, SHALL receive neither stage.

#### Scenario: Crunchy's own value is never warped
- **WHEN** global Crunchy is at any value
- **THEN** Crunchy's own displayed and consumed value is never warped by either stage

#### Scenario: Raising Crunchy does not alter Crunchy's own value
- **WHEN** global Crunchy is raised from zero
- **THEN** Crunchy's own value changes only by the amount the operator dialed in
- **THEN** no fuegoization is applied on top of that dialed-in value

### Requirement: Fuegoized values are what the knobs display
Because fuego is applied on the cached-knob seam, the encoder ring SHALL display the fuegoized (processed) value, matching how Sheaf displays other post-processed parameter values. No separate UI code path SHALL be required to make fuegoization visible.

#### Scenario: Knob reflects the warp
- **WHEN** global Crunchy is raised while a parameter is untouched
- **THEN** that parameter's encoder ring indicator moves to the fuegoized value

#### Scenario: Only the processed value is shown
- **WHEN** a parameter's ring is rendered
- **THEN** the ring shows the processed value alone
- **THEN** no separate raw-knob ghost indicator is drawn

### Requirement: Fuego row identity is the parameter's zero-based bank slot index
Because the fuegoize scramble pattern is keyed by a row index, each parameter's fuego row identity SHALL be its **zero-based** slot index within its sixteen-slot bank (slots `0..15`). The bank's Crispy control therefore has row identity **14** and global Crunchy **15**. Legacy row identifiers from the previous product SHALL NOT be carried over.

#### Scenario: Row identity follows the slot
- **WHEN** a parameter occupies a given slot in its bank
- **THEN** its fuego row identity is that slot's zero-based index

#### Scenario: The Crispy row identity is fixed
- **WHEN** the cascade warps a parameter on any bank
- **THEN** the Crispy control's own row identity used by the cascade is 14 in every bank

#### Scenario: Character parity with the previous product is not claimed
- **WHEN** the same knob positions are compared against the previous desktop product
- **THEN** the fuegoized results may differ
- **THEN** this is accepted and no parity assertion exists
