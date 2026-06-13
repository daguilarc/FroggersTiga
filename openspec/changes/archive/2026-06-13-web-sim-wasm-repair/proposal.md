## Why

The browser sim still fails the basic contract: engine bootstrap is fragile, Vite warns on every dev start about an invalid WASM path, per-page **Randomize** calls WASM exports that were never added to `CMakeLists.txt`, and `web-sim-bootstrap-repair` was archived with verification tasks unchecked. The terminal warnings are a real signal — `import wasmUrl from "../public/froggers.wasm?url"` is the wrong Vite pattern for `public/` assets and must be replaced before we trust dev or Pages deploys.

## What Changes

### WASM bootstrap path (blocks Play + labels)

- Replace `../public/froggers.wasm?url` import with a Vite-correct public URL: `` `${import.meta.env.BASE_URL}froggers.wasm` `` fetched on the main thread.
- Keep `WebAssembly.compile()` on main thread; pass `WebAssembly.Module` via `processorOptions.wasmModule` (structured-cloneable, sync instantiate in worklet).
- Add `predev` guard: if `web/public/froggers.wasm` is missing, print actionable error (`npm run build:wasm`) and abort dev start.

### WASM export gap (per-page randomize)

- Add `_froggers_randomize_page` and `_froggers_randomize_page_mod` to `wasm/CMakeLists.txt` `EXPORTED_FUNCS`.
- Rebuild `web/public/froggers.wasm`; wire `npm run build:all` as documented first-run path.

### Verification gate (no more false-done)

- Mandatory browser checklist before archive: status **Engine ready**, first `screen` populates knob labels without Play, Play audible with External off, Stop silent, page **Randomize** does not throw.
- Add a small Node smoke script (`scripts/verify-wasm-exports.mjs`) that asserts required export names exist — run in `build:wasm` post-step.

### External audio permission (mic / line-in)

- **Pessimistic UI** — External stays **Off** until `getUserMedia` resolves; WASM `external: true` only after stream connects.
- **Explicit prompt on user gesture** — `getUserMedia` runs on External button click (user gesture). If audio is not running, ensure `AudioContext` + worklet exist first (`initWorklet` if needed) so the permission dialog is not deferred to a non-gesture path.
- **Denied / blocked handling** — map `NotAllowedError`, `NotFoundError`, and insecure-context failures to readable status text with recovery steps (browser site-settings / lock icon → allow microphone).
- **Permissions API probe** — on External click, if `navigator.permissions.query({ name: "microphone" })` returns `denied`, show blocked message before calling `getUserMedia` (avoids silent auto-deny with no explanation).

### Dev ergonomics

- Silence Vite public-path warnings on `npm run dev`.
- README: first clone → `npm run build:all` then `npm run dev`.

## Capabilities

### New Capabilities

- `web-wasm-public-url`: Correct Vite public-dir WASM URL resolution for dev, `dist/`, and GitHub Pages `BASE_URL`.
- `web-wasm-export-parity`: WASM export list matches `froggers-processor.ts` interface; automated export smoke test.
- `web-external-audio-permission`: Mic permission request, denial recovery, pessimistic External toggle.

### Modified Capabilities

- `sim-parameter-display-names`: Web knob labels SHALL come from WASM `screen` payload only after successful bootstrap (no `—` placeholders).
- `web-wasm-audio-bootstrap`: Promote archived bootstrap spec to `openspec/specs/`; URL resolution superseded by `web-wasm-public-url` (archived `?url` pattern rejected).
- `web-simulator`: External toggle delta only; permission flow owned by `web-external-audio-permission`.

## Impact

- `web/src/main.ts` — WASM URL, bootstrap error messages, external permission flow, optional predev hook via package.json
- `web/package.json` — `predev` script, document `build:all`
- `web/src/froggers-processor.ts` — no path change; confirm `wasmModule` contract documented
- `wasm/CMakeLists.txt` — two missing exports
- `scripts/verify-wasm-exports.mjs` — new export smoke test
- `README.md` — first-run instructions
- Supersedes incomplete verification from archived `web-sim-bootstrap-repair` tasks 1.6, 2.3, 5.3
