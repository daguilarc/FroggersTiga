## ADDED Requirements

### Requirement: Pair-AR time constants match VCV Fundamental ADSR display range

The pair-sum A/R envelope SHALL map knob 0.0–1.0 to a time constant τ (seconds) using exponential spacing from **1 ms** (knob 0) to **10 s** (knob 1), matching VCV Fundamental ADSR Attack/Decay/Release parameter display endpoints (`MIN_TIME = 1e-3 s`, `MAX_TIME = 10 s`, ratio 10 000).

The min and max seconds SHALL be named constants on `PairArEnvelope` (`kMinTimeSec`, `kMaxTimeSec`). The mapping SHALL use `PhaseUtils::ExpParam::Compute(kMinTimeSec, kMaxTimeSec, knob)` — not inline magic numbers or a duplicate pow formula.

#### Scenario: Minimum knob time constant

- **WHEN** `PhaseUtils::ExpParam::Compute(PairArEnvelope::kMinTimeSec, PairArEnvelope::kMaxTimeSec, 0.0)` is evaluated
- **THEN** the result equals `1e-3` seconds within floating-point tolerance

#### Scenario: Maximum knob time constant

- **WHEN** `PhaseUtils::ExpParam::Compute(PairArEnvelope::kMinTimeSec, PairArEnvelope::kMaxTimeSec, 1.0)` is evaluated
- **THEN** the result equals `10.0` seconds within floating-point tolerance

#### Scenario: Center knob geometric midpoint

- **WHEN** `PhaseUtils::ExpParam::Compute(PairArEnvelope::kMinTimeSec, PairArEnvelope::kMaxTimeSec, 0.5)` is evaluated
- **THEN** the result equals `0.1` seconds within floating-point tolerance

### Requirement: Level-follower semantics unchanged

Changing the time range SHALL NOT alter pair-AR triggering: attack when the pair-sum target increases, release when it decreases. The engine SHALL NOT introduce a gate input for pair-AR in this change.

#### Scenario: Follower still tracks pair magnitude

- **WHEN** VCO1+VCO2 sum magnitude increases while Attack 1+2 is at maximum
- **THEN** the pair-12 contribution rises more slowly than when Attack 1+2 is at minimum

#### Scenario: Release still tracks falling pair level

- **WHEN** VCO2+VCO3 sum magnitude decreases while Release 2+3 is at maximum
- **THEN** the pair-23 contribution falls more slowly than when Release 2+3 is at minimum

### Requirement: Manual documents time range and follower behavior

The simulator manual SHALL state that pair-AR Attack/Release knobs span **1 ms – 10 s** (exponential), that the envelope follows pair-sum level (not a gate), and that the knob mapping matches VCV ADSR Attack/Release **time range** — not gate-triggered ADSR behavior.

#### Scenario: Manual pair-AR table

- **WHEN** a reader opens `SIM_MANUAL.md` Audio pair-AR section
- **THEN** the documented time range is 1 ms – 10 s, follower semantics are described, and Delay time (~0–2 s) is not conflated with pair-AR
