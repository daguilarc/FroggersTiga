## Why

The Audio page pair-sum A/R controls (Attack 1+2, Release 1+2, Attack 2+3, Release 2+3) were added in `audio-pair-ad-controls`, but randomize wiring was never completed. `AudioPairArState` exposes `randomizeMod()` yet has no `randomizeKnobs()`, and neither desktop nor WASM host paths invoke pair-AR randomization on page **Randomize** / **Randmod** or global **Rand All** — unlike the eight vertical rows and unlike Delay FX knobs. That is an OMNI violation: the same user actions must hit the same parameter sets on every host.

## What Changes

- Add `AudioPairArState::randomizeKnobs()` mirroring `DelayState::randomizeKnobs()` (four knobs, uniform 0–1)
- Centralize pair-AR randomize orchestration in one shared helper used by both `PagedHostIO` and `DesktopHostIO` (no copy-paste per platform)
- Wire pair-AR into the existing randomize matrix:
  - **Page Randomize** on Audio page (host page 0): page rows **and** pair-AR knobs
  - **Page Randmod** on Audio page: page row mod routes **and** pair-AR mod routes
  - **Rand All**: all page knobs **and** pair-AR knobs **and** Delay knobs (desktop + web/WASM)
  - **Rand Mods**: unchanged for pair-AR (global path already calls `randomizeMod()`); page Randmod gap fixed above
- Add a focused unit test asserting knob values change after each randomize entry point
- Update `GlobalStrip` tooltip if needed (“all pages + Delay knobs” → include pair-AR explicitly)
- No web TypeScript changes required if host layer is fixed (page Rand already calls `randomizePage`; global Rand All already calls WASM `randomizeAllIncludingDelay`)

## Capabilities

### New Capabilities

- `pair-ar-randomize`: Pair-AR knob and mod randomization wired into page-level and global randomize actions on desktop and WASM hosts, with shared orchestration and tests

### Modified Capabilities

- (none — baseline specs not archived on main)

## Impact

- `src/core/AudioPairArState.hpp` — new `randomizeKnobs()`
- New `sim/HostRandomize.hpp` (or equivalent) — shared page/global randomize orchestration for pair-AR
- `src/core/PagedHostIO.hpp` — delegate page/all randomize to shared helper
- `src/core/DesktopHostIO.hpp` — same helper in mutation `applyMutation` paths
- `sim/WasmSimHost.hpp` — global Rand All path picks up pair-AR via `PagedHostIO` / helper (no separate WASM-only hack)
- `desktop/Source/GlobalStrip.cpp` — tooltip text alignment
- Optional: `SIM_MANUAL.md` one-line note that Audio page Randomize includes pair-AR band
- Tests: new or extended sim unit test file
