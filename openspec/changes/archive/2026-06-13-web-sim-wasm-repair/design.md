## Context

Bootstrap data flow today:

```
main.ts                          AudioWorklet (froggers-processor.ts)
────────                         ────────────────────────────────────
fetch(wasmUrl) ──compile──▶ WebAssembly.Module
       │                              │
       │    processorOptions          │ new Instance(module, WASM_IMPORTS)
       └──────────────────────────────▶ froggers_create() → post "ready" + "screen"
```

`web-sim-bootstrap-repair` fixed the fatal `fetch is not defined` in the worklet and switched to main-thread `WebAssembly.compile()`. That part is structurally correct.

Remaining failures:

1. **Wrong Vite import** — `import wasmUrl from "../public/froggers.wasm?url"` resolves to `/public/froggers.wasm?url`. Vite warns on every dev start and documents the correct pattern: public assets are served at root (`/froggers.wasm`), not under `/public/`.
2. **Export gap** — `froggers_randomize_page` / `froggers_randomize_page_mod` exist in `bindings.cpp` but not in `wasm/CMakeLists.txt`. Verified: `WebAssembly.instantiate` succeeds but those two exports are absent; page **Randomize** is a no-op or throws at runtime.
3. **False completion** — archived change left verification tasks unchecked; no automated export smoke test.
4. **External permission UX** — `setExternalEnabled(true)` flips UI and WASM to **On** before `getUserMedia` succeeds. If the user previously denied mic access, many browsers return `NotAllowedError` immediately with no second prompt. Status shows a raw exception string; recovery path is unclear.

WASM itself instantiates cleanly with current stubs (`wasi_snapshot_preview1.fd_write`, `env.emscripten_notify_memory_growth`). `STANDALONE_WASM=1` is set; stubs remain harmless.

## Goals / Non-Goals

**Goals:**

- Zero Vite public-path warnings on `npm run dev`
- WASM loads on main thread; worklet sync-instantiates from precompiled `WebAssembly.Module`
- All `froggers-processor.ts` export names present in built `.wasm`
- Clear failure when `web/public/froggers.wasm` missing on dev start
- Browser verification checklist completed before archive
- External mic permission requested on user gesture; denial surfaced with recovery instructions

**Non-Goals:**

- Rewriting the web layout / mobile OLED compaction (already done in bootstrap-repair)
- Desktop changes
- Removing WASM import stubs (works; STANDALONE build may still reference env symbols)

## Decisions

### D1: Public URL via `import.meta.env.BASE_URL` (not `?url` from `public/`)

**Choice:** `const wasmUrl = \`${import.meta.env.BASE_URL}froggers.wasm\``; `fetch(wasmUrl)` in `initWorklet()`.

**Why:** Vite serves `web/public/froggers.wasm` at `/froggers.wasm` in dev and copies it to `dist/` root on build. `BASE_URL` handles GitHub Pages subpath deploys. Eliminates the terminal warning.

**Alternative rejected:** Keep `?url` import from `../public/` — triggers warnings; non-idiomatic.

**Alternative rejected:** Move WASM to `src/assets/` — duplicates `build:wasm` copy target; `public/` is already CI convention.

### D2: Keep `WebAssembly.compile()` on main thread, pass `Module` not `ArrayBuffer`

**Choice:** Continue `processorOptions: { wasmModule }` after `WebAssembly.compile(await response.arrayBuffer())`.

**Why:** `WebAssembly.Module` is structured-cloneable into the worklet. Worklet constructor stays synchronous — no async constructor trap. Matches current working code path.

### D3: Export parity enforced by script

**Choice:** `scripts/verify-wasm-exports.mjs` reads `web/public/froggers.wasm`, instantiates with stubs, asserts export name set matches a canonical list (including `froggers_randomize_page`, `froggers_randomize_page_mod`). Hook into `npm run build:wasm` tail.

**Why:** Prevents repeat of "bindings.cpp has it, CMakeLists doesn't" drift.

### D4: `predev` missing-WASM guard

**Choice:** `package.json` `"predev": "node ../scripts/check-wasm-present.mjs"` — exits 1 with message if `public/froggers.wasm` absent.

**Why:** Fresh clone + `npm run dev` currently yields opaque fetch failure.

### D5: Pessimistic External toggle + permission-first flow

**Choice:** Refactor `setExternalEnabled` into two phases:

```
User clicks External
       │
       ▼
┌──────────────────┐     denied/blocked     ┌─────────────────────────┐
│ ensure audio ctx │ ──────────────────────▶ │ status: blocked message │
│ (init if needed) │                         │ External stays Off      │
└────────┬─────────┘                         └─────────────────────────┘
         │ granted
         ▼
┌──────────────────┐
│ getUserMedia     │ ──▶ connect micSource ──▶ UI On + WASM external:true
└──────────────────┘
```

- UI and WASM `external` flag update **only after** `MediaStream` connects.
- On click: optional `permissions.query({ name: "microphone" })` — if `denied`, skip `getUserMedia` and show site-settings instructions.
- Map errors: `NotAllowedError` → "Microphone blocked — allow mic for this site in browser settings, then click External again"; `NotFoundError` → "No microphone found"; insecure context → "HTTPS required for microphone".
- If `!workletNode`, `await initWorklet()` inside the click handler before `getUserMedia`. Do not update button label or WASM `external` until the stream connects.

**Why:** Optimistic On-before-stream misleads users; auto-denied browsers never re-prompt without settings change. Current code flips UI/WASM at the top of `setExternalEnabled` then returns early when `!workletNode` — that is the bug.

**Permission detail:** See `web-external-audio-permission` spec; `web-simulator` delta only restates the External toggle contract.

**Alternative rejected:** Request mic on page load — violates "no prompt on Play with External off" and annoys VCO-only users.

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| `BASE_URL` mismatch on exotic deploy | CI Pages workflow uses same `vite build`; smoke test on `dist/` |
| Export list drifts again | Single `REQUIRED_EXPORTS` array in verify script; processor TS types as second check |
| User skips `build:wasm` | `predev` guard + README first-run |
| Randomize throws if export still missing | Verify script blocks bad wasm copy |
| Permissions API unsupported (Safari) | Fall through to `getUserMedia`; catch `NotAllowedError` |
| External before Play needs AudioContext | On External click: `await initWorklet()` when `!workletNode`; pessimistic UI until stream connects |

## Migration Plan

1. Fix `main.ts` URL (`web-wasm-public-url`)
2. Add CMake exports → `npm run build:wasm` → verify script passes (`web-wasm-export-parity`)
3. Refactor external permission flow in `main.ts` (`web-external-audio-permission`)
4. Browser smoke test locally (tasks §5)
5. Push; CI rebuilds wasm + web → `docs/`
6. Archive `web-sim-wasm-repair` only after checklist green; promote new specs to `openspec/specs/`

## Open Questions

None blocking implementation.
