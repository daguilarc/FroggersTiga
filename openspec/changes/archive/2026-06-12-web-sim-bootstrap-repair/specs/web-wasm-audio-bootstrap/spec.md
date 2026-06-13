## MODIFIED Requirements

### Requirement: WASM loads from Vite-resolved URL

The main UI thread SHALL fetch `froggers.wasm` using a build-time URL (Vite `?url` import or equivalent) so the fetch target matches the emitted asset in dev (`npm run dev`), production (`npm run build`), and GitHub Pages (`docs/`). The AudioWorklet SHALL receive the bytes via `processorOptions` and instantiate synchronously — it SHALL NOT perform its own network fetch.

#### Scenario: Production build Play succeeds

- **WHEN** the user opens the built sim from `dist/` or published `docs/` and clicks **Play**
- **THEN** main-thread WASM fetch returns HTTP 200
- **AND** status shows **Playing** with sample rate
- **AND** audio output is non-silent with default knob values and External off

#### Scenario: Dev server Play succeeds

- **WHEN** the user runs `npm run dev` and clicks **Play**
- **THEN** WASM loads without 404
- **AND** the worklet posts `{ type: "ready" }` followed by `{ type: "screen" }`

### Requirement: Play error surfaces clearly

When WASM load or AudioWorklet setup fails, the status line SHALL show a specific error message. The UI SHALL not silently return to **Audio stopped** without displaying the failure reason.

#### Scenario: WASM missing

- **WHEN** main-thread WASM fetch fails
- **THEN** status displays `Engine error: WASM fetch failed: <status>` or the caught exception message
- **AND** Play remains disabled

#### Scenario: Worklet instantiate failure

- **WHEN** `WebAssembly.instantiate` throws in the worklet
- **THEN** status displays `Error: <exception message>`
- **AND** Play remains disabled
