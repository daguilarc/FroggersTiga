## 1. WASM transport + lifecycle (blocks web)

- [x] 1.1 `import wasmUrl from "../public/froggers.wasm?url"` in `froggers-processor.ts`
- [x] 1.2 Eager worklet init on page load; `postScreen` on WASM `ready` (host page 0)
- [x] 1.3 Stop = `setRunning: false` + disconnect; keep AudioContext + worklet alive
- [x] 1.4 Main: apply labels/scopes from `screen` when stopped; page pills work without Play
- [x] 1.5 Verify `npm run dev` and `npm run build` — Play stays **Playing**, VCO audible

## 2. WASM scope range export

- [x] 2.1 Scope rings (96 samples × mod 4/5/6) in `WasmSimHost.hpp`
- [x] 2.2 Push mod levels in `processBlock`; attach to `screen` payload
- [x] 2.3 Export `froggers_copy_scope_samples` in `wasm/bindings.cpp` + `CMakeLists.txt` / `build.sh`

## 3. Mod bay CV scopes

- [x] 3.1 `web/src/CvScopeCanvas.ts` — Continuous + StepHold + idle
- [x] 3.2 Rewrite `renderModBay()` — three canvas scopes; remove `mod-meter-fill` bars
- [x] 3.3 CSS: scope cells ≥40 px; hint **CV trace while playing**

## 4. Rotary knobs

- [x] 4.1 `web/src/RotaryKnob.ts` — 44×44 px, vertical drag
- [x] 4.2 Replace range inputs in `main.ts`
- [x] 4.3 CSS: knob column fits eight rotaries at ≤720 px

## 5. Parameter labels

- [x] 5.1 Remove `Knob ${i+1}` hardcode; init empty or **—** until first `screen`
- [x] 5.2 `updateKnobLabels` from `screen` row names + **Mod depth** when patched
- [x] 5.3 All six pages match desktop strings (requires `delay-grain-filter-row0` labels)

## 6. Build + verify

- [x] 6.1 `npm run build:wasm`
- [x] 6.2 Browser manual quick start; scopes + labels while stopped; page change while stopped
