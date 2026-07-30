## ADDED Requirements

### Requirement: Playwright runs on web changes in CI

The repository SHALL include a CI workflow that builds WASM, builds the web sim preview, and runs `npm run test:e2e` when files under `web/`, `wasm/`, or `sim/` change.

#### Scenario: Web PR triggers e2e

- **WHEN** a pull request modifies `web/src/main.ts`
- **THEN** CI builds `froggers.wasm`, starts the preview server, and executes Playwright tests

#### Scenario: WASM change triggers e2e

- **WHEN** a pull request modifies `wasm/` or `sim/` sources that affect the engine
- **THEN** CI rebuilds WASM and runs Playwright tests

#### Scenario: Non-web changes skip e2e

- **WHEN** a pull request modifies only `desktop/` files
- **THEN** the web Playwright workflow does not run

### Requirement: Playwright covers mobile audio session lifecycle

Playwright tests SHALL verify mobile-emulated `navigator.audioSession` sequences for Play (`playback`), External enable (`auto` then `play-and-record`), and External disable / Stop (`playback` then `auto`), using a test spy injected before page load.

#### Scenario: Mobile playback session

- **WHEN** Playwright uses mobile viewport and user agent and starts Play with External off
- **THEN** the audio session log contains `playback`

#### Scenario: Desktop no-op

- **WHEN** Playwright uses desktop viewport and user agent and starts Play
- **THEN** the audio session log remains empty

### Requirement: Playwright covers routing UX copy

Playwright tests SHALL assert the subtitle distinguishes Play from External and the status line includes the mobile earpiece hint when External is on and audio is playing under mobile emulation.

#### Scenario: Subtitle copy

- **WHEN** the sim page loads
- **THEN** the subtitle states Play produces sound without External

#### Scenario: Mobile hint visible

- **WHEN** mobile emulation is active, audio is playing, and External is on
- **THEN** the status line mentions earpiece / headphones guidance

### Requirement: Playwright validates manual documentation sync

Playwright tests SHALL assert `web/public/sim-manual.md` documents mobile External routing including earpiece behavior.

#### Scenario: Manual mobile section

- **WHEN** the manual doc test runs
- **THEN** `sim-manual.md` contains a Mobile browsers section referencing earpiece routing

### Requirement: Shared sim UI selectors

Button labels, copy fragments, and key element selectors used by Playwright SHALL live in a single shared module consumed by Appium specs.

#### Scenario: Selector authority

- **WHEN** Appium or Playwright references the External Audio button
- **THEN** both import the same label constant from the shared module

#### Scenario: Hint fragment authority

- **WHEN** Playwright asserts the mobile earpiece hint in `#status`
- **THEN** it uses shared hint fragment constants matching `main.ts` copy
