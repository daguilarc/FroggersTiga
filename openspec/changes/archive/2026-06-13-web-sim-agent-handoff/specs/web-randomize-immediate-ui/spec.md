## ADDED Requirements

### Requirement: Global randomize actions post immediate screen updates

After `marbles`, `randomizeAll`, or `randomizeMod` messages are handled in the worklet, the processor SHALL call `postScreen()` before the next audio block completes, matching the existing behavior of `randomizePage` and `randomizePageMod`.

#### Scenario: Rand All while stopped

- **WHEN** the user clicks Rand All before or after Play
- **THEN** knob columns update to new values on the next `screen` message without waiting for audio frame cadence

#### Scenario: Marbles randomize

- **WHEN** the user triggers Marbles randomize
- **THEN** morph and row values visible in the UI update immediately via `screen`

#### Scenario: Rand Mod

- **WHEN** the user clicks Rand Mod
- **THEN** mod depths and effective row values refresh in knob columns immediately

### Requirement: Engine start posts initial screen

When the worklet receives `setRunning` with `running: true`, it SHALL call `postScreen()` once so scopes, knobs, and mod bay reflect engine state before the first periodic frame-20 tick.

#### Scenario: Play from stopped

- **WHEN** main sends `setRunning: true`
- **THEN** a `screen` message arrives before or with the first audible blocks
- **AND** mod bay scope canvases receive sample data when audio is running
