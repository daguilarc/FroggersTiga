## Context

```
Sim (desktop / web) — unchanged              VCV Rack (this change)
─────────────────────────────────            ─────────────────────────────
Audio page pair-AR band (4 knobs)            LEFT expander: VCO AR module
  Att./Rel. 1+2, Att./Rel. 2+3                 Att./Rel. × VCO1, VCO2, VCO3
  Level follower on pair sums                  Level follower on |v1|, |v2|, |v3|
Audio column Crispy (row 7, right expander)  LEFT expander Crispy (VCO domain)
  Fuegoizes Audio rows 0–6                     Fuegoizes VCO1–3 on main/right
Global + column randomize (right expander)     LEFT expander Randomize + Randmod
                                               (six A/R knobs only)

Patch chain today:
  [ Main 72 HP ] —right→ [ Voicing 72 HP ]   [ FX 36 HP optional ]

Patch chain after:
  [ VCO AR ~24 HP ] —left→ [ Main 72 HP ] —right→ [ Voicing 72 HP ]   [ FX ]
```

Main module owns `PagedHostIO` + `ProcessBlock`. Right expander writes page knobs. Left expander writes `VcoArState` + VCO Crispy + triggers local randomize.

## Goals / Non-Goals

**Goals:**

- Per-VCO Attack/Release (6 knobs) on a left-linked expander
- Time range 1 ms – 10 s via `PairArEnvelope` constants + `ExpParam::Compute` (same as `pair-ar-vcv-time-range`)
- Envelope follows each VCO magnitude (`|vN|`) — attack on rise, release on fall
- Dedicated Crispy on left expander affecting VCO1–3 params in the voicing chain
- Randomize + Randmod on left expander for A/R knobs only
- Single label table (`ParamDisplayNames::forVcvVcoAr`) and layout table (`VcvPanelLayout`)
- Path silkscreen; expander required for per-VCO AR (graceful bypass when unlinked)

**Non-Goals:**

- Replacing sim pair-AR on desktop/web
- Gate-triggered ADSR stage machine (no sustain knob)
- Moving Audio column Crispy or pair-AR onto VCV right expander
- Publishing `vcv/` to public main (local-only GPL boundary unchanged)
- Mod CV on every A/R param in v1 (optional v1.1 — spec allows mod jacks if panel fits)

## Decisions

### D1 — Third module: left expander only

**Choice:** Register `FroggersTigaVcoArExpander` in `plugin.json`. User places it **to the left** of main; main walks `leftExpander.module` chain to find VCO AR state.

**Why:** Right expander is full 72 HP voicing columns. Per-VCO AR + Crispy + randomize does not fit main faceplate without crowding mod rack / I/O.

**Alternative rejected:** Add A/R rows to Audio column on right expander — duplicates sim pair-AR semantics, not per-VCO.

### D2 — DSP: three envelopes, VCV-gated

**Choice:** `VcoArState` holds 3× `PairArEnvelope`, 6 knob values, optional mod source/depth per knob. `FroggersEngine::MixOscVoices`:

```cpp
// when host reports vcoArLinked:
e1 = m_vcoAr[0].Step(fabs(v1), att0, rel0, sr);
e2 = m_vcoAr[1].Step(fabs(v2), att2, rel2, sr);
e3 = m_vcoAr[2].Step(fabs(v3), att4, rel4, sr);
return (e1 * sign(v1) + e2 * sign(v2) + e3 * sign(v3)) / 3.f;  // or envelope * |v| blend — pick magnitude-only
```

When `!vcoArLinked`, existing pair-AR path (`m_pair12`, `m_pair23`) runs for sim; when linked on VCV, **per-VCO path replaces pair-AR** for that process block.

**Why:** One `PairArEnvelope` struct, loop over three VCOs — OMNI, no copy-paste AR math. VCV and sim paths mutually exclusive per host flag.

### D3 — Label table

| Index | Label |
|-------|-------|
| 0 | Att. VCO1 |
| 1 | Rel. VCO1 |
| 2 | Att. VCO2 |
| 3 | Rel. VCO2 |
| 4 | Att. VCO3 |
| 5 | Rel. VCO3 |

`ParamDisplayNames::forVcvVcoAr(uint8_t index)` — VCV silkscreen + widget tooltips only; not used on desktop/web.

### D4 — Left-expander Crispy (VCO domain)

**Choice:** One knob `VCO_CRISPY_PARAM` on left expander. Engine maps it to fuego mask over **Audio page rows 0–2** (VCO1–VCO3 frequency knobs on main/right expander) — same fuego math as page Crispy but **independent knob** and **scoped to rows 0–2 only**, not full page rows 0–6.

**Why:** User asked for Crispy that "controls the VCOs in the main one" without conflating with right-expander Audio column Crispy (row 7).

### D5 — Randomize on left expander

**Choice:** Two momentary buttons: **Randomize** → `VcoArState::randomizeKnobs()`, **Randmod** → `VcoArState::randomizeMod()`. Rising-edge handler matches `vcv-randomize-controls` pattern (single dispatch table entry).

**Why:** Operators randomize A/R times without touching voicing-column Randomize (which hits full page rows 0–6).

### D6 — Panel layout (~24 HP)

```
┌─ Froggers Tiga VCO AR ─────────────────────┐
│  [Rand] [Randmod]                         │
│  ┌ VCO1 ─┐  ┌ VCO2 ─┐  ┌ VCO3 ─┐        │
│  │ Att   │  │ Att   │  │ Att   │        │
│  │ Rel   │  │ Rel   │  │ Rel   │        │
│  └───────┘  └───────┘  └───────┘        │
│  [ Crispy ]                               │
└───────────────────────────────────────────┘
```

Three equal columns; constants in `VcvPanelLayout.hpp` (`kVcoArHp = 24`, column pitch, row step).

### D7 — Expander sync

**Choice:** Left expander `process()` writes knob values into main module's `VcoArState` via pointer set at link time (same pattern as right expander → `SetPageKnob`). Main `process()` reads state before `host.ProcessBlock`.

**Why:** Single engine instance on main; expanders are control surfaces only.

## Risks / Trade-offs

- **[Risk] Two Crispy knobs confuse operators (left VCO vs Audio row 7)** → Silkscreen "VCO Crispy" vs "Crispy"; manual Rack section documents scope
- **[Risk] Engine fork sim vs VCV mix paths** → Host flag `vcoArLinked`; unit test both paths
- **[Risk] Left expander omitted — silent expectation gap** → Main runs pair-AR sim path OR dry VCO mix; document in DEVELOPMENT.md
- **[Risk] HP too tight for mod jacks** → v1 knobs only; mod inputs deferred to follow-up if 24 HP insufficient

## Migration Plan

1. Add `VcoArState` + host IO on main module
2. Ship left expander module + SVG
3. Update DEVELOPMENT.md patch diagram
4. Manual: link left AR, patch main+right, verify per-VCO swell differs from sim pair-AR behavior

Rollback: disable module in `plugin.json`; sim hosts unaffected.

## Open Questions

- Mod CV per A/R knob on v1 panel: include if 24 HP layout passes bounds CI, else defer.
