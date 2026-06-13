## ADDED Requirements

### Requirement: Worklet source compiles before Vite build

The web sim SHALL compile `web/src/froggers-processor.ts` to `web/public/froggers-processor.js` via an explicit build script before `vite build` and before `vite` dev server start.

#### Scenario: Production build emits JavaScript

- **WHEN** `npm run build` completes
- **THEN** `web/public/froggers-processor.js` exists
- **AND** `web/dist/froggers-processor.js` exists at the dist root (copied from public)
- **AND** the file does not contain TypeScript `interface` declarations or parameter type annotations

#### Scenario: Dev pre-step compiles worklet

- **WHEN** the developer runs `npm run dev`
- **THEN** the worklet compile script runs before Vite starts
- **AND** `addModule(`${BASE_URL}froggers-processor.js`)` succeeds on Play

#### Scenario: Worklet not in hashed assets pipeline

- **WHEN** `npm run build` completes
- **THEN** no file matching `dist/assets/froggers-processor-*` exists
- **AND** the worklet is served from the stable public URL path, not a Rollup-hashed asset

### Requirement: Build verification gate

The repository SHALL include a verification script that fails CI if the worklet output is missing or contains TypeScript syntax markers. The script SHALL scan the entire output file.

#### Scenario: CI rejects raw TypeScript output

- **WHEN** `verify-worklet-js` runs against a file containing `interface WasmExports`
- **THEN** the script exits non-zero

#### Scenario: CI rejects partial TypeScript leakage

- **WHEN** `verify-worklet-js` runs against a file containing `: WebAssembly.Imports` type annotation
- **THEN** the script exits non-zero

#### Scenario: CI accepts valid output

- **WHEN** `verify-worklet-js` runs after a successful esbuild compile
- **THEN** the script exits zero

### Requirement: Explicit esbuild dependency

The web package SHALL declare `esbuild` as an explicit devDependency so `scripts/build-worklet.mjs` imports it deterministically from `web/node_modules`.

#### Scenario: Fresh clone builds worklet

- **WHEN** a developer runs `cd web && npm install && npm run build:worklet`
- **THEN** esbuild resolves without relying on transitive Vite hoisting
- **AND** `web/public/froggers-processor.js` is created
