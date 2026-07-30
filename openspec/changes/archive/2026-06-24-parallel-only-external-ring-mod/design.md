## Context

`FroggersEngine::MixExternalAndOsc` currently branches on `hasExternal`:

- Gate closed: `OLVL × average(VCO1, VCO2, VCO3)` — VCO-only.
- Gate open: morph between product and parallel via `ZeroedExp(fueg)` where `fueg` is Audio page knob 8 (FUEG on Field, Crispy on sim).

All hosts share this engine. FUEG/Crispy also drives the fuegoizer (and on Field firmware, PM3 depth when `!SetSimDedicatedPm3Knob`). External mix topology is a separate concern that should not share knob 8.

## Goals / Non-Goals

**Goals:**

- External gate open → single mix formula: `(ext×VCO1 + ext×VCO2 + ext×VCO3) / 3`.
- Delete product multiply path and `fueg` parameter from `MixExternalAndOsc`.
- Delete all operator-doc references to product ring mod, parallel-vs-product morph, and FUEG/Crispy external-mix role.
- Rewrite sim operator manual (`SIM_MANUAL.md` + mirrors) in learner-first order; fix desktop Delay column error; dumb down Crispy and Audio/Random explanations.
- Preserve VCO-only path when gate is closed; preserve fuegoizer and PM3 semantics for knob 8.

**Non-Goals:**

- Changing external gate thresholds (~−40 dBFS Schmidt trigger).
- Stereo I/O on Daisy Field (mono in/out stays as-is).
- Repurposing FUEG/Crispy for a new Audio-page function beyond existing fuegoizer/PM3 roles.
- Host UI label changes (Crispy/FUEG names unchanged).
- Full Field manual restructure (Field keeps hardware-specific sections; signal flow, external mix, FUEG, and Audio page align with sim plain-language tone).

## Decisions

### 1. Simplify `MixExternalAndOsc` — delete product branch

**Choice:** When `hasExternal`, return `(input * v1 + input * v2 + input * v3) * (1/3)` only. Drop `fueg` from the function signature and call site.

**Rationale:** Removes the alternate mode entirely rather than forcing `t = 1` or leaving unreachable product code. Matches user intent: no reference to the other mode.

**Rejected:** `t = 1.0f` morph — keeps dead product path and misleading `fueg` parameter.

### 2. Do not add a replacement topology knob

**Choice:** No new parameter for external mix shape.

**Rationale:** Product topology is removed by design, not hidden.

### 3. Docs: subtract, do not reframe as “locked”

**Choice:** Remove mix-topology sections/tables; describe external path as parallel ring mod only. State gate-closed = VCO-only unchanged.

**Rationale:** Operator language should not imply a disabled or locked mode exists.

### 4. Spec placement

**Choice:** New capability `external-ring-mod-mix` for engine behavior; deltas for doc/parity specs that mention topology.

### 5. External mix does not use `MixOscVoices`

**Choice:** When `hasExternal`, return `(extIn × VCO1 + extIn × VCO2 + extIn × VCO3) / 3`. Do not multiply `extIn` by `MixOscVoices(v1, v2, v3)`.

**Rationale:** `MixOscVoices` applies pair-AR dynamics to the VCO-only sum. External ring mod is per-voice multiply then average — a different domain step. Conflating the two would change timbre when pair-AR is active.

**Algebra note:** Parallel formula equals `extIn × average(VCO1, VCO2, VCO3)`; reuse a shared average helper only if OMNI one-time-helper trigger count is met (repetition + domain boundary). Default implementation: keep the explicit parallel return in `MixExternalAndOsc`; do not add a wrapper whose only job is `(v1+v2+v3)/3`.

### 6. Doc subtraction inventory

Remove topology language from every operator surface, not only tables:

| Location | Stale content |
|----------|----------------|
| `MANUAL.md` signal-flow ascii | “FUEG continuum when external present” |
| `MANUAL.md` | Mix topology table (lines 27–34), pages table “PM3 + mix topology”, “Audio page exception: PM3 and mix topology” heading, bullet 2 mix topology, “entanglement continuum”, Audio knob 8 row |
| Sim manuals + quick-dict mirrors | Crispy “blends external ring-mod topology” |
| Main specs at archive | `sim-pm3-knob-parity` Crispy external-mix role; `sim-operator-doc-parity` Crispy mix-topology glossary |

Replace with: gate closed → VCO-only at `OLVL`; gate open → parallel ring mod only; FUEG/Crispy = fuegoizer (+ PM3 on Field Audio).

### 7. Sim manual — learner-first rewrite (not topology patch)

**Choice:** Replace `SIM_MANUAL.md` structure. Single scroll path for new users; demote reference material.

**Section order:**

1. **Title + one-liner** — sim hosts share one guide; Field hardware → `MANUAL.md`.
2. **Getting sound** — Play / Stop; web engine-ready note.
3. **External input (optional)** — Ext. In.; VCO-only vs parallel ring mod when gate opens; peak meter on web/desktop.
4. **Randomize buttons** — per-page Randomize / Rand mod; global strip Rand All, Rand Mods, Rand Resample, Rand waveforms; one sentence each.
5. **Crispy** — knob 8 on every page; scramble knobs 1–7; mod-then-scramble; moddable; explicitly not external mix control.
6. **How audio flows** — `VCOs (+ optional ext ring mod) → Drive → Filter → Reverb`; Delay as parallel wet FX on sim hosts.
7. **Audio** — VCOs, cross-coupler, PM1–3, external ring mod, pair-AR (brief), waveforms.
8. **Random** — Marbles-inspired dual S&H; bags; Rand Resample; mod-rack outputs.
9. **Drive → Filter → Reverb → Delay** — in that learning order (matches desktop FX columns + Delay last).
10. **Mod bay** — sources, routing, depth crossfade (compressed).
11. **Desktop / Web / Plugin / VCV** — short host boxes; correct six-column desktop layout.
12. **Appendix** — version history, OpenSpec host-master link, MIDI CC matrices.

**Desktop layout copy (verified):**

```text
[ Audio | Random | Drive | Filter | Reverb | Delay ]  ← six equal columns
         ↑ core pages 0,1,4,3,2              ↑ page 5
```

Not an overlay. `MainComponent::resized` uses `area.getWidth() / 6`.

**Rejected:** Keep current “Layout → Global → Mod bay → Page reference” order — front-loads navigation trivia before Play/External/Randomize.

## Risks / Trade-offs

- **[Risk] Users who relied on product ring mod lose that sound** → Documented as intentional removal in proposal; no migration knob.
- **[Risk] Field Audio page FUEG still controls PM3 on hardware** → MANUAL Audio exception section updated to drop mix topology only; PM3 bullet retained.
- **[Risk] Stale doc mirrors** → Task list includes all seven manual/quick-dict paths plus MANUAL signal-flow ascii and pages table.
- **[Risk] Pair-AR vs external path confusion** → Document that pair-AR shapes VCO-only mix only; external ring mod always uses per-VCO parallel average.
- **[Risk] Manual rewrite scope creep** → Sim rewrite is structure + tone + topology removal + layout fix; knob semantics unchanged; quick-dict stays short glosses synced to manual headings.
