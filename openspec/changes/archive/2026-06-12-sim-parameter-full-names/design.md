## Context

```text
Firmware OLED (4-char)     Sim UI today              Target
─────────────────────     ─────────────             ──────
V1VO, RVMX, FUEG…    →    same abbrev in columns  →  full titles + "Crunch"
```

`Parameter::m_name` stays 4 characters for Field OLED and core identity. Sim hosts already alias Audio rows 0–2 to **VCO1** / **VCO2** / **VCO3** on desktop; web WASM still forwards `GetName()` abbreviations everywhere else.

**Layout:** Default desktop window **1680×720** → ~280 px per panel. Row label column is `row width − (knob 38 + jack 20 + optional wave 30)`. Font ~11–12 px; longest labels ~13 chars (`Cross-coupler`, `Comb feedback`) fit without ellipsis at 1680 if `kMinLabelWidth` bumps to **72** and label uses single line.

## Goals / Non-Goals

**Goals:**

- One dictionary, three consumers: desktop `getRowName()`, WASM `ScreenRow.name`, Quick Dict left column.
- **Crunch** for fuegoizer row (index 7) on all six pages.
- Quick Dict matches what users see in columns.

**Non-Goals:**

- Renaming firmware OLED symbols or `MANUAL.md` knob tables.
- VCV Rack faceplate labels.
- Changing randomize skip semantics (still skips Crunch row for knob randomize).

## Decisions

### 1. `sim/ParamDisplayNames.hpp`

Static lookup — no runtime allocation:

```cpp
namespace ParamDisplayNames {
constexpr uint8_t kNumPages = 6;
constexpr uint8_t kNumRows = 8;
const char* forHostPageRow(uint8_t hostPage, uint8_t row);
} // namespace
```

`hostPage` matches desktop panel index / web `hostPage`: 0 Audio … 4 Drive, 5 Delay.

### 2. Authoritative display dictionary

| Page | Row | OLED (internal) | Sim display |
|------|-----|-----------------|-------------|
| **Audio** | 0 | V1VO | VCO1 |
| | 1 | V2VO | VCO2 |
| | 2 | V3VO | VCO3 |
| | 3 | XCPL | Cross-coupler |
| | 4 | PM1A | Phase mod 1 |
| | 5 | PM2A | Phase mod 2 |
| | 6 | OLVL | VCO level |
| | 7 | FUEG | **Crunch** |
| **Marbles** | 0 | PROB | Step chance |
| | 1 | DJV1 | Deja vu 1 |
| | 2 | SZ1 | Bag size 1 |
| | 3 | SLW1 | Slew 1 |
| | 4 | DJV2 | Deja vu 2 |
| | 5 | SZ2 | Bag size 2 |
| | 6 | SLW2 | Slew 2 |
| | 7 | FUEG | **Crunch** |
| **Reverb** | 0 | RVMX | Wet/dry |
| | 1 | RSIZ | Room size |
| | 2 | RDEC | Decay |
| | 3 | RPRE | Pre-delay |
| | 4 | RDMP | Damping |
| | 5 | RMOD | LFO depth |
| | 6 | RRAT | LFO rate |
| | 7 | FUEG | **Crunch** |
| **Filter** | 0 | DELF | Filter delay (was Pure delay; see `web-sim-core-fix`) |
| | 1 | BUPF | Bump center |
| | 2 | BUPR | Bump gain |
| | 3 | BUPW | Bump width |
| | 4 | COMF | Comb delay |
| | 5 | COMQ | Comb feedback |
| | 6 | CMLP | Comb LP |
| | 7 | FUEG | **Crunch** |
| **Drive** | 0 | GAIN | Drive |
| | 1 | SHAPE | Shape |
| | 2 | SRR1 | SRR 1 |
| | 3 | SRR2 | SRR 2 |
| | 4 | DIGR | Reorganizer |
| | 5 | HASH | Bit depth |
| | 6 | FUZZ | Fuzz |
| | 7 | FUEG | **Crunch** |
| **Delay** | 0 | DTIM | Delay time |
| | 1 | DSND | Send |
| | 2 | DFBK | Feedback |
| | 3 | DWID | Stereo width |
| | 4 | DTON | Tone |
| | 5 | DMOD | Mod depth |
| | 6 | DMIX | Wet mix |
| | 7 | FUEG | **Crunch** |

**Rejected:** Local per-host string tables — duplicates drift (desktop-compact-layout lesson).

### 3. Desktop wiring

`DesktopPanelBackend::getRowName()` → `ParamDisplayNames::forHostPageRow(m_pageIndex, row)` (remove separate VCO1 array — dictionary covers Audio 0–2).

`DelayHostBackend::getRowName()` → `ParamDisplayNames::forHostPageRow(5, row)`.

`DelayState::rowName()` — keep for internal/debug or delegate to display names; patch overlay may still reference OLED ids in tooltips later.

### 4. WASM / web

`bindings.cpp` screen builder: emit `ParamDisplayNames::forHostPageRow(hostPage, row)` instead of `param->GetName()`.

Web `DELAY_HINTS` row 7: **Crunch**; rows 0–6 hints optional (label already descriptive) — trim redundant hint text on Delay page to avoid double labels.

Web page blurbs: replace “fuegoizer” with “Crunch” where user-facing.

### 5. Quick Dict

Rewrite sections using **`Display name : short gloss`**. Example:

```text
Crunch : Scramble knobs 1–7 resolution (Field: FUEG)
Cross-coupler : CCW 1→2, CW 2→3 from noon
```

Field cross-ref in gloss only where helpful (`FUEG` in Manual, not in column).

### 6. Layout tweak

`kMinLabelWidth` 48 → **72** in `SubModulePanel.cpp` if soak test at 1680 shows clipping on “Cross-coupler”.

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| Long labels clip on narrow windows | 1680 default verified; ellipsis only below min width |
| Manual / Quick Dict diverge on Crunch vs FUEG | Manual keeps FUEG; Quick Dict gloss links Field name |
| WASM rebuild required | `npm run build:wasm` in tasks |

## Migration Plan

1. Add `ParamDisplayNames.hpp` + unit test or constexpr size checks.
2. Desktop + Delay backend.
3. WASM bindings + rebuild.
4. Web blurbs/hints.
5. Quick Dict + sync.
6. Visual verify six panels + web six pages.

## Open Questions

- None. SRR 1/2 kept abbreviated (user-facing full name still clearer than `SRR1`).
