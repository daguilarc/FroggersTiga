## Context

```
ios-external-audio-routing (implemented)
──────────────────────────────────────
main.ts: isMobileWeb, applyMobileAudioSession, hints, External gating
index.html + manuals: copy

web/e2e/ (landed, not in CI)
────────────────────────────
Playwright: 9 tests — session spy, mocked getUserMedia, mobile/desktop profiles
  mobile-audio-routing.spec.ts (8)
  manual-mobile-routing.spec.ts (1 — sim-manual.md earpiece copy)

Gap
───
• No CI gate (and CI must build WASM — froggers.wasm not bundled by npm run build)
• Button/copy strings duplicated 7+ times in specs (OMNI repetition violation)
• No real-device Safari/Chrome validation
• Speaker vs earpiece not assertable in Playwright
```

## Goals / Non-Goals

**Goals:**

- Playwright CI on every web/sim/wasm PR (emsdk WASM build → preview → e2e)
- Single shared selector module (`web/test-shared/simSelectors.ts`) — OMNI: Playwright + Appium import one authority; no duplicated button name strings, subtitle text, or status hint fragments
- Appium smoke: real device opens sim URL, Play, External toggle, status hint visible on iOS
- Documented **routing checklist** for human operator during Appium session (earpiece / loudspeaker / headset)

**Non-Goals:**

- Automated proof of physical speaker routing in CI (platform limit)
- Appium in default PR CI (requires device farm / local hardware)
- Replacing Playwright with Appium for session-spy tests
- Full web browser E2E (page pills, knob drag, mod bay, help modal, External MIDI, Marbles) — audio routing verification only in v1

## Decisions

### D1 — Two-layer test pyramid

| Layer | Tool | Runs | Validates |
|-------|------|------|-----------|
| L1 | Playwright | CI every web/sim/wasm change | Session API sequences, subtitle, status hint, desktop guard, transport with mocked mic, manual doc sync |
| L2 | Appium | Manual / nightly with devices | Real Safari/Chrome UI, mic permission, hint on device; routing via checklist |

### D2 — Shared constants module

**Choice:** `web/test-shared/simSelectors.ts` exports:

| Export | Source of truth | Used by |
|--------|-----------------|---------|
| `PLAY_LABEL`, `STOP_LABEL` | `index.html` transport buttons | Playwright, Appium |
| `EXTERNAL_OFF_LABEL`, `EXTERNAL_ON_LABEL` | `main.ts` toggle text | Playwright, Appium |
| `STATUS_SELECTOR`, `SUBTITLE_SELECTOR` | `#status`, `.subtitle` | Playwright, Appium |
| `SUBTITLE_TEXT` | `index.html` subtitle copy | Playwright subtitle test |
| `STATUS_HINT_EXTERNAL_ON`, `STATUS_HINT_EARPIECE` | `main.ts` hint fragments | Playwright hint test |
| `PLAYING_STATUS_TEXT` | `"Playing"` wait target | `startSimAudio()` |

Playwright (`web/e2e/`) and Appium (`web/appium/`) both import from `../test-shared/simSelectors.ts`.

**OMNI repetition rule:** one authority for all copy used in assertions. E2e helpers (`MOBILE_USE`, session spy) stay Playwright-only — not duplicated in Appium.

**Helper extraction review (simTransport):** Playwright uses `page.getByRole`; Appium uses `$('~label')` / accessibility — tap paths differ. **Do not extract** a shared tap helper (trigger count <2 for identical logic). Share selectors only.

### D3 — Playwright (existing + CI)

**Choice:** Keep current `web/e2e/mobile-audio-routing.spec.ts`, `manual-mobile-routing.spec.ts`, and helpers. Add `.github/workflows/web-e2e.yml`:

```yaml
# Steps (mirror pages.yml WASM path):
- setup emsdk
- build WASM → web/public/froggers.wasm
- cd web && npm ci && npx playwright install chromium && npm run test:e2e
```

- Trigger on `web/**`, `wasm/**`, `sim/**` changes
- Playwright `webServer` already runs `npm run build && npm run preview` — WASM must exist **before** that command

No WebKit project in Playwright — mobile behavior via `MOBILE_USE` Chromium emulation (already proven).

### D4 — Appium stack

**Choice:** Appium 2 + WebdriverIO in `web/appium/`:

