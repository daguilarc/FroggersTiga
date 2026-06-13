## Why

Sim hosts (desktop six-panel rack and web paged UI) still show **4-character firmware OLED labels** (`RVMX`, `COMF`, `FUEG`, …) in parameter columns. At current font sizes the columns have room for **readable full titles**. Users also want the fuegoizer branded **Crunch** in sim UI — not “Fuegoizer” or `FUEG`. A single sim-side naming dictionary keeps desktop, web, WASM screen rows, and Quick Dict aligned without touching Daisy Field OLED strings.

## What Changes

- Add **`sim/ParamDisplayNames`** — authoritative map `(page, row) → display string` for all six sim pages + Delay overlay.
- **Desktop** `SubModulePanel` row labels use display names (via `getRowName()`).
- **Web** knob columns and OLED mock use the same names (WASM screen payload or TS map fed from shared table).
- **Knob 8** on every page: label **Crunch** (not `FUEG` / Fuegoizer).
- **Quick Dict** rewritten: left token = sim display name; short gloss on the right (`Cross-coupler : CCW 1→2, CW 2→3 from noon`).
- **Unchanged:** firmware `Parameter` 4-char names (`V1VO`, `FUEG`, …), `MANUAL.md` Field tables, patch cable IDs, core `InitParam` strings.

## Capabilities

### New Capabilities

- `sim-parameter-display-names`: Full column titles per module; Crunch for fuegoizer row.

### Modified Capabilities

- `quick-dict-format`: Left token SHALL be full sim display name (not 4-char PRMT); Crunch replaces Fuegoizer entries.

## Impact

- `sim/ParamDisplayNames.hpp` (new)
- `desktop/Source/PanelBackend.hpp`, `sim/DelayState.hpp` (display lookup)
- `wasm/bindings.cpp` (screen row names)
- `web/src/main.ts` (remove duplicate hints where label is sufficient; DELAY_HINTS optional trim)
- `QUICK_DICT.md`, `web/public/quick-dict.md` (via sync script)
- `SubModulePanel.cpp` — label min width if truncation appears at 1680×720
