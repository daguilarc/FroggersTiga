## ADDED Requirements

### Requirement: WASM URL uses Vite public root

The web sim SHALL resolve `froggers.wasm` via `` `${import.meta.env.BASE_URL}froggers.wasm` `` (or equivalent) on the main UI thread. It SHALL NOT import WASM from `../public/froggers.wasm?url`.

#### Scenario: Dev server starts without public-path warning

- **WHEN** the developer runs `npm run dev` with `web/public/froggers.wasm` present
- **THEN** Vite does not emit "use `/froggers.wasm` instead of `/public/froggers.wasm`" warnings for the WASM load path

#### Scenario: Dev fetch succeeds

- **WHEN** the sim loads at `http://localhost:<port>/`
- **THEN** main-thread `fetch` of `froggers.wasm` returns HTTP 200
- **AND** the worklet posts `{ type: "ready" }` followed by `{ type: "screen" }`

#### Scenario: Production and GitHub Pages

- **WHEN** the user opens the built sim from `dist/` or published `docs/`
- **THEN** WASM fetch resolves relative to `import.meta.env.BASE_URL`
- **AND** Play produces audible output with External off

### Requirement: Missing WASM fails fast on dev start

When `web/public/froggers.wasm` is absent, `npm run dev` SHALL exit with an actionable error before Vite starts.

#### Scenario: Fresh clone without wasm build

- **WHEN** the developer runs `npm run dev` without `web/public/froggers.wasm`
- **THEN** the terminal prints instructions to run `npm run build:wasm` or `npm run build:all`
- **AND** the dev server does not start
