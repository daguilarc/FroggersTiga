## 1. WASM URL fix (main thread)

- [x] 1.1 `web/src/main.ts`: remove `import wasmUrl from "../public/froggers.wasm?url"`; use `` const wasmUrl = `${import.meta.env.BASE_URL}froggers.wasm` ``
- [x] 1.2 Confirm `initWorklet()` still `fetch` → `arrayBuffer` → `WebAssembly.compile` → `processorOptions: { wasmModule }`
- [x] 1.3 `npm run dev`: no Vite public-path warnings for WASM

## 2. WASM export parity

- [x] 2.1 `wasm/CMakeLists.txt`: add `_froggers_randomize_page` and `_froggers_randomize_page_mod` to `EXPORTED_FUNCS`
- [x] 2.2 `scripts/verify-wasm-exports.mjs`: derive `REQUIRED_EXPORTS` from `WasmExports` in `froggers-processor.ts` (single list); instantiate wasm with stubs; assert every name present
- [x] 2.3 `web/package.json`: append `node ../scripts/verify-wasm-exports.mjs` to `build:wasm` after copy step
- [x] 2.4 `npm run build:wasm` — verify script passes

## 3. Dev ergonomics

- [x] 3.1 `scripts/check-wasm-present.mjs`: exit 1 with `npm run build:wasm` hint if `web/public/froggers.wasm` missing
- [x] 3.2 `web/package.json`: add `"predev": "node ../scripts/check-wasm-present.mjs"`
- [x] 3.3 `README.md`: first-run path `cd web && npm run build:all && npm run dev`

## 4. External audio permission

- [x] 4.1 Refactor `setExternalEnabled`: do not set `externalEnabled`, button label, or WASM `external: true` until `getUserMedia` succeeds and `micSource.connect(workletNode)` completes
- [x] 4.2 On External click: `await initWorklet()` when `!workletNode` (page-load bootstrap may still be in flight); then run permission flow
- [x] 4.3 Optional `permissions.query({ name: "microphone" })` — if `denied`, show blocked message without calling `getUserMedia`
- [x] 4.4 Map `NotAllowedError` / `NotFoundError` / insecure context to readable status + recovery hint
- [x] 4.5 WASM `external: true` sent only after stream connected; revert on failure

## 5. Browser verification (required before archive)

- [x] 5.1 `npm run dev`: status **Engine ready — click Play** without console errors
- [ ] 5.2 Before Play: Audio page knob labels show VCO1…Crunch from first `screen`
- [ ] 5.3 Play → VCO audible, External off, no mic prompt
- [ ] 5.4 Stop → silent; Play re-enabled
- [ ] 5.5 Filter page row 0 label **Comb offset** (not Manual text)
- [ ] 5.6 Page **Randomize** on Audio changes knobs without error
- [x] 5.7 `npm run build` + `npm run preview`: same bootstrap behavior
- [ ] 5.8 External on → permission prompt → grant → ring mod audible
- [ ] 5.9 External on with mic denied → stays Off, status shows recovery instructions
