> **Reconciled (omni 1.2):** Code-backed + Playwright; tasks 4.1–4.5 remain open for real-device routing.

## 1. Web — mobile audio session helper (desktop no-op)

- [x] 1.1 Add `isMobileWeb()` in `web/src/main.ts` (720px breakpoint + mobile UA; matches existing `mobileMic` scope)
- [x] 1.2 Add `applyMobileAudioSession(mode: 'playback' | 'reset' | 'externalOn' | 'externalOff')` — return immediately when `!isMobileWeb()`; guard `'audioSession' in navigator`
- [x] 1.3 Call `'playback'` from `startAudio` when External off; `'reset'` / `'externalOn'` in `setExternalEnabled(true)`; `'externalOff'` in `disconnectExternalStream()`

## 2. Web — UX copy

- [x] 2.1 Extend `applyPlayingStatus()` with mobile built-in-vs-headset hint when `isMobileWeb() && externalEnabled && audioRunning`
- [x] 2.2 Update subtitle in `web/index.html` per design D5

## 3. Docs sync

- [x] 3.1 Add mobile External routing paragraph to `SIM_MANUAL.md` Web section (built-in earpiece vs headset; desktop unaffected)
- [x] 3.2 Copy to `docs/sim-manual.md` and `web/public/sim-manual.md`

## 4. Verification

- [x] 4.6 Desktop Chrome/Safari wide viewport: confirm no `audioSession` mutations (Playwright `desktop audio session guard`)
- [ ] 4.1 iPhone Safari: Play only, no headset → bottom speaker (manual — Playwright covers session only)
- [ ] 4.2 iPhone Safari: External on, no headset → earpiece likely; hint visible (Playwright covers hint + session)
- [ ] 4.3 iPhone Safari: External on, headphones → audio in headphones (manual device)
- [ ] 4.4 iPhone Safari: External off after on → built-in speaker without reload (Playwright covers session reset)
- [ ] 4.5 Android Chrome: External on → no regression (Playwright mobile emulation covers session; manual spot-check)

## 5. Playwright e2e

- [x] 5.1 Add `@playwright/test`, config, `npm run test:e2e`
- [x] 5.2 Mobile session lifecycle tests (playback / reset / externalOn / externalOff)
- [x] 5.3 Desktop no-op guard tests
- [x] 5.4 Subtitle, status hint, manual copy tests
