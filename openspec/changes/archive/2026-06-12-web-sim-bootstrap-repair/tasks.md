## 1. Main-thread WASM load (blocks all WASM UI)

- [x] 1.1 `main.ts`: `import wasmUrl from "../public/froggers.wasm?url"`; fetch `arrayBuffer()` in `initWorklet()` parallel with `addModule`
- [x] 1.2 `main.ts`: pass `processorOptions: { wasmBytes }` to `AudioWorkletNode` constructor
- [x] 1.3 `froggers-processor.ts`: remove `fetch`, async `loadWasm`, and `wasmUrl` import
- [x] 1.4 `froggers-processor.ts`: sync `WebAssembly.instantiate(bytes)` in constructor from `processorOptions.wasmBytes`; post `ready` + `setHostPage(0)` on success
- [x] 1.5 `froggers-processor.ts`: post `{ type: "error" }` when `wasmBytes` missing or instantiate throws
- [ ] 1.6 Verify `npm run dev`: status **Engine ready — click Play**; no **fetch is not defined**

## 2. Parameter labels

- [x] 2.1 `main.ts`: remove hardcoded `i < 7 ? "—" : "Crunch"` init; use empty string until first `screen`
- [x] 2.2 Confirm `updateKnobLabels` sets all eight names from `screen.rows` including **Crunch** on row 7
- [ ] 2.3 Verify labels visible before Play on Audio page (VCO1…Crunch)

## 3. Mod bay hint dedup

- [x] 3.1 `index.html`: replace static hint with single **CV trace while playing** on toggle row (or move to toggle `aria-describedby`)
- [x] 3.2 `main.ts`: remove hint element creation inside `initModBay()`
- [x] 3.3 Confirm only one hint visible when mod bay expanded

## 4. Mobile field layout

- [x] 4.1 `style.css`: `@media (max-width: 720px)` — `.oled { min-height: 0 }`; hide `.oled-name` and `.oled-bar-wrap` in compact mode
- [x] 4.2 `main.ts` or CSS: apply `.oled--compact` on mobile (or pure CSS media query on `.oled` descendants)
- [x] 4.3 Reduce `.knobs` fixed `height: 220px` on mobile if excess gap remains between chrome and knobs
- [ ] 4.4 Verify 390 px viewport: no large black void; knob labels readable; wave buttons on Audio page

## 5. Build + manual verification

- [x] 5.1 `npm run build` — open `dist/index.html`; Play works; WASM loads
- [ ] 5.2 GitHub Pages path: confirm `docs/` asset resolves (if applicable)
- [ ] 5.3 Manual: Play → VCO audible; Stop → silent; page pill while stopped updates labels
- [ ] 5.4 Manual: External off — no mic prompt on Play
