# Web testing

## Playwright (CI + local)

Playwright runs in Chromium with mobile/desktop profiles. It validates session API sequences, UX copy, and desktop no-op guards — not physical speaker routing.

### Prerequisites

WASM must exist before e2e (`startSimAudio` waits for `"Playing"`):

```bash
cd web && npm run build:wasm   # or npm run build:all
```

### Local run

```bash
cd web
npm ci
npx playwright install chromium
npm run test:e2e
```

Playwright starts its own preview server (`build` + `preview` on port 4173). For manual browser testing on a phone:

```bash
cd web && npm run build:all && npm run preview -- --host 0.0.0.0 --port 4173
```

Open `http://<your-lan-ip>:4173` on the device. Physical audio routing (earpiece vs loudspeaker) requires manual verification — see `ios-external-audio-routing` tasks 4.1–4.5.

## Shared selectors

Button labels and copy fragments live in `web/test-shared/simSelectors.ts`. Playwright imports from there.
