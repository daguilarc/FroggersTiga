## Why

The browser sim is broken for first-time users: Play tears down immediately (WASM fetched from wrong URL vs worklet bundle). No `screen` messages → knobs stay **Knob 1…7**, mod bay shows 6 px bars not scopes, controls are vertical sliders not rotaries.

`sim-parameter-full-names` added `ParamDisplayNames` but web task 5.2 was never finished. Delay **Detune** / Filter **Comb line** / 2 s cap live in **`delay-grain-filter-row0`** (apply before or with this change).

## What Changes

### Web sim (browser)

- **WASM load path** — Vite `?url` import for `froggers.wasm`; clear error on failure.
- **UI bootstrap without audio** — `screen` on WASM `ready` before `setRunning`; page changes while stopped update labels.
- **Per-knob labels** — remove `Knob N` placeholders; `ParamDisplayNames` on every column + OLED.
- **Touch rotary knobs** — replace `<input type="range">` (44×44 px min).
- **Mod bay CV scopes** — canvas traces (VCO continuous, Marbles step-hold); WASM `froggers_consume_mod_scope_range`.
- **Browser quick start** in `web/public/manual.md`.

**Unchanged:** Delay/Filter DSP and display names (see `delay-grain-filter-row0`).

## Capabilities

### New Capabilities

- `web-wasm-audio-bootstrap`: WASM URL, Play/Stop, initial screen before audio.
- `web-rotary-knobs`: Touch rotary controls.
- `web-mod-cv-scopes`: Rectangular CV traces in mod bay.
- `web-parameter-labels`: Per-knob `ParamDisplayNames` including before Play.
## Impact

- `web/src/` — main, processor, RotaryKnob, CvScopeCanvas, style
- `wasm/bindings.cpp` — scope range export + rebuild
- `web/public/manual.md`
