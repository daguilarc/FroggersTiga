## ADDED Requirements

### Requirement: v2-scope-grid-for-ef-sources
Desktop v2 SHALL render a scope grid where indices 7–12 (envelope followers) display color-coded oscilloscope traces consistent across the grid.

#### Scenario: Per-VCO color coding
- **WHEN** VCO1 EF scope is active
- **THEN** its trace uses the VCO1 canonical color from host display constants
- **WHEN** VCO1+VCO2 EF scope is active
- **THEN** its trace blends or dual-tones VCO1 and VCO2 colors per `HostPanelLayout` v2 rules

#### Scenario: Scope updates at UI refresh rate
- **WHEN** audio is running
- **THEN** each EF scope cell reads from a ring buffer fed at audio rate and repaints at the v2 UI timer rate (≥15 Hz)

#### Scenario: Pair and sum scopes show combined signal
- **WHEN** VCO2+VCO3 EF is displayed
- **THEN** the trace represents the envelope follower of `|VCO2 + VCO3|` (or documented sum-then-detect path from design)

### Requirement: v2-random-sh-gated-green-leds
Random S&H sources (indices 13 and 14) SHALL NOT use oscilloscope cells; they SHALL use gated green LEDs with level-proportional brightness per `sim/ModLedBrightness.hpp`.

#### Scenario: Random S&H LED brightness curve
- **WHEN** Random S&H 1 CV is 0.3 with audio running
- **THEN** LED brightness equals `ModLedDisplayBrightness(0.3, true)`

#### Scenario: EF scopes exclude Random S&H
- **WHEN** the scope grid renders
- **THEN** cells for indices 13 and 14 show LED widgets only, not waveform traces