- `wdio.conf.ts` with capabilities for `Safari` (iOS) and `Chrome` (Android)
- `BASE_URL` env points at machine running `npm run preview -- --host 0.0.0.0` (device reaches host IP, not `127.0.0.1`)
- Specs mirror Playwright flows **without** session spy or getUserMedia mock
- **Package layout:** `web/appium/package.json` for WDIO deps only; specs import selectors via `import { ... } from '../test-shared/simSelectors.ts'` (sibling path, no duplicate npm package for selectors)

**Why WebdriverIO:** common Appium wrapper, TS support, parallel capability config.

**BASE_URL guard:** `before` hook probes `BASE_URL`; if unreachable, skip suite with logged message (defensive — real edge case when operator forgets preview or firewall blocks).

### D5 — Routing verification protocol (Appium)

**Choice:** Each Appium run appends a **Routing Checklist** markdown log the operator fills (`web/appium/routing-checklist-template.md`):

| Step | Action | Operator records | Maps to ios-external-audio-routing |
|------|--------|------------------|-------------------------------------|
| 1 | Play, no headset | loudspeaker / earpiece / silent | 4.1 |
| 2 | External on, no headset | loudspeaker / earpiece / silent; hint visible Y/N | 4.2 |
| 3 | External on, wired/BT headset | headphones / speaker / other | 4.3 |
| 4 | External off | loudspeaker restored Y/N | 4.4 |
| 5 | Android Chrome External on | no regression Y/N | 4.5 |

Automated Appium asserts: UI state only (`#status` contains hint fragments, buttons toggle). Routing columns are human — honest scope.

Optional future spike: iOS `AVAudioSession` route via native helper — out of v1.

### D6 — Relationship to `ios-external-audio-routing`

**Choice:** Implementation stays in `ios-external-audio-routing`. This change owns **verification infrastructure** only. Close routing change tasks 4.1–4.5 via Appium checklist + Playwright CI green.

**Split of 4.x ownership:**

| Task | Playwright | Appium checklist |
|------|------------|------------------|
| 4.1 Play → loudspeaker | session `playback` only | operator hears output route |
| 4.2 External → earpiece + hint | hint DOM text | operator hears earpiece |
| 4.3 External + headphones | — | operator hears headphones |
| 4.4 External off → speaker | session reset sequence | operator hears loudspeaker |
| 4.5 Android no regression | mobile emulation session | operator spot-check |

### D7 — OMNI audit notes (pre-apply)

| Check | Status | Notes |
|-------|--------|-------|
| Repetition | **Fix in task 1** | 7+ duplicate label strings in specs today |
| Helper extraction | **Compliant** | Selectors shared; tap/session helpers stay tool-specific |
| Nesting | **Compliant** | Appium page objects flat, max 3 levels |
| Defensive code | **Fix in task 3** | Appium skips when `BASE_URL` unreachable |
| CI data flow | **Fix in task 2** | WASM build before Playwright webServer |
| Accumulate then apply | **Compliant** | simSelectors is read-only constants, no loop mutation |
| Scope drift | **Contained** | v1 = audio routing infra; full web UI is explicit non-goal |

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| Appium flake on real devices | Manual/nightly only; retries=1 |
| Device cannot reach localhost preview | Document `BASE_URL=http://<host-ip>:4173`; preview `--host 0.0.0.0` |
| Duplicate test logic Playwright vs Appium | Share selectors + checklist, not full flow code |
| CI time from WASM + build + preview | Cache npm + emsdk; run only on `web/**`, `wasm/**`, `sim/**` |
| WASM build failure blocks all web PRs | Same as pages.yml — WASM is already required for deploy |
| Hint copy drift between main.ts and tests | simSelectors imports match main.ts fragments; single edit point |

## Migration Plan

1. Extract `web/test-shared/simSelectors.ts`; update Playwright imports
2. Add GitHub Actions Playwright workflow (emsdk + WASM + e2e)
3. Scaffold `web/appium/` with one iOS + one Android smoke spec
4. Add `web/TESTING.md` with Playwright + Appium runbooks
5. Execute Appium checklist once; attach log to close routing change 4.x

## Open Questions

1. Device farm (BrowserStack/Sauce) vs local-only Appium — default **local-only** in v1; env vars for cloud later
2. Full web UI E2E (knobs, pages, mod bay) — defer to separate change; note in TESTING.md as future matrix row
