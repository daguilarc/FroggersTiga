## Why

`web-sim-bootstrap-fix` restored labels and Play-first bootstrap, but the live UI still feels broken: a bordered empty strip between page chrome and knobs (`#mod-route-summary`), a 220 px black `#oled` void before and after stop, flat knobs with no module grouping, and **Play produces a split-second of audio then silence** with transport stuck (Play disabled, no recovery). Archived `web-sim-bootstrap-repair` §4 (compact OLED, no black void) was never verified. Layout debt and a transport state bug are separate from bootstrap and must be fixed together.

## What Changes

### Transport / audio continuity (blocks “Play once”)

- Main thread owns `audioRunning` for transport UI — **stop syncing from WASM `screen.audioRunning`** (race desyncs Play/Stop and mod-bay state).
- Add `transportIntentPlaying` — true after Play, false after Stop or worklet error; used by suspend recovery (D5).
- `syncTransportUi()` single path updates Play/Stop disabled state, status suffix, mod-bay idle.
- `audioContext.onstatechange`: if suspended while `transportIntentPlaying`, show “Click Play to resume” and re-enable Play.
- On worklet `error`, call `stopAudio()`, clear `transportIntentPlaying`, always re-enable Play — no stuck disabled Play.

### Dead UI chrome removal

- `#mod-route-summary`: **hidden when no mod routes** on current page (no empty bordered box, no “No mod routes” placeholder strip); **hidden on page load** before first screen tick.
- `#oled`: **collapsed when empty or stopped**; no `min-height: 220px` black rectangle on desktop when there is no OLED content.

### Knob column boundaries (not group meta-panels)

- Each `.knob-col` is a bordered vertical cell: title label, rotary knob, and mod-source dropdown — one neat box per parameter.
- **No** outer group meta-panels (no VCOs / Coupling / Output wrappers, no group titles, no `HOST_PAGE_GROUPS` DOM).
- Eight columns in one horizontal row; page chrome blurb carries module context.

### OLED behavior (finish archived compact layout)

- **Stopped / pre-Play:** OLED hidden or ≤48 px strip (wave buttons on Audio only when playing).
- **Playing, desktop (>720 px):** full eight-row OLED with values.
- **Playing, mobile (≤720 px):** compact strip (waves + badges only) — complete archived §4.4.

## Capabilities

### New Capabilities

- `web-transport-state-ownership`: Main-thread transport authority; no WASM screen overwrite; context suspend recovery.
- `web-knob-column-cells`: Bordered `.knob-col` per parameter; no group meta-panels.
- `web-oled-collapse`: No empty black OLED; compact mobile strip; visible only when useful.

### Modified Capabilities

- `web-mod-route-summary`: Hidden entirely when page has zero mod routes (no empty placeholder box).

## Impact

- `web/src/main.ts` — transport sync, hide route summary, OLED render gates, flat knob row
- `web/src/style.css` — `.knob-col` column cells, oled collapsed/compact, mod-route hidden
- Does not change WASM, processor logic (except stop posting `audioRunning` to screen for main-thread sync — optional cleanup)
- **Prerequisite for `web-ext-in-meter`:** main-thread `audioRunning` authority (Task §1)
