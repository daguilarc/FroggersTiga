## Why

The browser sim is dead on arrival: status shows **Error: fetch is not defined**, Play stays disabled, and knob labels remain `—` because WASM never loads. `web-sim-core-fix` archived; it called `fetch()` inside `AudioWorkletGlobalScope`, where the API does not exist. Canonical web bootstrap spec: this change + `openspec/specs/sim-parameter-display-names`. Secondary UX debt stacks on top: duplicate mod-bay hints, a page blurb that does not name parameters, and a 220 px black OLED void that duplicates knob-column labels on mobile.

## What Changes

### Engine bootstrap (blocks everything)

- **Main-thread WASM fetch** — `main.ts` fetches `froggers.wasm` via Vite `?url` import; passes `ArrayBuffer` to worklet via `processorOptions.wasmBytes` before `AudioWorkletNode` construction.
- **Worklet synchronous instantiate** — `froggers-processor.ts` removes `fetch()`; `WebAssembly.instantiate(bytes)` in constructor from transferred bytes.
- **Bootstrap verification** — status line shows load progress; Play enables only after `ready` + first `screen`; regression test in manual checklist.

### Parameter labels (depends on bootstrap)

- **Remove hardcoded placeholders** — delete `i < 7 ? "—" : "Crunch"` init; labels empty until first `screen`, then `ParamDisplayNames` from WASM for all eight columns on every page.
- **Page chrome scope** — title stays `Audio (1/6)`; blurb becomes one optional sentence (module role), not a substitute for per-knob names.

### Mod bay hints (cleanup)

- **Single hint** — remove duplicate "CV level while playing" (HTML) vs "CV trace while playing" (JS); one line: **CV trace while playing** on the mod-bay toggle, not inside the scope grid.

### Mobile field layout (black void)

- **Knob column = primary label surface** — param name above each rotary is authoritative on all viewports.
- **OLED panel compact on mobile** — at ≤720 px: collapse OLED to a slim value strip (wave SVG buttons on Audio page only + mod badges); remove `min-height: 220px` black void when stopped.
- **Desktop width** — full OLED mock remains for Field parity.

## Capabilities

### New Capabilities

- `web-wasm-main-thread-load`: Main-thread fetch + `processorOptions` transfer; worklet sync instantiate; no `fetch` in AudioWorklet.
- `web-field-layout-compact`: Mobile-first knob column labels; compact OLED strip; page chrome without per-knob duplication.

### Modified Capabilities

- `web-mod-meter-visibility`: Single mod-bay hint text (from `web-chrome-cohesion`); remove nested duplicate hint in `#mod-bay`.
- `web-parameter-labels`: Labels from first `screen` only; no hardcoded `—` / `Crunch` init split.
- `web-wasm-audio-bootstrap`: WASM load path requirement updated — fetch on main thread, not worklet.

## Impact

- `web/src/main.ts` — fetch WASM, pass `processorOptions`, init label placeholders
- `web/src/froggers-processor.ts` — remove async `loadWasm` fetch; sync instantiate from bytes
- `web/index.html` — mod-bay hint dedup
- `web/src/style.css` — compact OLED @720px, layout gap fixes
- Supersedes incomplete verification from `web-sim-core-fix` tasks 1.5 / 6.2
- No WASM/C++ changes required
