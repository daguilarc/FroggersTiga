# web-mobile-global-strip-placement Specification

## Purpose
Place the global randomize strip directly under External MIDI and above the mod bay on all viewports without changing button ids.
## Requirements
### Requirement: Global randomize strip under External MIDI

On all viewports, the global randomize strip (Rand All, Rand Mods, Rand Resample, Rand waveforms) SHALL appear directly below the External MIDI button and above the mod bay and knob grid.

The strip SHALL remain a single DOM element with unchanged button `id` attributes (`rand-all`, `rand-mod`, `marbles-btn`, `rand-morphs`).

#### Scenario: Strip precedes mod bay

- **WHEN** the sim page is loaded at any viewport width
- **THEN** `.global-strip` bounding box top is less than `#mod-bay` bounding box top

#### Scenario: Strip follows External MIDI

- **WHEN** the sim page is loaded at any viewport width
- **THEN** `.global-strip` bounding box top is greater than `#external-midi-btn` bounding box bottom

### Requirement: No duplicate global strip controls

The host SHALL expose exactly one `.global-strip` with four global randomize buttons. Mobile and desktop SHALL NOT use separate duplicate button sets.

#### Scenario: Single strip in DOM

- **WHEN** the sim HTML is parsed
- **THEN** exactly one element matches `.global-strip`

### Requirement: Playwright layout regression

The web Playwright suite SHALL include mobile-emulated and desktop-emulated tests asserting global strip layout order relative to External MIDI and mod bay without starting audio.

#### Scenario: CI mobile global strip test

- **WHEN** `npm run test:e2e` runs in CI with mobile emulation
- **THEN** the global strip placement spec passes

#### Scenario: CI desktop global strip test

- **WHEN** `npm run test:e2e` runs in CI with desktop emulation
- **THEN** the global strip placement spec passes

