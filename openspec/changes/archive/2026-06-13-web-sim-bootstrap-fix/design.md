## Context

Current bootstrap data flow (broken in dev):

```
page load
    │
    ▼
initWorklet() ──fetch wasm──▶ compile ──addModule(?worker&url)──▶ FAIL (Vite HMR import)
    │                                                                  │
    │                                                                  ▼
    │                                                         no "ready" / "screen"
    ▼
blank labels, Play disabled, status "Engine error" or stuck "Loading..."
```

Working reference ([thenoriegas.info](https://thenoriegas.info)):

```
page load ──▶ renderLFOs() ──▶ labels visible immediately
Play click ──▶ new AudioContext() ──▶ audio + UI active
```

`web-sim-wasm-repair` already fixed WASM public URL, export parity, and external permission UX. This change fixes the **worklet loader** and **UI gating** that make the sim appear totally dead.

## Goals / Non-Goals

**Goals:**

- `npm run dev`: Play works; labels visible before Play; no console `addModule` failure
- `npm run preview` and GitHub Pages: same behavior
- Single source for display names: `HOST_PAGE_LABELS` in `main.ts` mirrors `ParamDisplayNames.hpp` (grep-verified)
- Bootstrap on Play provides user gesture for AudioContext + mobile Safari
- Status line reflects each phase clearly

**Non-Goals:**

- Rewriting WASM engine or processor logic
- Desktop changes
- Replacing RotaryKnob / mobile layout (separate chrome work)
- Duplicating wasm-repair export verify or external permission flows

## Decisions

### D1: Processor import uses `?url`, not `?worker&url`

**Choice:** `import processorUrl from "./froggers-processor.ts?url"`

**Why:** `?worker&url` resolves to `?worker_file&type=module` with `import "/node_modules/vite/dist/client/env.mjs"` prepended. AudioWorkletGlobalScope cannot load Vite HMR. Plain `?url` serves `/src/froggers-processor.ts` clean in dev; production build already bundles to `/assets/froggers-processor-*.js`.

**Alternative rejected:** Keep `?worker&url` — dev permanently broken.

**Alternative rejected:** Blob URL loader (thenoriegas pattern) — works but duplicates Vite bundling; `?url` is idiomatic for AudioWorklet in Vite.

### D2: Bootstrap on Play click, not page load

**Choice:** Remove `void initWorklet()` at bottom of `main.ts`. `startAudio()` calls `initWorklet()` if `!workletNode`. First Play click: resume context + fetch wasm + addModule + connect.

**Why:** User gesture unlocks audio on iOS; UI is not blocked on failed background bootstrap; matches thenoriegas Play-first pattern.

**External click:** `setExternalEnabled(true)` still calls `await initWorklet()` when `!workletNode` (existing wasm-repair contract).

### D3d: Pre-Play Randomize gated

**Choice:** Disable page Randomize buttons until `engineReady`. If clicked before ready, set status "Click Play first" — do not bare `send()` to a missing worklet.

**Why:** Randomize requires WASM exports; pre-Play `send()` is a no-op like page pills were before D3b.

### D3: Static labels from one table, WASM updates state

**Choice:** Define `HOST_PAGE_LABELS: string[][]` in `main.ts` — six pages × eight rows, copied from `ParamDisplayNames.hpp`. On DOM build and page change, call `applyStaticKnobLabels(hostPage)`. `onScreenUpdate` updates values/OLED/mod; `updateKnobLabels` only swaps to "Mod depth" when mod active.

**Why:** Labels never blank; OMNI single-source table (one array, loop on page change — no per-page copy-paste blocks). WASM remains authoritative for values and mod routing.

**Verification:** Task greps `HOST_PAGE_LABELS` against `ParamDisplayNames.hpp` strings.

### D3b: Local page navigation before WASM exists

**Choice:** Add `setHostPage(n)` and `changeHostPage(delta)` that update local `hostPage`, call `renderPageChrome()` + `applyStaticKnobLabels(hostPage)`, and post `hostPage` / `hostPageDelta` to the worklet only when `workletNode` exists. Wire pills, prev/next, swipe, and `[`/`]` to these helpers instead of bare `send()`.

**Why:** With bootstrap deferred to Play, `send()` is a no-op until the worklet exists. Without local page state, pre-Play navigation leaves labels stuck on Audio and page pills appear dead — the "basic buttons" regression.

**Alternative rejected:** Keep WASM-only page state — requires bootstrap before any navigation; contradicts Play-first UX.

### D4: Minimal mobile audio unlock

**Choice:** One `document.addEventListener("touchstart", resumeCtx, { once: true, passive: true })` that calls `audioContext?.resume()` if context exists and is suspended. No full unmute.js port.

**Why:** Play click is the primary user gesture for AudioContext creation and resume on iOS. Touchstart only helps when External-before-Play creates a suspended context; it is optional belt-and-suspenders, not the main unlock path.

### D3c: Bootstrap failure recovery

**Choice:** If `initWorklet()` or first Play bootstrap throws, re-enable Play and show a retry status. Do not leave Play disabled when `engineReady` is still false.

**Why:** Current code disables Play during bootstrap and never re-enables on failure — user is stuck after one bad load.

### D5: GitHub Pages `VITE_BASE`

**Choice:** In `.github/workflows/pages.yml`, build step: `VITE_BASE=/FroggersTiga/ npm run build` (adjust repo name if fork differs).

**Why:** Project pages serve from `/RepoName/`; default `base: "/"` 404s wasm on Pages.

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| `HOST_PAGE_LABELS` drifts from C++ header | Grep verify task; same strings as SIM_MANUAL / Quick Dict |
| Label table duplicates header | Acceptable: TS cannot include `.hpp`; one 6×8 table, not scattered literals |
| Play click slower first time (wasm fetch) | Status shows "Loading engine..."; subsequent Play is instant |
| External before Play still needs initWorklet | Existing wasm-repair flow preserved |
| Pre-Play page pills no-op without local nav | D3b: local `hostPage` + static label refresh |
| Pre-Play Randomize no-op | D3d: disable until `engineReady` |
| Bootstrap failure leaves Play disabled | D3c: re-enable Play on error |

## Migration Plan

1. Fix processor import + vite-env.d.ts
2. Add `HOST_PAGE_LABELS`, paint labels on init; wire local page navigation (D3b)
3. Move bootstrap to Play; remove page-load initWorklet; add failure recovery (D3c)
4. Add touchstart unlock
5. Update Pages CI with VITE_BASE
6. Browser checklist (dev + preview)
7. Archive `web-sim-bootstrap-fix`; mark wasm-repair §5 browser tasks done

## Open Questions

None blocking.
