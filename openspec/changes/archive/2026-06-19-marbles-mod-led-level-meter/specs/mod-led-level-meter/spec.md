# mod-led-level-meter Specification

## Purpose

Cross-host proportional brightness for Random mod rack LEDs (indices 5 and 6) on desktop standalone, web/WASM, VST/AU, and VCV Rack, driven by one shared curve in `sim/`.

## ADDED Requirements

### Requirement: Shared mod LED brightness curve

The repository SHALL provide `sim/ModLedBrightness.hpp` as the single authority for Random mod LED display brightness. The function `ModLedDisplayBrightness(float cv01, bool active)` SHALL:

- Return `0` when `active` is false.
- Clamp `cv01` to `[0, 1]`.
- Return `1` when `cv01 >= kModLedFullBrightnessCv` (initial value **0.55**).
- Return `normalized * normalized`, where `normalized = cv01 / kModLedFullBrightnessCv`, for `0 < cv01 < kModLedFullBrightnessCv`.

Host UI code SHALL NOT duplicate threshold or curve literals for indices 5 and 6.

#### Scenario: Zero CV is dark

- **WHEN** `ModLedDisplayBrightness(0, true)` is evaluated
- **THEN** the result is `0`

#### Scenario: Reference CV is full brightness

- **WHEN** `ModLedDisplayBrightness(0.55, true)` is evaluated
- **THEN** the result is `1`

#### Scenario: Inactive transport is dark

- **WHEN** `ModLedDisplayBrightness(0.8, false)` is evaluated
- **THEN** the result is `0`

#### Scenario: Mid CV is partial brightness

- **WHEN** `ModLedDisplayBrightness(0.275, true)` is evaluated with exponent 2.0
- **THEN** the result is `0.25`

#### Scenario: Out-of-range CV is clamped

- **WHEN** the function is evaluated with active CV below `0` or above `1`
- **THEN** the results are `0` and `1` respectively

### Requirement: Desktop and VST Random LEDs use shared curve

Desktop standalone and VST/AU plugin editors SHALL render Random 1 and Random 2 mod cells (indices 5 and 6) with green brightness proportional to `ModLedDisplayBrightness(GetCvOut(index), audioRunning)`.

#### Scenario: Desktop dim held CV

- **WHEN** audio is running and `GetCvOut(5)` is `0.2`
- **THEN** the Random 1 LED renders visibly dimmer than at `GetCvOut(5) == 0.55`

#### Scenario: Desktop idle is off

- **WHEN** audio is not running
- **THEN** Random LEDs render at zero brightness regardless of stored CV

### Requirement: Web Random LEDs use generated curve parity

Web `ModLedIndicator` SHALL import `modLedDisplayBrightness` from the existing `web/src/hostDisplay.generated.ts` produced by `scripts/generate-host-display.mjs`. The LED element SHALL initialize and expose `data-brightness` in `[0, 1]` and a numeric `--mod-led-brightness` CSS custom property with the same value on each render tick. CSS SHALL render a dark base plus a green overlay driven by that property, not by a binary selector.

#### Scenario: Web generator check

- **WHEN** `node scripts/generate-host-display.mjs --check` runs
- **THEN** `hostDisplay.generated.ts`, including the mod LED constant/function, matches the C++ display authorities

#### Scenario: Web DOM reflects level

- **WHEN** audio is running and mod level 5 is `0.3`
- **THEN** the Random 1 `.mod-led` element has `data-brightness` equal to `modLedDisplayBrightness(0.3, true)` within floating-point tolerance
- **THEN** its `--mod-led-brightness` property equals the exposed `data-brightness`

### Requirement: VCV Random LEDs use shared curve

VCV primary module `LIGHT_MOD_RANDOM1` and `LIGHT_MOD_RANDOM2` SHALL call `ModLedDisplayBrightness` with `host.GetCvOut(5)` and `host.GetCvOut(6)` respectively, passing `active = true` during normal `process()`.

#### Scenario: VCV partial brightness

- **WHEN** `GetCvOut(6)` is `0.4` during process
- **THEN** `LIGHT_MOD_RANDOM2` brightness is `ModLedDisplayBrightness(0.4, true)` not a binary 0/1

### Requirement: Automated verification

The sim test suite SHALL include `ModLedBrightness_test.cpp` with clamp, inactive, and golden points for the shared curve. Web CI SHALL include Playwright spec `web/e2e/marbles-mod-led-level.spec.ts` covering initialized idle darkness, live numeric brightness, removal of the binary DOM contract, CSS-property parity, and imported curve points.

#### Scenario: Sim golden test passes

- **WHEN** `ModLedBrightness_test` runs in the sim test target
- **THEN** all golden `(cv, active) → brightness` pairs pass

#### Scenario: Playwright runs in CI

- **WHEN** the Web E2E workflow executes on changes under `web/`, `wasm/`, or `sim/`
- **THEN** `marbles-mod-led-level.spec.ts` is included in `npm run test:e2e`
