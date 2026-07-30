## Context

```
Shipped (v2.3.0)                    Target (v2.4.0)
─────────────────                   ─────────────────
No setPanel()                       Gray SVG + tiny corner “FroggersTiga”
Blank aluminum                      Black silkscreen from ParamDisplayNames
Broken ui::Label titles             Labels on SVG only (zero Rack label widgets)
CC @ 13.5 overlaps GATE @ 13.5      Dedicated CC row (design D4)
GreenRedLight on mod rack           GreenLight only (>55% threshold)
Expander A/B (3+3)                  Voicing (4 pages) + FX (2 pages + stereo I/O)
RoundBlackKnob (8 rows tight)       RoundSmallBlackKnob
No oscilloscopes (correct)          Explicit non-goal — LED-only mod rack
```

## OMNI audit (vcv-rack-panel-ux)

| Finding | Severity | Fix |
|---------|----------|-----|
| Zero `setPanel()` calls | **Blocker** | Minimal SVG per model |
| `addPageTitleLabel` uses `box.size` before layout (0×0) | **Blocker** | Delete helper; labels on SVG |
| CC switches X=13.5 share column with GATE X=13.5 | **Blocker** | Move CC row up; gate stays bottom row |
| `SmallLight<GreenRedLight>` on mod + CC | **Wrong widget** | `SmallLight<GreenLight>` |
| Proposed `VcvRowAbbrev.hpp` duplicates `ParamDisplayNames` | **OMNI violation** | SVG text sourced from `ParamDisplayNames` only |
| Proposed `ui::Label` SVG fallback | **Unnecessary** | SVG carries all silkscreen; no Rack label widgets |
| `RoundBlackKnob` on 8-row expander | **Density** | `RoundSmallBlackKnob` |
| Oscilloscope mentions in cross-change docs | **Scope noise** | VCV: LED-only; no scope widgets |
| “Marbles” naming in desktop core | **Out of VCV scope** | VCV already uses “random 1/2”; no rename here |

**Nesting / repetition (current `plugin.cpp`):** mod-output and CC-enable loops are compliant. Expander column loop is compliant. No extraction needed.

## Goals / Non-Goals

**Goals:**

- Every jack, knob, switch readable at 100% zoom without hover (boring gray silkscreen text)
- Tiny corner “FroggersTiga” watermark on each panel SVG (~6–8 pt Comic Sans; unobtrusive)
- Voicing 48 HP (pages 0,1,3,4) + FX 36 HP (pages 2,5) + stereo I/O
- Primary: mod rack green LEDs + master I/O; CC/MIDI on non-overlapping rows
- Single label authority: `ParamDisplayNames`

**Non-Goals:**

- Decorative panel art, color coding, custom knob skins
- Oscilloscope / CV trace widgets on VCV (LED-only per `vcv-vst-field-parity-panel` D3b)
- Renaming desktop `m_marbles*` / `Marbles` engine types (separate change)
- True stereo DSP (FX L/R duplicate mono)
- VST / desktop / web UI changes
- Publishing `vcv/` to GitHub `main`

## Decisions

### D1 — Minimal panel SVG

**Choice:** One SVG per model with `#808080` panel fill, `#000000` silkscreen text. “FroggersTiga” in Comic Sans at **~6–8 pt**, tucked into a bottom corner (e.g. lower-left, clear of screws and jacks). Light/dark variants follow Rack `createPanel` convention. Widget positions use `mm2px()` matching SVG hole centers.

**Why:** User wants zero visual flair — brand is a tiny corner watermark, not hero text. Rack-standard `setPanel()` fixes the “floating widgets on blank aluminum” problem.

### D2 — Labels: SVG silkscreen from ParamDisplayNames

**Choice:** Generate or hand-place SVG `<text>` using `ParamDisplayNames::forHostPage` (column titles) and `forHostPageRow` (row labels). Tooltips from `configParam`/`configInput` remain for hover detail. No `ui::Label` widgets. No separate abbrev header.

**Why:** One authority (OMNI data flow). Avoids broken dynamic labels and duplicate string tables.

### D3 — Voicing 48 HP + FX 36 HP

| Module | HP | Pages | Columns |
|--------|-----|-------|---------|
| Primary | 24 | — | Mod rack + I/O |
| Voicing | 48 | 0,1,3,4 | 4 × 12 HP |
| FX | 36 | 2,5 | 2 × 12 HP + stereo I/O |

Chain: `[FX] ← [Voicing] ← [Primary]`

### D4 — Primary layout de-overlap

| Row | Y (grid) | Contents |
|-----|----------|----------|
| Mod rack | 1.5 | Random btn, VCO Env / Random 1 / Random 2 outs + **green** LEDs |
| CC enables | 10.5 | CC1 / CC2 `CKSS` + green dim/bright LEDs |
| Master I/O | 13.5 | Audio, CV, gate (11.5), MIDI, CV outs |

CC switches at 13.5/15 GRID X on row 10.5 — gate at 11.5 GRID X on row 13.5. Minimum 2 GRID separation verified in bounds script.

### D5 — Green LED mod indicators

**Choice:** `SmallLight<GreenLight>` for mod sources 4–6 and CC enable pairs. Bright when CV > 0.55; off otherwise. CC enable LEDs: bright when on, dim (0.4) when off.

**Why:** User directive. `GreenRedLight` implies red state that does not exist in this design.

### D6 — Widget palette

**Choice:** `RoundSmallBlackKnob` + `ThemedPJ301MPort` on expanders; `TL1105`, `CKSS`, `ThemedPJ301MPort`, `MidiButton` on primary. Standard Rack componentlibrary only.

### D7 — VST / desktop parallel

| VCV issue | VST/desktop risk |
|-----------|------------------|
| No panel SVG | None — JUCE UI |
| GreenRedLight | None — desktop scopes separate |
| CC/gate overlap | None — separate layout |
| Oscilloscopes | VST keeps scopes; VCV never adds them |

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| 48 HP voicing wide | Standard Rack; user-validated topology |
| Mono engine vs stereo FX jacks | Document duplicate-out in DEVELOPMENT.md |
| BREAKING slug rename | Bump 2.4.0; migration note |
| Comic Sans not installed | Embed as SVG path or use `Comic Sans MS` with fallback sans |

## Migration Plan

1. Land minimal SVGs + layout fix; bump 2.4.0
2. User replaces Expander A/B with Voicing + FX in patches
3. `arch -x86_64 ./build.sh --install`
