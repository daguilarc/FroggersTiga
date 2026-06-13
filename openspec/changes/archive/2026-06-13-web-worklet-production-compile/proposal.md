## Why

GitHub Pages audio is dead because the AudioWorklet module shipped to production is **raw TypeScript**, not JavaScript. Vite `?url` copies the source file as a static asset; dev mode transpiles on the fly, so localhost falsely passes. Renaming the asset to `.js` only fixed MIME type — the browser still parses `interface` and `: type` annotations and throws `Missing initializer in const declaration` on `addModule()`.

Deployed artifact proof: `docs/assets/froggers-processor-CgwnExQX.ts` begins with `const WASM_IMPORTS: WebAssembly.Imports =`.

## What Changes

- Add an explicit **esbuild compile step** for `froggers-processor.ts` → `web/public/froggers-processor.js` (same pipeline shape as pre-built WASM: source → script → public → Vite copies to dist).
- Load the worklet via `BASE_URL + 'froggers-processor.js'` in `main.ts` — remove `?url` import.
- Wire `build:worklet` → `verify:worklet` into `npm run build`, `npm run predev`, and CI Pages workflow (already runs `npm run build`; no workflow edit).
- Add `esbuild` as an explicit `web` devDependency (deterministic import from `scripts/build-worklet.mjs`; do not rely on transitive Vite nesting).
- Revert the broken `assetFileNames` `.ts`→`.js` rename hack in `vite.config.ts`.
- Add `scripts/verify-worklet-js.mjs` — required gate; scans the **entire** emitted file for TypeScript syntax markers.
- Treat `web/public/froggers-processor.js` as generated output (regenerated every dev/build; not source-controlled).
- Update `web-worklet-module-load` spec: production worklet MUST be transpiled JS at a stable public URL with valid `application/javascript` MIME on static hosts.

## Data Flow

```
web/src/froggers-processor.ts
  → scripts/build-worklet.mjs (esbuild, ESM, es2020)
  → web/public/froggers-processor.js
  → vite build copies public/* → dist/froggers-processor.js
  → main.ts: processorUrl = BASE_URL + 'froggers-processor.js'
  → audioContext.audioWorklet.addModule(processorUrl)
```

## Capabilities

### New Capabilities

- `web-worklet-esbuild`: Explicit esbuild pipeline for AudioWorklet source; dev and production parity.

### Modified Capabilities

- `web-worklet-module-load`: Replace "plain `?url` import" requirement with "transpiled `.js` in public URL space"; add production parse/MIME scenarios.

## Impact

- `web/src/main.ts` — processor URL source
- `web/src/froggers-processor.ts` — unchanged logic; build target only
- `web/vite.config.ts` — remove asset rename hack
- `web/package.json` — `esbuild` devDependency; `build:worklet`, `verify:worklet`, `predev`, `build` scripts
- `scripts/build-worklet.mjs` (new), `scripts/verify-worklet-js.mjs` (new)
- `.github/workflows/pages.yml` — inherits via `npm run build` (no edit)
- `openspec/specs/web-worklet-module-load/spec.md` — requirement correction on archive
