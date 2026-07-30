## Context

```
Today (Audio page)
──────────────────
Desktop SubModulePanel     Web #knobs grid
8 vertical rows            2 × 4 columns (8 cells)
label → [wave] knob jack   label → knob → mod select
rows 0–7 = Page params     rows map to Page params 0–7
Crispy = row 7             Crispy = cell 7

FroggersEngine
StepOscillators → v1,v2,v3
MixExternalAndOsc: (v1+v2+v3)/3 — no pair-sum envelopes
Page::x_numParameters = 8 — full; pair-AR cannot steal a row
```

Pair-AR params are **Audio-only extensions** beside the existing 8-row page model — not row 8+ inside `Page`, not fuegoized with Crispy unless explicitly wired later.

## Goals / Non-Goals

**Goals:**

- Four labeled controls: Attack 1+2, Release 1+2, Attack 2+3, Release 2+3
- AR envelope on `(v1+v2)*0.5` and `(v2+v3)*0.5` contributions into the osc mix
- Mod CV per param (same assignable mod set as other Audio rows)
- Single label/order authority; table-driven UI on desktop and web
- Desktop horizontal band with jack-on-top column stack; web third grid row

**Non-Goals:**

- VCV Rack / VST / Field firmware
- Separate decay and sustain stages (Release knob = combined decay+release time)
- Fuego/Crispy scrambling of pair-AR knobs in v1
- Renaming reverb “Decay” row

## Decisions

### D1 — Param model outside Page rows

**Choice:** `AudioPairArState` in `sim/AudioPairArLayout.hpp` — 4 knobs, 4 mod sources, 4 mod depths, 2× `RuntimeParam` smoothers per pair (attack rate, release rate). Host IO: `SetAudioPairArKnob(i,v)`, `GetAudioPairArEffective(i)`, mod source/depth mirroring page rows.

**Why:** `Page` is fixed at 8 params with fuego wiring on rows 0–6. Pair-AR is a distinct surface band; cramming into row indices would break Crispy semantics and snapshot layout.

### D2 — Label table (single authority)

**Choice:** `ParamDisplayNames::forAudioPairAr(uint8_t index)` backed by:

| Index | Label |
|-------|-------|
| 0 | Attack 1+2 |
| 1 | Release 1+2 |
| 2 | Attack 2+3 |
| 3 | Release 2+3 |

Desktop, web, manual, and snapshot field docs read this table only.

### D3 — DSP: one helper, two pair instances

**Choice:** `PairArEnvelope` struct (attack coeff, release coeff, level state). Two instances in `FroggersEngine`: `m_pair12`, `m_pair23`. Each sample:

1. Compute target from `|v_a + v_b| * 0.5` (or signed sum magnitude — pick `fabs(v1+v2)*0.5` for stability)
2. `PairArEnvelope::Step(target, attackKnob, releaseKnob)` — when target > level use attack time; else release time
3. Replace raw pair contribution in mix with `envelopedPair * pairWeight`

Knob 0–1 map to pair 12 attack/release; 2–3 to pair 23. Times via `PairArEnvelope` + `PhaseUtils::ExpParam::Compute` (1 ms – 10 s; see `pair-ar-vcv-time-range`).

**Why:** Same operation on two pairs → one struct, loop or two members — no copy-paste AR math.

### D4 — Desktop layout: horizontal band, inverted column

**Choice:** `SubModulePanel` when `m_pageIndex == 0`:

```
┌─ existing 8 vertical rows (unchanged) ─────────────┐
├─ pair-AR band (horizontal, 4 equal columns) ───────┤
│  [jack]   [jack]   [jack]   [jack]                 │
│  (knob)   (knob)   (knob)   (knob)                 │
│  label    label    label    label                  │
└────────────────────────────────────────────────────┘
```

`layoutPairArBand()` iterates `kAudioPairArCellCount` from `AudioPairArLayout.hpp` — one loop creates bounds, wires sliders, registers jack rects for `collectInputPorts`. Column width = `(panelWidth - padding) / 4`.

