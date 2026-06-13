## ADDED Requirements

### Requirement: process() errors recover transport on main thread

If an uncaught exception occurs inside the AudioWorklet `process()` method, the processor SHALL post `{ type: "error", message: string }` to the main thread, output silence for that frame, and avoid repeated crash loops on subsequent frames.

#### Scenario: postScreen failure during Play

- **WHEN** `process()` throws while transport is active
- **THEN** the worklet posts an `error` message
- **AND** main thread runs `stopAudio()`
- **AND** Play is enabled and Stop is disabled
- **AND** status includes a retry hint

#### Scenario: Constructor errors unchanged

- **WHEN** WASM fails to instantiate in the worklet constructor
- **THEN** existing constructor catch still posts `error` before any `process()` call

## MODIFIED Requirements

### Requirement: Worklet error recovers transport

On worklet `error`, the sim SHALL stop audio, re-enable **Play**, and show a retry status. This SHALL apply to errors posted from `process()` runtime failures as well as constructor and WASM load failures.

#### Scenario: WASM error during Play

- **WHEN** the worklet posts `{ type: "error" }` while transport was active
- **THEN** `stopAudio()` runs, `transportIntentPlaying` becomes false
- **AND** Play becomes enabled
- **AND** Stop becomes disabled
- **AND** status includes a retry hint

#### Scenario: Silent worklet death prevented

- **WHEN** a fault would previously kill the worklet without posting a message
- **THEN** the processor catch posts `error` instead
- **AND** main thread transport state matches Stopped, not stuck Playing
