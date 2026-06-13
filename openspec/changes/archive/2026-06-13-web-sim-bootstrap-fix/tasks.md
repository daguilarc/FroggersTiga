## 1. AudioWorklet module path

- [x] 1.1 `web/src/main.ts`: change processor import to `./froggers-processor.ts?url`
- [x] 1.2 `web/src/vite-env.d.ts`: declare `*?url` for processor (remove or keep `?worker&url` only if unused elsewhere)
- [x] 1.3 `npm run dev` + Play: no `addModule` error; no vite client import in worklet module

## 2. Static knob labels

- [x] 2.1 Add `HOST_PAGE_LABELS` (6×8) in `main.ts` matching `ParamDisplayNames.hpp`
- [x] 2.2 `applyStaticKnobLabels(hostPage)`: one loop sets eight `knobMainLabels` on init and page change; `updateKnobLabels` (WASM `screen`) only overlays **Mod depth** when mod active — static names remain the base layer
- [x] 2.3 Grep verify: Comb offset, Stereo width, Diffusion, XOR, Bit depth, Crunch present in table
- [x] 2.4 Before Play: Audio page shows VCO1…Crunch; Filter row 0 shows Comb offset
- [x] 2.5 Add `setHostPage(n)` / `changeHostPage(±1)`: update local `hostPage`, `renderPageChrome()`, `applyStaticKnobLabels(hostPage)`; call `send({ type: "hostPage" | "hostPageDelta" })` only when `workletNode` exists
- [x] 2.6 Wire pills, prev/next, swipe, and `[`/`]` keyboard to local page helpers (not bare `send()`)
- [x] 2.7 After knob DOM build, call `applyStaticKnobLabels(0)` once before any async bootstrap
- [x] 2.8 Pre-Play Randomize: disable `page-rand-knobs` / `page-rand-mod` until `engineReady`; on click before ready, set status "Click Play first" (do not bare `send()`)

## 3. Bootstrap on Play

- [x] 3.1 Remove page-load `void initWorklet()` and its `.catch()` handler at bottom of `main.ts` (no async bootstrap on load)
- [x] 3.2 Idle status: "Click Play to start" on load (`index.html` `#status` or first paint in JS)
- [x] 3.3 `startAudio()`: show "Loading engine..." during first bootstrap; "Playing — …" on success
- [x] 3.4 `engineReady` / Play disabled logic: enable Play on load (labels visible); disable only while bootstrap or playing
- [x] 3.5 External click still calls `initWorklet()` when `!workletNode` (preserve wasm-repair flow)
- [x] 3.6 On bootstrap failure: re-enable Play, show retry status (do not leave Play disabled when `engineReady` is false)

## 4. Mobile audio unlock

- [x] 4.1 One-time `touchstart` listener resumes suspended `audioContext` if present

## 5. GitHub Pages base path

- [x] 5.1 `.github/workflows/pages.yml`: `VITE_BASE=/FroggersTiga/ npm run build` (or repo-correct path)
- [x] 5.2 Verify built `index.html` asset paths use `/FroggersTiga/` prefix

## 6. Browser verification (required before archive)

- [ ] 6.1 `npm run dev`: labels visible before Play; Play → audible VCO; Stop → silent
- [ ] 6.2 Page Randomize on Audio changes knobs without error (requires wasm-repair exports)
- [ ] 6.3 Filter page row 0 **Comb offset** before Play (via pill nav, no WASM) and after Play
- [ ] 6.7 Pre-Play pill nav: chrome title + all eight labels update without `screen` message
- [ ] 6.8 Pre-Play Randomize: buttons disabled or status "Click Play first"; after Play, Randomize works
- [ ] 6.4 `npm run build && npm run preview`: same behavior as dev
- [ ] 6.5 External on → mic prompt → grant → ring mod (pessimistic UI from wasm-repair)
- [ ] 6.6 Mark `web-sim-wasm-repair` tasks §5.2–5.9 complete if verified here
