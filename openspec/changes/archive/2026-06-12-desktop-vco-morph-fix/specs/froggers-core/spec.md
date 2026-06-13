## MODIFIED Requirements

### Requirement: Sim-only VCO waveform morph DSP

For sim hosts, `FroggersEngine` SHALL provide three `VcoWaveMorph` targets (VCO1–3) with knob values in 0..1. `VcoWaveMorph::GetMorph` SHALL return the modulated knob value clamped to **0..1** (linear). It SHALL **NOT** use `PhaseUtils::ExpParam::Compute` with `min=0` — that mapping is invalid and produces NaN for knob > 0. `EvalWaveMorph(phase, morph)` SHALL smoothly blend sine → saw → square across morph ∈ [0,1]. Sim `StepOscillators` SHALL use `EvalWaveMorph` for all three VCOs when sim morph mode is active.

**Supersedes:** `sim-hosts-multi-ui` requirement “exponential knob mapping (`PhaseUtils::ExpParam` or equivalent)” for morph read.

#### Scenario: Morph at endpoints

- **WHEN** VCO1 morph knob is 0.0
- **THEN** `ModulatedMorph(0)` is 0.0 and output matches sine; at knob 1.0 morph is 1.0 and output matches square; at knob 0.5 morph is 0.5 and output matches saw within float tolerance

#### Scenario: Randomize all morphs

- **WHEN** a sim host calls `RandomizeVcoMorphs()`
- **THEN** each VCO morph receives an independent uniform random value in [0,1] via `RGen`
- **AND** `GetMorph` returns that value (not stuck at 0 from NaN)
