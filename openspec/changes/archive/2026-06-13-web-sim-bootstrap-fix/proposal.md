## Why

`web-sim-wasm-repair` fixed WASM URL and exports, but the web sim still fails every browser checklist item in dev: blank knob labels, disabled Play, dead Randomize. Root cause: `froggers-processor.ts?worker&url` makes Vite inject HMR client code into the AudioWorklet module — `addModule()` fails in dev. Secondary cause: page-load async bootstrap gates the entire UI; labels stay empty until a `screen` message that never arrives when bootstrap fails. [thenoriegas.info](https://thenoriegas.info) works because labels render synchronously and audio starts on Play click.

## What Changes

### AudioWorklet module path (blocks dev)

- Replace `import processorUrl from "./froggers-processor.ts?worker&url"` with `?url` so Vite serves a clean module without `vite/dist/client/env.mjs`.
- Update `vite-env.d.ts` module declaration accordingly.

### Bootstrap on Play (user gesture)

- Remove page-load `void initWorklet()` — defer WASM fetch + worklet creation to **Play** click (first click also resumes AudioContext).
- Status line: idle → "Click Play to start" (labels already visible) → "Loading engine..." during bootstrap → "Playing — …" on success.
- Keep `initWorklet()` reusable for External click (call only when worklet missing).

### Static knob labels (never blank)

- Seed knob column labels from a single `HOST_PAGE_LABELS` table in `main.ts` that mirrors `ParamDisplayNames.hpp` for all six pages × eight rows.
- Local page navigation (pills, prev/next, swipe, keyboard) updates `hostPage`, chrome, and static labels before WASM exists; WASM sync on Play when worklet is live.
- WASM `screen` messages update values/OLED/mod state; labels refresh only when page changes or mod routing switches a column to "Mod depth".
- No `—` placeholders after first paint.

### Mobile audio unlock

- Add minimal touchstart resume pattern (from thenoriegas.info) so iOS Safari unlocks AudioContext on first user interaction before Play bootstrap.

### GitHub Pages base path

- CI Pages workflow: `VITE_BASE=/FroggersTiga/ npm run build` so `import.meta.env.BASE_URL` resolves wasm and assets on project pages.

## Capabilities

### New Capabilities

- `web-worklet-module-load`: AudioWorklet processor URL must not use Vite worker wrapper in dev or production.
- `web-bootstrap-on-play`: Engine bootstrap runs on Play click, not page load; clear status progression.
- `web-static-knob-labels`: Knob labels visible immediately from sim display-name table; WASM screen updates state not primary naming.
- `web-mobile-audio-unlock`: Touch/gesture AudioContext resume before first Play bootstrap on mobile Safari.

### Modified Capabilities

- `sim-parameter-display-names`: Web labels SHALL be visible before Play from static sim display names; WASM `screen` confirms values and mod routing, not initial label paint.

## Impact

- `web/src/main.ts` — processor import, remove page-load init, Play-first bootstrap, static labels, mobile unlock
- `web/src/vite-env.d.ts` — `?url` declaration for processor
- `sim/ParamDisplayNames.hpp` — reference only; grep task verifies `HOST_PAGE_LABELS` matches
- `.github/workflows/pages.yml` — set `VITE_BASE` for project pages deploy
- Complements `web-sim-wasm-repair` (WASM URL/exports/external UX already applied); does not duplicate those fixes
