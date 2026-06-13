## ADDED Requirements

### Requirement: Main-thread compile, worklet sync instantiate

The main UI thread SHALL `fetch` WASM bytes, `WebAssembly.compile()` to a `WebAssembly.Module`, and pass that module to the AudioWorklet via `processorOptions.wasmModule`. The worklet SHALL construct `new WebAssembly.Instance(wasmModule, imports)` synchronously in its constructor.

WASM fetch URL resolution is owned by `web-wasm-public-url`; this requirement covers compile/instantiate only.

#### Scenario: No fetch in AudioWorklet

- **WHEN** reviewing `web/src/froggers-processor.ts`
- **THEN** no `fetch`, `XMLHttpRequest`, or dynamic `import()` loads WASM
- **AND** bootstrap does not throw `fetch is not defined`

### Requirement: Play error surfaces clearly

When WASM load or AudioWorklet setup fails, the status line SHALL show a specific error message. Play SHALL remain disabled until `{ type: "ready" }` is received.

#### Scenario: WASM fetch failure

- **WHEN** main-thread WASM fetch returns non-200
- **THEN** status displays `Engine error: WASM fetch failed: <status>` or the caught exception message
- **AND** Play remains disabled

#### Scenario: Worklet instantiate failure

- **WHEN** `WebAssembly.Instance` throws in the worklet constructor
- **THEN** status displays `Error: <exception message>`
- **AND** Play remains disabled
