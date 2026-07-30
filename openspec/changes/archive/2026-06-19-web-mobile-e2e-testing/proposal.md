## Why

The `ios-external-audio-routing` web implementation (mobile Audio Session lifecycle, External mic gating, status hints, subtitle/manual copy) needs **two verification layers**:

1. **Playwright** — fast CI checks for everything automatable in Chromium (session sequences, copy, desktop no-op guard, transport + mocked `getUserMedia`)
2. **Appium** — real Safari/Chrome on physical phones for flows Playwright cannot validate (actual OS audio routing, permission prompts, headset vs built-in output)

Playwright tests already exist under `web/e2e/` from `ios-external-audio-routing` (tasks 5.1–5.4): **9 tests** in two files — `mobile-audio-routing.spec.ts` (8) and `manual-mobile-routing.spec.ts` (1 doc-sync). This change **does not re-implement** them — it adds CI (including WASM build), shared selectors, Appium, and the routing checklist.

## What Changes

- **Formalize Playwright** as the default web e2e gate: document scope, add CI workflow with **emsdk + WASM build** (tests wait for `"Playing"` — engine requires `froggers.wasm`), shared selector module at `web/test-shared/simSelectors.ts`
- **Add Appium** project under `web/appium/` for iOS Safari + Android Chrome smoke flows against a preview URL
- **Split assertions**: Playwright owns session spy + DOM copy; Appium owns real-browser UI flows + documented human/audio checklist for earpiece vs loudspeaker (no false claim of fully automated speaker detection)
- **Cross-link** `ios-external-audio-routing` manual verification tasks 4.1–4.5 to Appium session checklist
- **Non-goals:** Desktop app tests, WASM unit tests, guaranteed automated earpiece detection without human or native audio tap spike, **full web UI coverage** (page nav, knobs, mod bay, help modal, External MIDI — separate future change if needed)

## Capabilities

### New Capabilities

- `web-playwright-e2e`: Playwright config, helpers, CI (WASM + preview + e2e), coverage matrix for mobile audio routing + web transport
- `web-appium-device-smoke`: Appium real-device smoke flows and routing verification checklist

### Modified Capabilities

- (none in `openspec/specs/`)

## Impact

- `web/test-shared/simSelectors.ts` — single UI label + copy fragment authority (buttons, subtitle, status hint substrings, element ids)
- `web/e2e/` — refactor imports; Playwright CI
- `web/appium/` — Appium/WebdriverIO config + specs (imports selectors via relative path from sibling `test-shared/`)
- `.github/workflows/web-e2e.yml` — Playwright job on `web/**`, `wasm/**`, `sim/**` changes; emsdk WASM build before preview
- `openspec/changes/ios-external-audio-routing/tasks.md` — reference this change for tasks 4.1–4.5 execution protocol

## OMNI Audit Summary (pre-apply)

| Check | Finding | Resolution in this change |
|-------|---------|---------------------------|
| Repetition | Button labels, subtitle, hint fragments duplicated across `helpers.ts` + `mobile-audio-routing.spec.ts` | Task 1.1 extracts all to `simSelectors.ts` |
| CI data flow | `npm run build` does not produce WASM; `startSimAudio()` waits 45s for `"Playing"` | CI workflow builds WASM first (mirror `pages.yml`) |
| Path filter | `web/**` only misses WASM/sim changes that break engine | Trigger on `web/**`, `wasm/**`, `sim/**` |
| Helper extraction | Tap paths differ Playwright vs Appium | Share selectors only; no shared tap helper |
| Scope honesty | Playwright cannot assert physical speaker routing | Appium checklist for 4.1–4.5; automation asserts DOM only |
| Defensive code | Appium on unreachable `BASE_URL` wastes operator time | Skip with clear message when preview unreachable |