**Why:** Matches user mock (horizontal contrast to vertical rows); OMNI: no fourfold copy-paste layout blocks.

### D5 — Web layout: third row, same column widget

**Choice:** When `hostPage === 0`, render **12** knob columns (append 4 after the existing 8). CSS grid stays `repeat(4, 1fr)` — natural third row. Labels from WASM screen payload or static table matching `ParamDisplayNames`. Mod selects on all 12 cells on Audio page.

When `hostPage !== 0`, keep 8 columns (hide/remove pair-AR cols via `layoutKnobCols` page filter).

**Why:** Reuses existing knob column DOM; no inverted jack stack on web.

### D6 — WASM / screen protocol

**Choice:** Extend worklet screen message with `pairArRows: ScreenRow[4]` when page 0, or dedicated `pairAr` array. Bindings: `froggers_set_audio_pair_ar_knob`, mod source/depth, read effective values for knob display.

**Why:** Web third row must reflect engine state and mod routing without a second label source.

### D7 — Snapshot v2 extension

**Choice:** Add 4× knob + mod fields to `SimPresetSnapshot` (same version bump as `vst-plugin-host-ux` if landed, else increment here). Default attack/release mid-range on v1 read miss.

### D8 — Verification

**Choice:** Native test `PairArEnvelope_test.cpp` — attack rises, release falls, mod depth scales rate. Manual: desktop patch CC1 → Attack 1+2, hear pair 12 swell; web third row visible only on Audio tab.

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| Panel height overflow (desktop) | Increase `SubModulePanel` min height; verify 1440×720 VST/editor still fits (VST out of scope but share layout constants) |
| Web 12-cell Audio page taller on mobile | CSS already 2-col at narrow breakpoints; third row wraps to 6 rows — acceptable |
| Mix timbre shift vs saved presets | Snapshot defaults = instant AR (short times) or mid; document in manual |
| Confusion with reverb “Decay” | Labels use **Release**; manual glossary entry |

## Migration Plan

1. Land engine + host IO + snapshot
2. Desktop band + patch ports
3. WASM + web third row
4. Rebuild wasm bundle; manual sync
5. Manual A/B desktop vs web same four values

## Open Questions

1. **Trigger source:** envelope follows `|v1+v2|` instantaneous level (default in D3) vs gate on cross-coupler activity — confirm on apply if user wants edge-triggered AD instead
2. **Pair mix formula:** replace `(v1+v2+v3)/3` with `(env12 + v2 + env23)/3` or additive structure — resolve during engine patch with listening test

### D9 — Global strip: **Rand Resample** (not “Random”)

**Choice:** Add `ParamDisplayNames::forGlobalStrip(GlobalStripAction)` table:

| Action | Label | Engine call |
|--------|-------|-------------|
| RandAll | Rand All | `RandomizeAllPages()` |
| RandMods | Rand Mods | `RandomizeAllMod()` |
| MarblesStep | **Rand Resample** | `PressButton(0)` / `ButtonCallback(0)` |
| RandWaveforms | Rand waveforms | `RandomizeVcoMorphs()` |

**Why **Rand Resample** over alternatives:**

| Candidate | Verdict |
|-----------|---------|
| Random | Rejected — does not say *what* (user feedback) |
| Rand dice / Rand dice-roll | Rejected — cute but non-standard vs existing Rand Mods / Rand waveforms |
| Rand bags | Rejected — noun-only; does not describe the action |
| Rand draw | Rejected — ambiguous (“draw” reads as graphics/objects) |
| Rand step | Acceptable but overlaps mod-rack “Random 1 S&H” jargon |
| **Rand Resample** | **Chosen** — resamples both marbles S&H bags; clear audio/DSP meaning |

Desktop `GlobalStrip`, web `#marbles-btn`, manuals, and keyboard hint read the table. VCV `vcv-rack-field-parity` silkscreen uses the same string when the global strip lands.

**Also:** fix desktop `m_randomizeVcoWaveform` label **Rand waves** → **Rand waveforms** (already correct on web/manual). GlobalStrip tooltip for marbles: “Resample both random S&H channels (draws from bags)”.
