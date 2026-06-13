## Why

Filter row 0 (`DELF`) and row 4 (`COMF`) both looked like delay knobs — **Comb line** vs **Comb delay** — but they do different jobs. Row 4 sets **comb pitch** (notch spacing). Row 0 is a short fractional line **before** the comb that shifts phase into it — an **offset** into the comb, not a second pitch control.

The current mapping is also backwards: knob minimum runs the **longest** tap (~50 ms) because `PureDelay` uses `1/freq` with frequency rising as the knob rises. Users expect higher knob → longer offset / more smear.

## What Changes

- **Rename** Filter row 0 sim label → **Comb offset** (Quick Dict: `Short line before comb — smears strike, not pitch`).
- **Remap** row 0 DSP: exponential **0.001 s – 0.1 s**; **knob 0 = 1 ms, knob 1 = 100 ms** (monotonic).
- **Keep** `PureDelay` before `Comb::Process`; firmware OLED `DELF` unchanged.
- **Consolidates** `delay-grain-filter-row0` Filter row 0 work (archive that change; canonical spec → `openspec/specs/filter-comb-offset/spec.md`).

**Unchanged:** row 4 **Comb delay** (comb pitch), peak EQ rows 1–3, comb feedback/LP.

## Label distinction (user-facing)

| Row | Label | What it does |
|-----|-------|----------------|
| 0 | **Comb offset** | How far/smear before the comb strikes (phase primer) |
| 4 | **Comb delay** | Comb notch pitch / loop length |

## Capabilities

### New Capabilities

- `filter-comb-offset`: Row 0 time remap + **Comb offset** sim label.

### Modified Capabilities

- `quick-dict-format`: Row 0 entry; supersedes Comb line gloss.

## Impact

- `src/core/FroggersEngine.hpp` — row 0 target: delay seconds, not Hz band
- `src/core/Comb.hpp` — explicit seconds → samples on `PureDelay`
- `sim/ParamDisplayNames.hpp`, `QUICK_DICT.md`, `web/public/manual.md` Filter section
