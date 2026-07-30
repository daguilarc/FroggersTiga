## ADDED Requirements

### Requirement: playwright-rand-waveforms-morph-svg

Playwright tests SHALL verify that Rand waveforms updates VCO morph button SVG on the Audio page after Play and engine readiness.

#### Scenario: Rand waveforms morph SVG after Play

- **WHEN** Playwright starts Play, waits for engine readiness, and clicks Rand waveforms on the Audio page
- **THEN** at least one VCO morph button SVG differs from its pre-click state

### Requirement: playwright-external-meter-after-play

Playwright tests SHALL verify external meter labeling after Play with External Audio enabled.

#### Scenario: External meter active after Play

- **WHEN** Playwright enables External Audio (with mic permission stubbed or granted in test harness), starts Play, and waits for screen updates
- **THEN** `#external-meter-label` does not read `Off` or `Waiting for Play`
- **THEN** `#external-meter-fill` width is greater than 0% or the label indicates active monitoring when test signal is injected

#### Scenario: External meter waiting before Play

- **WHEN** Playwright enables External Audio without starting Play
- **THEN** `#external-meter-label` reads `Waiting for Play`
