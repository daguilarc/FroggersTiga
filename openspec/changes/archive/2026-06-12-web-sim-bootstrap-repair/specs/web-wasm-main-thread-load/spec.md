## ADDED Requirements

### Requirement: WASM fetch runs on main thread only

The browser sim SHALL fetch `froggers.wasm` on the main UI thread using a Vite `?url` import. The resulting `ArrayBuffer` SHALL be passed to the AudioWorklet via `AudioWorkletNode` `processorOptions` before the node is constructed. The AudioWorklet processor SHALL NOT call `fetch`, `XMLHttpRequest`, or other main-thread-only network APIs.

#### Scenario: Worklet instantiates from transferred bytes

- **WHEN** `initWorklet()` completes successfully
- **THEN** the worklet has received `wasmBytes` in `processorOptions`
- **AND** `WebAssembly.instantiate` runs synchronously in the worklet constructor
- **AND** the worklet posts `{ type: "ready" }` followed by `{ type: "screen" }`

#### Scenario: Missing wasmBytes fails clearly

- **WHEN** the worklet is constructed without `processorOptions.wasmBytes`
- **THEN** the worklet posts `{ type: "error", message: "Missing wasmBytes in processorOptions" }`
- **AND** Play remains disabled

### Requirement: WASM fetch errors surface on status line

Main-thread WASM fetch failures SHALL display a specific error on the status line. Play SHALL remain disabled until a successful load.

#### Scenario: WASM 404

- **WHEN** `fetch(wasmUrl)` returns a non-OK status
- **THEN** status displays `Engine error: WASM fetch failed: <status>` or the caught exception message
- **AND** Play is disabled

#### Scenario: Successful load enables Play

- **WHEN** WASM loads and the worklet posts `ready`
- **THEN** Play is enabled
- **AND** status displays **Engine ready — click Play**
