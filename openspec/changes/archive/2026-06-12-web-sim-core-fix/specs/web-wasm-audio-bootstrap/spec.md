## ADDED Requirements

### Requirement: WASM loads from Vite-resolved URL

The AudioWorklet SHALL load `froggers.wasm` using a build-time URL (Vite `?url` import or equivalent) so the fetch target matches the emitted asset in dev (`npm run dev`), production (`npm run build`), and GitHub Pages (`docs/`).

#### Scenario: Production build Play succeeds

- **WHEN** the user opens the built sim from `dist/` or published `docs/` and clicks **Play**
- **THEN** WASM fetch returns HTTP 200
- **AND** status shows **Playing** with sample rate
- **AND** audio output is non-silent with default knob values and External off

#### Scenario: Dev server Play succeeds

- **WHEN** the user runs `npm run dev` and clicks **Play**
- **THEN** WASM loads without 404
- **AND** the worklet posts `{ type: "ready" }` followed by `{ type: "screen" }`

### Requirement: Initial screen before audio runs

The worklet SHALL post a `screen` message after WASM `ready` and host page selection, before the UI sends `setRunning: true`. The UI SHALL apply knob labels, OLED rows, and mod scope idle state from this message while transport is stopped.

#### Scenario: Labels visible before Play

- **WHEN** the page loads and the user has not clicked Play
- **THEN** knob column labels show `ParamDisplayNames` for host page 0 (e.g. **VCO1**, not **Knob 1**)
- **AND** OLED rows list the same parameter names

#### Scenario: Page change while stopped

- **WHEN** audio is stopped and the user selects another page pill
- **THEN** knob labels update to that page's `ParamDisplayNames` without requiring Play

### Requirement: Play error surfaces clearly

When WASM load or AudioWorklet setup fails, the status line SHALL show a specific error message. The UI SHALL not silently return to **Audio stopped** without displaying the failure reason.

#### Scenario: WASM missing

- **WHEN** WASM fetch fails
- **THEN** status displays `Error: WASM fetch failed: <status>` or the caught exception message
- **AND** Play is re-enabled
