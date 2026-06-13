## Context

```
Field Audio page (MANUAL.md)          Sim today (wrong for default use)
────────────────────────────          ─────────────────────────────────
Knob 5  PM1A  Phase mod 1             ✓ dedicated
Knob 6  PM2A  Phase mod 2             ✓ dedicated
Knob 7  OLVL  VCO-only level          Labeled "VCO Envelope" — misnamed fallback
Knob 8  FUEG  Crunch + PM3 + mix      PM3 buried in Crunch

Sim default: External OFF → OLVL is main volume; user never asked for that.
PM3 is gated by cross-coupler 2→3 (same as firmware) but has no visible control.
```

Existing pattern: `SetSimWaveMorph(bool)` gates sim-only oscillator behavior without touching `DaisyIO.hpp`. Same gate pattern for PM3 knob remapping.

## Goals / Non-Goals

**Goals:**

- Audio row 6 on desktop + web controls PM3 depth with same `ZeroedExp` mapping as PM1/PM2
- Crunch on sim controls only fuegoizer (knobs 1–7 scramble) and mix topology when external gate is high
- Fixed `OLVL` at firmware init default (0.4) on sim so VCO-only path is audible without a Field artifact knob
- Single dictionary + single engine flag — no per-host JS/C++ layout branches

**Non-Goals:**

- Firmware OLED or `MANUAL.md` knob table changes
- New knob count or reordering DOM/panel rows
- Moving mix topology off Crunch (still `FUEG` when external present)
- Exposing OLVL on sim elsewhere

## Decisions

### D1: `SetSimDedicatedPm3Knob(bool)` on `FroggersEngine`

**Choice:** New bool default `false`; `DesktopHostIO::Init()` and `PagedHostIO::Init()` call `SetSimDedicatedPm3Knob(true)` alongside existing `SetSimWaveMorph(true)`.

**Why:** Mirrors proven sim/firmware split. Field build never enables the flag.

### D2: Add `RuntimeParam m_pm3`; row 6 reads PM3 when flag true

**Choice:**

```cpp
// ReadParamsBlock when m_simDedicatedPm3Knob:
m_pm3.SetTarget(ZeroedExp(m_audioGenParams->GetParam(6)));
m_oscLvl.SetTarget(ExpMap(0.01f, 1.0f, 0.4f)); // fixed, ignore stored row-6 knob for OLVL

// StepOscillators when m_simDedicatedPm3Knob:
float pm3d = m_pm3.Process();
// else unchanged: pm3d = ZeroedExp(fuegKnob);
```

**Why:** Reuses Audio page parameter slot 6 — knob messages unchanged (`knob index 6`). Only DSP routing changes. PM1/PM2 already use `ZeroedExp(GetParam(4|5))`.

**Alternative rejected:** Relabel only — leaves PM3 on Crunch; user explicitly asked for functionality.

### D3: Display name **Phase mod 3** in `ParamDisplayNames` row 6 Audio

**Choice:** Replace **VCO Envelope** with **Phase mod 3** to align with PM1/PM2 naming and engine behavior.

**Why:** Mod source **VCO Envelope** (mod index 4, `UpdateM5FromVco`) stays a separate mod-bay concept — no collision once row label matches PM3.

### D4: Crunch docs on sim only

**Choice:** Quick Dict / sim manual Crunch line drops "also PM3 on Audio page" for sim docs; Field note remains in `MANUAL.md`.

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| Users lose OLVL trim on sim | Intentional; 0.4 default matches firmware init; External off is normal sim path |
| Saved presets with row-6 values meant as level now affect PM3 | Acceptable sim-only semantic shift; document in sim manual one line |
| PM3 inaudible without 2→3 coupling | Same as Field; hint text optional follow-up |
| `sim-parameter-display-names` spec conflict | Delta replaces VCO Envelope scenario |

## Migration Plan

1. Engine flag + `m_pm3` + host Init calls
2. `ParamDisplayNames` + web static labels
3. Docs sync
4. Manual test: Audio page, cross-coupler CW, row 6 affects VCO3 PM; Crunch at min does not zero PM3 when row 6 up
5. Archive; merge spec delta

## Open Questions

- (none — fixed OLVL 0.4 matches firmware `InitParam("OLVL", 6, 0.4f)`)
