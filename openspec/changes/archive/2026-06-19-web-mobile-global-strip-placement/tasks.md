> **Reconciled (omni 1.2):** Complete (11/11); `global-strip-placement.spec.ts` in CI.

## 1. HTML structure (single strip)

- [x] 1.1 Add `.transport-play` wrapper for Play/Stop inside `.controls-top`
- [x] 1.2 Add `.transport-io` wrapping `.external-controls` + `.global-strip` (move strip from bottom of `#app`)
- [x] 1.3 Change `.global-strip` to `<nav aria-label="Global randomize">`

## 2. CSS — placement (all viewports)

- [x] 2.1 `.transport-play { display: flex; gap: 1rem; align-items: center }`
- [x] 2.2 `.transport-io { display: flex; flex-direction: column; gap: 0.35rem }`
- [x] 2.3 `.global-strip { margin-top: 0 }` — remove bottom-only spacing
- [x] 2.4 Remove desktop-only `.global-strip { justify-content: center }` at bottom
- [x] 2.5 Preserve `min-height: 44px` on strip buttons

## 3. Verification

- [x] 3.1 Add `web/e2e/global-strip-placement.spec.ts` — mobile + desktop, strip above `#mod-bay` and below `#external-midi-btn`
- [x] 3.2 Run `npm run test:e2e` — all pass
- [x] 3.3 Manual: Rand All reachable without scrolling past knobs on mobile and desktop
