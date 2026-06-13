## Why

Audio row 7 on desktop and web is labeled **VCO Envelope** but drives firmware `OLVL` — a silent-fallback oscillator level for when external input is absent. Daisy Field almost always has external in; sim defaults **External off**, so users get a misnamed level knob while **Phase mod 3** (VCO2 → VCO3 PM when 2→3 coupling is on) stays hidden inside **Crunch** (`FUEG`). PM1 and PM2 have dedicated knobs; PM3 should too on sim hosts.

## What Changes

- Rename Audio row 6 (knob 7) from **VCO Envelope** to **Phase mod 3** in `ParamDisplayNames`, web static labels, Quick Dict, and sim manual
- **Sim engine:** Audio row 6 knob drives PM3 depth (`ZeroedExp`, same curve family as PM1/PM2); **Crunch no longer sets PM3 depth** on sim (Crunch keeps fuegoizer + external mix topology only)
- **Sim engine:** `OLVL` uses fixed firmware default (0.4) on sim — no user knob (Field-only concern when external is silent)
- Enable via `FroggersEngine::SetSimDedicatedPm3Knob(true)` in desktop and WASM host `Init()` only; firmware unchanged (`OLVL` on knob 7, PM3 on `FUEG`)
- Update Crunch gloss on sim docs: remove "also PM3" for sim hosts

## Capabilities

### New Capabilities

- `sim-audio-pm3-control`: Sim-only engine routing — row 6 → PM3, fixed OLVL, Crunch decoupled from PM3

### Modified Capabilities

- `sim-parameter-display-names`: Audio row 6 label **Phase mod 3** (replaces **VCO Envelope** requirement)

## Impact

- `src/core/FroggersEngine.hpp` — `m_pm3`, `SetSimDedicatedPm3Knob`, `ReadParamsBlock`, `StepOscillators`
- `src/core/DesktopHostIO.hpp`, `src/core/PagedHostIO.hpp` — call setter in `Init()`
- `sim/ParamDisplayNames.hpp`, `web/src/main.ts` static `PAGE_ROW_LABELS`
- `QUICK_DICT.md`, `SIM_MANUAL.md`, `web/public/quick-dict.md`, `web/public/sim-manual.md` (+ `npm run sync:docs`)
- `openspec/specs/sim-parameter-display-names/spec.md` delta at archive
- **Not changed:** `MANUAL.md`, Daisy firmware OLED, Field knob 7 `OLVL` / knob 8 `FUEG` semantics
