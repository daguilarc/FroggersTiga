## Context

```
main.ts Play click
    → fetch(wasmUrl)           ✓ compiled WASM in public/
    → addModule(processorUrl)  ✗ raw TS masquerading as .js

Dev (Vite):  ?url → dev server transpiles on fetch → works
Prod build:  ?url → Rollup copies src verbatim → interface, : number survive
Pages:       .ts MIME video/mp2t OR .js MIME but TS syntax → both fail
```

`web-sim-bootstrap-fix` chose `?url` over `?worker&url` to avoid Vite HMR client injection in AudioWorkletGlobalScope. That trade was correct for dev HMR avoidance, but **`?url` never transpiles in production** — a gap nobody verified with `npm run build && vite preview` against GitHub Pages MIME rules.

The failed `.js` rename in `vite.config.ts` (`assetFileNames`) only changed the filename extension; Rollup still emitted TypeScript source. Browser error: `Missing initializer in const declaration` at line 1 (`const WASM_IMPORTS: WebAssembly.Imports =`).

`readCString` / TextDecoder fix is already landed; this change does not touch worklet DSP logic.

## Data Flow

| Stage | Input | Transform | Output |
|-------|-------|-----------|--------|
| Compile | `web/src/froggers-processor.ts` | `scripts/build-worklet.mjs` (esbuild) | `web/public/froggers-processor.js` |
| Verify | compiled JS file | `scripts/verify-worklet-js.mjs` | exit 0 or CI fail |
| Bundle | `web/public/*` | `vite build` (public dir copy) | `web/dist/froggers-processor.js` |
| Runtime | `processorUrl` string | `addModule()` | AudioWorkletGlobalScope loads plain ESM |

Reuse the existing script conventions from `scripts/verify-wasm-exports.mjs` and `scripts/check-wasm-present.mjs`: resolve repo root via `fileURLToPath`, explicit paths, `process.exit(1)` on failure, success log line.

## Goals / Non-Goals

**Goals:**

- Production and dev load the **same class of artifact**: plain ES module JavaScript at a stable public URL (`/froggers-processor.js`, not hashed `assets/` path).
- `addModule()` succeeds on GitHub Pages (correct MIME + parseable JS).
- Build fails fast if worklet output still contains TypeScript markers.
- Mirror existing WASM pattern: explicit pre-build script, verified in CI via `npm run build`.

**Non-Goals:**

- Switching back to `?worker&url` (HMR injection risk remains).
- Bundling worklet into main chunk (AudioWorklet requires separate module URL).
- Changing worklet DSP logic or `readCString`.
- esbuild `--watch` in dev (add only if edit-rebuild friction is reported).

## Decisions

### D1: esbuild precompile to `web/public/froggers-processor.js`

**Choice:** `scripts/build-worklet.mjs` runs esbuild on `web/src/froggers-processor.ts` → `web/public/froggers-processor.js` with `format: 'esm'`, `target: 'es2020'`, `platform: 'browser'`, `bundle: false` (single-file transpile, no bundling of deps).

**Dependency:** Add `esbuild` (^0.25.0, aligned with Vite 6) as explicit `devDependency` in `web/package.json`. Import from `join(root, 'web', 'node_modules', 'esbuild')` in the script — same root-resolution pattern as `verify-wasm-exports.mjs`.

**Why:** Same pipeline shape as WASM (`build:wasm` → public). No Rollup asset-copy path. One stable URL like `wasmUrl`.

**Alternative rejected:** Keep `?url` + Vite plugin to transpile assets — harder to verify, duplicates esbuild already in toolchain.

**Alternative rejected:** Rollup second entry — main bundle cannot import hashed worklet URL without extra plumbing; public URL is simpler.

**Alternative rejected:** Rely on transitive `esbuild` from Vite without explicit devDependency — hoisting varies; import path is non-deterministic.

### D2: `main.ts` uses `BASE_URL + 'froggers-processor.js'`

**Choice:**

```typescript
const processorUrl = `${import.meta.env.BASE_URL}froggers-processor.js`;
```

Remove `import processorUrl from "./froggers-processor.ts?url"`.

**Why:** Identical to `wasmUrl` pattern already proven in Pages CI.

### D3: Dev runs compile before `vite`

**Choice:**

```json
"predev": "npm run build:worklet && node ../scripts/check-wasm-present.mjs"
```

`predev` runs once at dev server start. After editing `froggers-processor.ts` during a live session, run `npm run build:worklet` manually or restart dev.

**Why:** Dev must not rely on Vite transforming a `?url` asset that no longer exists.

### D4: Verify script rejects TypeScript leakage

**Choice:** `scripts/verify-worklet-js.mjs` reads the **entire** output file and fails if any of these match:

- `/\binterface\s+[A-Za-z_]/` — TypeScript interface declarations
- `/:\s*WebAssembly\./` — typed WASM import (first-line smoke marker from deployed failure)
- `/\)\s*:\s*(number|void|string|boolean)\s*[{;]/` — return type annotations on functions

Mirror `verify-wasm-exports.mjs` structure: root path resolution, read file once, exit 1 with message on match, log OK on pass.

**Why:** The deployed failure mode is TypeScript syntax surviving at the top of the file; full-file scan catches regressions anywhere in output. Regex gate is sufficient — esbuild output is predictable; no need for a second parser.

**Rejected:** Scanning only the first 500 characters — TypeScript markers can appear after minification headers or comments.

### D5: Revert `vite.config.ts` assetFileNames hack

**Choice:** Remove `.ts`→`.js` rename block entirely; worklet no longer goes through Vite asset pipeline.

### D6: Generated worklet JS is not source-controlled

**Choice:** `web/public/froggers-processor.js` is regenerated by `build:worklet` on every `predev` and `build`. Do not commit it (unlike `froggers.wasm`, which requires emsdk to rebuild).

**Why:** esbuild is lightweight and always available after `npm install`; committing generated JS creates stale-artifact drift.

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| Stale worklet after TS edit during live dev | Document `npm run build:worklet` or dev restart in tasks |
| esbuild version drift | Explicit devDependency pinned to Vite-compatible ^0.25.0 |
| Hash cache on Pages | Fixed filename `froggers-processor.js`; cache bust via deploy only |
| verify regex false positive | Patterns target TypeScript-only syntax absent from es2020 JS output |

## Migration Plan

1. Land build script + verify script + package.json wiring + main.ts URL change + revert vite hack.
2. Run `npm run build` locally; `VITE_BASE=/FroggersTiga/ npm run preview`; confirm Play + audio.
3. Push; Pages workflow rebuilds via existing `npm run build`; verify live URL.
4. Archive change; merge spec delta into `openspec/specs/web-worklet-module-load/spec.md`.

## Open Questions

None — root cause confirmed in deployed artifact bytes.
