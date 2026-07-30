## Why

Product ring mod (`external × VCO1 × VCO2 × VCO3`) and the FUEG/Crispy morph between product and parallel topologies add complexity without matching how operators use external input. Parallel ring mod (`average(ext×VCO1, ext×VCO2, ext×VCO3)`) is the intended external behavior; the product path and all documentation/spec references to mix topology should be deleted, not preserved behind a knob or a forced blend.

## What Changes

- **BREAKING**: Remove product ring mod from `FroggersEngine::MixExternalAndOsc`; when external audio is detected, output is only the parallel average.
- Remove `fueg` from the external-mix code path (no morph, no `ZeroedExp`, no dead product branch).
- Update `MANUAL.md` (Field) with the same plain-language external-mix and FUEG semantics as the sim manual — topology removed, parallel ring mod only, Audio cross-mod/ring-mod explained clearly.
- Update **all** quick-dict mirrors (`QUICK_DICT.md`, `docs/quick-dict.md`, `web/public/quick-dict.md`) so Crispy/FUEG and Ext. In. glosses match across Field and sim copies.
- **Rewrite `SIM_MANUAL.md` and mirrors** — not a patch. Restructure top-to-bottom in learning order (transport → optional external → randomize strip → Crispy → Audio in depth → other pages → mod bay → host quirks). Plain language; fix factual errors (e.g. desktop Delay is the **sixth column**, not an “overlay”).
- Clarify that FUEG/Crispy remains the fuegoizer (and on Field Audio page, PM3 depth); it no longer has an external-mix role.
- Add a normative spec for external ring-mod mix behavior shared across all hosts.

## Capabilities

### New Capabilities

- `external-ring-mod-mix`: External-input gate and parallel ring-mod mix formula in the shared engine.

### Modified Capabilities

- `field-operator-doc-parity`: Field `MANUAL.md` and quick-dict Field/Sim glosses — parallel ring mod only, FUEG without mix topology, aligned tone with sim manual.
- `sim-operator-doc-parity`: Remove mix-topology language; require learner-first manual structure, plain-language Crispy gloss, correct desktop six-column layout, and Audio/Random signal-flow copy (Marbles attribution on Random).
- `sim-pm3-knob-parity`: Crispy/FUEG on sim is fuegoizer only; remove external-mix topology control requirement.

## Impact

- `src/core/FroggersEngine.hpp` — `MixExternalAndOsc`, `ProcessSample` call site
- `MANUAL.md`, `SIM_MANUAL.md`, `QUICK_DICT.md`, `docs/sim-manual.md`, `docs/quick-dict.md`, `web/public/sim-manual.md`, `web/public/quick-dict.md`
- OpenSpec deltas for `field-operator-doc-parity`, `sim-operator-doc-parity`, `sim-pm3-knob-parity`; new `external-ring-mod-mix` main spec synced to `openspec/specs/` at archive
- No host-parameter count change; audible behavior change when external input is active and FUEG/Crispy was not fully CCW

## OMNI rule audit (2026-06-24)

Audit scope: planned change artifacts + current `FroggersEngine.hpp` / operator docs. Implementation not started (all tasks open).

### Compliant

| Rule | Finding |
|------|---------|
| Data flow | Removing `fueg` from `MixExternalAndOsc` decouples knob 8 from pre-drive mix; `fuegKnob` remains on `StepOscillators` (+ PM3 when `!SetSimDedicatedPm3Knob`). |
| Dead code | Product branch and `ZeroedExp(fueg)` in the mix path are deleted, not forced to `t = 1`. |
| Nesting | Simplified `MixExternalAndOsc` stays at ≤2 control-flow levels. |
| Defensive code | No new guards; Schmidt gate semantics unchanged. |
| Plan language | Proposal/design/tasks use deterministic directives. |

### Gaps to close during implementation

