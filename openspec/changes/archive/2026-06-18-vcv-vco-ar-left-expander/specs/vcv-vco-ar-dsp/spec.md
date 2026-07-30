## ADDED Requirements

### Requirement: Per-VCO A/R when left expander linked

When the main module detects a linked VCO AR left expander, the engine SHALL apply independent attack–release envelopes to **each** oscillator magnitude `|v1|`, `|v2|`, `|v3|` before the oscillator mix.

- Attack knobs set rise time when `|vN|` increases
- Release knobs set fall time when `|vN|` decreases
- Time mapping SHALL use `PairArEnvelope` with `kMinTimeSec` / `kMaxTimeSec` (1 ms – 10 s exponential via `ExpParam::Compute`)

#### Scenario: VCO1 attack shortens rise

- **WHEN** Att. VCO1 is at minimum and VCO1 level increases abruptly
- **THEN** the VCO1 contribution reaches target faster than when Att. VCO1 is at maximum

#### Scenario: VCO3 release lengthens fall

- **WHEN** Rel. VCO3 is at maximum and VCO3 level decreases
- **THEN** the VCO3 contribution falls more slowly than when Rel. VCO3 is at minimum

### Requirement: Sim pair-AR path unchanged when expander absent

When no VCO AR left expander is linked, desktop/web/sim hosts SHALL continue using `AudioPairArState` pair-sum envelopes (`m_pair12`, `m_pair23`) unchanged.

VCV main module without left expander SHALL NOT enable per-VCO AR.

#### Scenario: Desktop pair-AR unaffected

- **WHEN** the desktop Audio pair-AR band adjusts Att. 1+2
- **THEN** behavior matches pair-sum level follower (not per-VCO)

#### Scenario: VCV without left expander

- **WHEN** main module runs with no left expander linked
- **THEN** per-VCO envelopes are bypassed

### Requirement: VCV per-VCO path replaces pair-AR on linked main

When VCO AR left expander is linked on VCV, the engine SHALL use per-VCO envelopes instead of pair-sum `m_pair12` / `m_pair23` for that process block.

#### Scenario: No double envelope

- **WHEN** left expander is linked and Att. VCO1 is at maximum
- **THEN** pair-12 pair-sum envelope does not additionally shape VCO1+VCO2
