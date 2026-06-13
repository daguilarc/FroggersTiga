## Why

The FroggersTiga web sim is broken in production browsers: Play produces ~1 ms of audio, then silence; Play stays grayed out; mod scopes freeze; Randomize appears dead. DevTools shows `Uncaught ReferenceError: TextDecoder is not defined` in `froggers-processor.ts` at `readCString` → `postScreen` → `process` frame 20. `TextDecoder` is not available in `AudioWorkletGlobalScope`. The worklet dies on the first screen post while main-thread `audioRunning` stays true — every symptom traces to this crash, not transport wiring alone. Prior session shipped knob live-value fixes and transport recovery in `main.ts`, but none of that matters until the worklet survives `postScreen`.

## What Changes

- **Worklet string decode**: Replace `new TextDecoder().decode(...)` in `readCString` with a manual UTF-8 decode over WASM memory bytes (ASCII parameter names only today). This is the default-safe choice for AudioWorklet — same as any embedded scope that cannot assume full DOM globals.
- **Worklet crash surfacing**: Wrap `process()` body in try/catch; on failure post `{ type: "error", message }` so main runs `stopAudio()`, re-enables Play, and shows retry status (handler already exists in `main.ts`).
- **Immediate UI after randomize**: Call `postScreen()` after `marbles`, `randomizeAll`, and `randomizeMod` in the worklet (page randomize paths already do). Knobs and scopes update on the next frame without waiting for audio tick 20 — [thenoriegas.info](https://thenoriegas.info) parity.
- **Engine start screen tick**: Call `postScreen()` when `setRunning(true)` so UI populates before frame 20 if audio is silent or slow to start.
- **Handoff bookkeeping**: Mark `web-knob-live-values` tasks done where code already matches spec (`syncKnobUi` uses `row.value`, drag snap, static labels).

## Capabilities

### New Capabilities

- `web-worklet-cstring-decode`: Decode WASM C strings inside the AudioWorklet without `TextDecoder`.
- `web-worklet-process-errors`: Fatal errors inside `process()` propagate to main transport recovery.
- `web-randomize-immediate-ui`: Global randomize actions post an immediate `screen` message like page randomize already does.

### Modified Capabilities

- `web-transport-state-ownership`: Worklet runtime death (not just explicit `error` messages from constructor) SHALL recover transport UI via the new process-error path.

## Impact

- `web/src/froggers-processor.ts` — `readCString`, `process`, `handleUi` (`setRunning`, randomize handlers)
- `web/src/main.ts` — error handler already wired; verify after worklet fix only
- `openspec/changes/web-knob-live-values/tasks.md` — check off completed items after verification
- No WASM rebuild required for string decode (names are ASCII from existing exports)
- Reference: [thenoriegas.info](https://thenoriegas.info) — knobs/scopes/randomize all driven by live state posts
