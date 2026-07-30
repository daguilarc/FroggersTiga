## 1. Shared test constants

- [x] 1.1 Add `web/test-shared/simSelectors.ts` — button labels (`Play`, `Stop`, `External Audio: Off/On`), element selectors (`#status`, `.subtitle`), copy constants (`SUBTITLE_TEXT`, hint fragments `external on` / `without headphones, iOS may use the earpiece`, `PLAYING_STATUS_TEXT`)
- [x] 1.2 Refactor `web/e2e/helpers.ts` and both specs to import from `test-shared/simSelectors.ts` (eliminate 7+ duplicate label strings)

## 2. Playwright CI

- [x] 2.1 Add `.github/workflows/web-e2e.yml` — path filter `web/**`, `wasm/**`, `sim/**`; setup emsdk; build WASM to `web/public/froggers.wasm`; then `cd web && npm ci && npx playwright install chromium && npm run test:e2e`
- [x] 2.2 Document local run (including `npm run build:wasm` or `build:all` prerequisite) in `web/TESTING.md`

## 3. Cross-change manual device verification

- [ ] 3.1 Manual device verification for `ios-external-audio-routing` tasks 4.1–4.5 (preview on LAN + phone browser; documented in `web/TESTING.md`)

## 4. Verification

- [x] 4.1 Playwright CI green — 15 tests across `mobile-audio-routing`, `manual-mobile-routing`, `global-strip-placement`, and `mobile-knob-labels` (requires WASM build)
- [ ] 4.2 `ios-external-audio-routing` 4.1–4.5 remain open until manual phone test (LAN preview protocol; not gated on Appium)

> **Reconciled (omni 1.3):** Playwright + shared selectors + CI retained. Unimplemented Appium scaffold, runbook, and struck-through tasks removed. Physical routing stays manual-only.
