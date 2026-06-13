## Why

The web sim correctly uses **one page at a time** (Field-style paging) for mobile — six host pages with prev/next arrows and 8 knob columns. That constraint stays. What's wrong is everything *around* the paging: mod assignment is a bare dropdown with no route visibility, **Randomize mod** is global-only (no per-page actions), wave controls show opaque `SIN`/`SAW`/`SQR` text, the mod bay is a static meter row with no connection to the current page, and Delay parameters (DTIM, DSND, DFBK, …) have no contextual help. Desktop got patch cables + per-panel randomize; web needs **parity of intent** through dropdown + summary UX, not cables.

Depends on core sim randomize fixes from `desktop-host-mutation-safety` (WASM `RandomizeModSim` path).

**Hard constraint:** Web mod assignment stays **dropdown-only** (`<select>` per knob). Route summary, pills, and chrome are read-only or scoped actions — never drag-to-connect. Patch cables remain desktop-only unless a future spike proves reliable touch patching.

## What Changes

- **Page chrome** — each host page gets a header: name, 1-line role blurb, **Randomize** + **Randomize mod** scoped to current page (including Delay).
- **Bottom page navigator** — six tappable pills/dots (44×44 px min) supplement ◀ ▶ for thumb reach; preserves separate pages.
- **Mod route summary** — below mod bay or in page chrome: list active mod assignments on **current page only** (e.g. `V1VO ← Marbles 1 · 62%`). No patch cables on web v2.1.
- **Mod column UX** — label stack: param name → slider → `Mod source` select → when source ≠ None, slider label becomes **Mod depth** and OLED shows mod badge.
- **Wave controls** — replace text badges with inline SVG icons (sine/saw/square bands); same three-band logic as desktop target.
- **Delay page identity** — accent styling, inline param hints (DTIM = time, DSND = send, DFBK = feedback, DWID = stereo width, DTON = tone, DMOD = mod rate, DMIX = mix, FUEG = fuegoizer).
- **Mod bay mobile** — collapsible on viewports ≤720 px; expanded by default on desktop-width.
- **Swipe paging** — horizontal swipe on knob area changes host page (optional supplement to arrows).
- **Global strip slimmed** — keep **Randomize all**, **Randomize mod (all)**, **Marbles**, **Randomize waves**; per-page randomize moves to page chrome.

## Capabilities

### New Capabilities

- `web-page-chrome`: Per-page header, scoped randomize actions, Delay param hints.
- `web-mod-route-summary`: Read-only list of current-page mod routes (dropdown model, not cables).
- `web-mobile-navigation`: Bottom page pills + optional swipe; 44 px touch targets preserved.

### Modified Capabilities

- `host-ui-delay-page`: Web Delay page presents full param semantics and per-page randomize mod (sources + depths after core fix).
- `sim-mod-patchbay`: Web mod dropdown UX — depth labeling, route summary, sim-valid set only.

## Impact

- `web/index.html` — page chrome shell, bottom nav, collapsible mod bay
- `web/src/main.ts` — page chrome, route summary, SVG wave buttons, swipe handler
- `web/src/style.css` — mobile layout, Delay accent, pill nav
- `web/src/froggers-processor.ts` — wire per-page randomize messages (`randomizePage`, `randomizePageMod`, `delayRandomizeMod`)
- `wasm/bindings.cpp` — exports for per-page randomize if missing
- Depends on `desktop-host-mutation-safety` WASM sim randomize for global/per-page mod pickers

---

**Superseded (global strip labels):** `web-chrome-cohesion` — **Rand All**, **Rand Mods**, **Rand waves** (page chrome keeps **Randomize** / **Randomize mod**).
