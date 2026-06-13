## Context

```text
main.ts ↔ froggers-processor.ts ↔ froggers.wasm ↔ PagedHostIO + DelayState
```

**Web failures:** WASM 404 at worklet-relative path; labels need worklet `screen` before audio; mod bay uses bars; knobs are sliders; Stop destroys worklet so page pills no-op.

Delay **Detune** / Filter **Comb line** / 2 s cap: **`delay-grain-filter-row0`** (apply first).

## Goals / Non-Goals

**Goals:**

- Play works in dev, build, and GitHub Pages.
- Knob labels from WASM `ParamDisplayNames` before first audio output.
- Page changes work while transport stopped (worklet + WASM stay alive).
- Web knob labels, rotaries, mod scopes match desktop intent.
- OMNI: one `CvScopeCanvas`, one `RotaryKnob`; scope rings in `WasmSimHost`; consume once per `postScreen`.

**Non-Goals:**

- Patch cables, MIDI, RECORD on web.
- StereoDelay detune or Filter display names (other change).

## Decisions

### 0. Worklet lifecycle (blocks label + stopped-page specs)

**Eager init on page load:**

1. `main.ts` creates `AudioContext` + `AudioWorkletNode` on `DOMContentLoaded` (or first paint).
2. Worklet loads WASM in constructor; on success posts `ready` then `screen` (host page 0).
3. UI applies labels/scopes from `screen` while `audioRunning === false`.

**Play / Stop:**

- **Play:** `audioContext.resume()`, `setRunning: true`, connect worklet to destination (existing silent-gain trick).
- **Stop:** `setRunning: false`, disconnect worklet from destination, stop external mic — **do not** `close()` AudioContext or null worklet.

Page pills and randomize work anytime WASM is loaded.

### 1. WASM URL

`import wasmUrl from "../public/froggers.wasm?url"` in `froggers-processor.ts`; `fetch(wasmUrl)`.

### 2. Bootstrap screen

On WASM `ready` inside worklet: `setHostPage(0)` + `postScreen()` before main sends `setRunning`.

Main `ready` handler: wait for first `screen`, then enable Play; do not require audio running for labels.

### 3. Scope rings (`WasmSimHost`)

- `static constexpr size_t kScopeSize = 96` (match desktop `CvScopeDisplay`).
- Three rings for mod indices 4, 5, 6.
- `processBlock`: after each `io.ProcessBlock` sample (or per-block subsample at screen rate), push `froggers_mod_level` into rings.
- `froggers_consume_mod_scope_range(host, modIndex, outPtr, maxCount)` copies and clears readable span for UI.

`postScreen` payload includes `scopeSamples: { vco: number[96], m1: number[96], m2: number[96] }` OR main calls consume export after screen — prefer **payload on screen** to avoid extra WASM round-trip.

### 4. `CvScopeCanvas.ts`

One class: Continuous mode (VCO), StepHold mode (Marbles). Idle = dimmed last level. Min height 40 px.

### 5. `RotaryKnob.ts`

44×44 px, vertical drag, `input` event with 0–1 value. Replace `<input type="range">` in `main.ts`.

### 6. Labels

Remove `Knob ${i+1}` init text; `updateSliderLabels` (rename to `updateKnobLabels`) sets `row.name` from every `screen`. Patched rows → **Mod depth** on column label only.

### 7. Manual

Browser quick-start paragraph in `web/public/manual.md`.

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| Eager AudioContext autoplay policy | Context starts suspended; labels work without resume |
| Scope payload size | 96×3×4 B ≈ 1.1 KB per screen @ ~15 Hz |
| Stop leaves context open | Acceptable for sim tab |

## Migration Plan

1. Worklet lifecycle + WASM URL (unblocks labels + Play).
2. Scope rings + export + mod bay canvases.
3. Rotary knobs.
4. `npm run build:wasm`; verify labels before Play, scopes while playing, page change while stopped.
