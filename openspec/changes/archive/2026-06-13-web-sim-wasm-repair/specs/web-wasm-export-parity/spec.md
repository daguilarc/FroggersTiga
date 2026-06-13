## ADDED Requirements

### Requirement: WASM exports match processor interface

The built `froggers.wasm` SHALL export every function invoked from `web/src/froggers-processor.ts`, including `_froggers_randomize_page` and `_froggers_randomize_page_mod`.

#### Scenario: Per-page Randomize works

- **WHEN** the user clicks **Randomize** on the Audio page while audio is running or stopped
- **THEN** knob values change without console errors
- **AND** the OLED and knob columns refresh from a new `screen` payload

#### Scenario: Per-page Randomize mod works

- **WHEN** the user clicks **Randomize mod** on any core page (0–4)
- **THEN** mod sources and depths update without throwing

### Requirement: Automated export smoke test

`npm run build:wasm` SHALL run `node ../scripts/verify-wasm-exports.mjs` after copying `froggers.wasm` to `web/public/`. The script SHALL derive its required export name list from the `WasmExports` interface in `web/src/froggers-processor.ts` and assert every name exists in the built module.

#### Scenario: Missing export blocks wasm copy

- **WHEN** `froggers_randomize_page` is removed from `EXPORTED_FUNCS` but remains in bindings
- **THEN** the verify script exits non-zero
- **AND** the developer sees which export is missing