| Rule | Finding | Resolution |
|------|---------|------------|
| Data flow boundary | VCO-only path uses `MixOscVoices` (pair-AR aware). External parallel path averages `extIn × each VCO` — it does **not** use `MixOscVoices`. Pair-AR does not shape external ring mod. | Capture in `design.md` decision 5 and `external-ring-mod-mix` spec. |
| Repetition | `(v1 + v2 + v3) / 3` appears in `MixOscVoices` (non–pair-AR path) and in the parallel external formula (`extIn ×` that average). | Do not route external mix through `MixOscVoices`. Optional shared `AverageVcoVoices(v1,v2,v3)` only if extraction meets one-time-helper trigger count (≥2 of 4); default: inline parallel return, no new helper. |
| Doc completeness | Stale topology copy spans more locations than tasks originally listed: MANUAL signal-flow ascii, pages table row, “Audio page exception” heading, “entanglement continuum” phrase, root `QUICK_DICT.md`. | Tasks 2.x / 3.x / 4.3 expanded below. |
| Verification | Compile + doc grep only; no engine-level behavioral check. | Add task 4.4: manual or automated check that gate-open mix ignores knob 8. |
| Main spec sync | `openspec/specs/sim-pm3-knob-parity` and `sim-operator-doc-parity` still require Crispy as external-mix topology control until archive. | Add archive task 5.x. |

### Risk surfaced by audit

When pair-AR is active and external gate is open, operators hear parallel ring mod on raw per-VCO samples while VCO-only path uses pair-AR dynamics. This is **current** behavior for the parallel endpoint (`t = 1`); the change locks it in. Document in MANUAL external-mix section so operators do not expect pair-AR on the external path.

## Sim manual rewrite (scope)

Current `SIM_MANUAL.md` buries essentials, jumps between “Layout”, “Global controls”, “Mod bay”, and “Page reference”, and states incorrect desktop chrome (“Delay overlay”). Verified in `MainComponent::resized`: desktop divides panel width by **6** — five core columns plus **Delay as the sixth column** (always visible, not an overlay).

### Reader order (top of page → bottom)

```
┌─────────────────────────────────────────────────────────────┐
│ 1. Getting sound     Play / Stop (web: wait for engine)     │
│ 2. External (opt.)   Ext. In. off = VCOs only; on = ring   │
│                      mod when input is loud enough          │
│ 3. Randomize strip   Page + global buttons, plain names     │
│ 4. Crispy            Knob 8 every page — dumbed-down gloss  │
│ 5. Signal path       One-line chain before page detail      │
│ 6. Audio (first)     3 VCOs, cross-coupler, PM, ext ring    │
│ 7. Random            Marbles-inspired; feeds mod S&H        │
│ 8. Drive → Filter →  Match desktop FX column order          │
│    Reverb → Delay     Delay = 6th column / web page 6       │
│ 9. Mod bay           After knobs make sense                 │
│ 10. Host differences Short: web pages vs desktop columns    │
└─────────────────────────────────────────────────────────────┘
```

### Crispy (target copy tone)

Knob 8 on every page. It **scrambles** knobs 1–7 on that page — turns smooth knob moves into gritty, jumpy values. Modulation applies first; Crispy scrambles the result. You can modulate Crispy too (scramble amount follows the modded Crispy value). It does **not** change how external audio is ring-modded; Ext. In. uses parallel ring mod when the input gate is open.

### Audio section (target content)

- Three oscillators (VCO1–3); VCO1/2 waveform morph, VCO3 sine.
- **Cross-coupler** (noon = off): turn CCW for VCO1↔VCO2 coupling; CW for VCO2↔VCO3.
- **Phase mod** knobs set how hard each coupled path pushes phase (PM1: VCO2→VCO1; PM2: VCO1+VCO3→VCO2; PM3: VCO2→VCO3 when coupled CW).
- **External input**: optional. Silent/off → hear VCO mix only. Loud enough → each VCO ring-modulates the external signal in parallel (average of ext×VCO1, ext×VCO2, ext×VCO3).
- Pair-AR band: optional level-follower on VCO pair sums (VCO-only path); keep short.

### Random section (target content)

Inspired by **Mutable Instruments Marbles**. Two random CV channels configured here; press **Rand Resample** (global strip) to draw new values from each bag. Outputs appear in the mod bay as Random 1/2 S&H. No internal clock — you trigger steps.

### Factual fixes (verified)

| Stale claim | Correct |
|-------------|---------|
| Desktop “Delay overlay” | Sixth column in the six-column row (`area.getWidth() / 6`; `m_panels[5]` takes the last column) |
| “Five adjacent panels” | Five core columns **plus** Delay column (six panels total) |
| Desktop column order | **Audio → Random → Drive → Filter → Reverb → Delay** (pages 0,1,4,3,2,5) |
| Web page pill order | **Audio → Random → Reverb → Filter → Drive → Delay** (host page index; differs from desktop columns) |

### Tone rules

- Short sentences. No host page index tables in the learner path.
- Tables only for knob rows inside each page section.
- Move OpenSpec links, version history, and host-input boundary matrices to an appendix or collapse into “Advanced / per-host”.
