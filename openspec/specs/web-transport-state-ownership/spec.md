# web-transport-state-ownership Specification

## Purpose
TBD - created by archiving change web-sim-layout-ux. Update Purpose after archive.
## Requirements
### Requirement: Main thread owns transport running state

The web sim SHALL treat `audioRunning` on the main thread as the sole authority for Play/Stop UI, mod-bay idle state, and user-visible transport status. WASM `screen` messages SHALL NOT overwrite main-thread `audioRunning`.

#### Scenario: Play stays running across screen ticks

- **WHEN** the user clicks **Play** and audio is routed through the worklet
- **THEN** main-thread `audioRunning` remains true until **Stop** or an explicit error handler
- **AND** incoming `screen` messages do not set main-thread `audioRunning` to false

#### Scenario: Stop ends transport

- **WHEN** the user clicks **Stop**
- **THEN** main thread sets `audioRunning` false and `transportIntentPlaying` false
- **AND** sends `setRunning: false` to the worklet
- **AND** Play is enabled and Stop is disabled

### Requirement: Transport intent tracks user Play/Stop

The sim SHALL maintain `transportIntentPlaying` on the main thread: true after **Play**, false after **Stop** or worklet error. `AudioContext` suspend recovery SHALL consult `transportIntentPlaying`, not WASM screen state.

#### Scenario: Suspend does not clear intent

- **WHEN** `audioContext.state` becomes `suspended` after the user clicked Play (and has not clicked Stop)
- **THEN** `transportIntentPlaying` remains true
- **AND** status indicates audio is suspended and Play can be clicked again

#### Scenario: Stop clears intent

- **WHEN** the user clicks **Stop**
- **THEN** `transportIntentPlaying` becomes false
- **AND** a subsequent `suspended` event does not show a Play-resume hint

### Requirement: AudioContext suspend recovery

When the `AudioContext` enters `suspended` while the user has not clicked Stop, the sim SHALL surface a recoverable status and re-enable **Play**.

#### Scenario: Context suspended during session

- **WHEN** `audioContext.state` becomes `suspended` after the user clicked Play
- **THEN** status indicates audio is suspended and Play can be clicked again
- **AND** Play is not left permanently disabled

### Requirement: Worklet error recovers transport

On worklet `error`, the sim SHALL stop audio, re-enable **Play**, and show a retry status.

#### Scenario: WASM error during Play

- **WHEN** the worklet posts `{ type: "error" }` while transport was active
- **THEN** `stopAudio()` runs, `transportIntentPlaying` becomes false
- **AND** Play becomes enabled
- **AND** Stop becomes disabled
- **AND** status includes a retry hint

