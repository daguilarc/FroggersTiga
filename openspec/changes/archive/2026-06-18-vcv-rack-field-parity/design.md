## Context

```
Web / desktop (reference)                 VCV today (shipped)
─────────────────────────                 ─────────────────────
6 submodule regions visible               4 voicing cols + FX ✓ (topology OK)
Per-page Randomize + Randmod              ✗ missing
Global: Rand All | Rand Mods |            ✗ missing (one TL1105 mislabeled)
         Random | Rand waveforms
VCO morph buttons (Audio rows 0–2)        ✗ missing
Mod rack: 5 cells (3 scopes + 2 LEDs)     ✗ 3 cells, missing CC1/CC2 outs
Faceplate labels readable                 ✗ was <text> (fix in progress)
Random → marbles step                     ✗ wired to RandomizeAllPages()
```

Archived `vcv-vst-field-parity-panel` Phase A–C tasks are marked complete, but manual gates 4.6, 5.1, 5.5 and panel-ux 5.3 were never signed off. Users opening Rack see knobs with **no labels**, **no randomize buttons**, **no wave morph UI**, and a **truncated mod rack** — the module does not match the sim UI contract.

## Goals / Non-Goals

**Goals:**

- One merged apply track: silkscreen + randomize + VCO morph + five-cell mod rack (LED-only) + wiring fix + verification
- Single label authority: `ParamDisplayNames.hpp`; single layout authority: `VcvPanelLayout.hpp` + `FieldParityWidget`
- Path-based SVG silkscreen (typical VCV workflow; nanosvg-safe)
- Full randomize surface matching web/desktop mutation map
- VCO morph buttons on Audio column only (page 0, rows 0–2)
- Five-cell mod rack with correct mod indices and LED feedback (no oscilloscopes on VCV)
- Rack smoke gate before any panel task marked done

**Non-Goals:**

- VST **visual** rework (local-only; JUCE UI already matches desktop field parity)
- Web sim changes
- Publishing `vcv/` or VST sources to GitHub main
- CV scope / oscilloscope widgets on VCV **mod rack** (LED-only mod rack stays)
- Mod-rack time-series CV traces sampling `GetCvOut()` over time

**In scope for audit only (tracked, not VCV apply):**

- VST plugin-host UX gaps (Record cluster, keyboard capture, preset snapshot completeness)
- VST manual DAW verification gate (archived task 2.5)

## Postmortem — what predecessors did wrong

| Failure | Evidence | Rule |
|---------|----------|------|
| Invisible silkscreen | SVG `<text>`; task 5.3 never checked in Rack | Verify **rendered output** in target host |
| False completion | field-parity 4.x marked done; user report contradicts | Manual gates block close |
| Missing randomize UI | No `RandomizePage` buttons in `plugin.cpp` | Map web/desktop controls explicitly |
| Miswired Random | `RANDOM_PARAM` → `RandomizeAllPages()` | Match desktop `PressButton(0)` |
| Missing VCO morph | No wave buttons in `FieldParityWidget` | Audio page rows 0–2 only |
| Truncated mod rack | 3 outputs vs desktop 5 | Match cell count + outputs; LEDs not scopes |
| Duplicate label tables | `generate_panels.py` PAGES/ROWS | Parse `ParamDisplayNames.hpp` |
| Detached SVG coordinates | CC labels at wrong Y vs widgets | Layout constants only |
| **Header additive offset** | `primaryIoY = header + panelH - margin` pushes I/O **4 px past** panel bottom; row labels at knob **center** Y | Recompute grid; anchor bottom from panel bottom |
| **Magic label offsets** | Python-only `11.0` mm, `±0.55` grid | Named constants in `VcvPanelLayout.hpp` |
| Copy-paste mod rack | 3 hardcoded output arrays; triplicate `updateModLights` | One `kModRackCells[]` table, one loop |
| Dual layout authority | C++ + Python both compute row/mod Y | `VcvPanelLayout.hpp` only; Python reads constants |
| Miswired Random shipped | `plugin.cpp:361–366` calls Rand All | Action table maps Random → `ButtonCallback(0)` |
| Redundant CV export | `CV_OUT*` + mod rack both write `GetCvOut(4/5)` | Remove bottom outs; mod rack sole export |
| False mod-rack label count | SVG "Random" in mod row; widget has TL1105 there | Random in global strip only; 5 mod cells |

## Decisions

### D1 — Merged change supersedes `vcv-panel-silkscreen-fix`

**Choice:** Continue silkscreen work under this change; archive `vcv-panel-silkscreen-fix` after apply.

**Status:** Path generator exists but **layout geometry is wrong** (user screenshot: labels on jacks, bottom row clipped, empty primary band). Tasks 1.4–1.5 must revert to open until D15–D17 pass.

### D15 — Header strip geometry (fix silkscreen regression)

**Problem:** `kHeaderStripGridY` was applied as a **uniform Y delta** to row centers **and** to bottom-anchored I/O:

```
WRONG (shipped)                         RIGHT
───────────────                         ─────
header                                  header (SVG + empty band)
+ rowStep unchanged                     + rowStep = usableHeight / (rows+2)
+ ioY = header + (panelH - margin)      + ioY = panelH - margin  (no header term)
  → jacks clip past y=380                 → bottom row + I/O fit above screw band
```

**Choice:**

- `contentTopY = kHeaderStripGridY * RACK_GRID_WIDTH`
- `contentBottomY = RACK_GRID_HEIGHT - kBottomIoMarginGrid * RACK_GRID_WIDTH` (new constant)
- `rowStepY = (contentBottomY - contentTopY) / (kRows + 2)`
- `rowCenterY(row) = contentTopY + rowStepY * (1.5 + row)`
- `primaryIoY = RACK_GRID_HEIGHT - kBottomIoMarginGrid * RACK_GRID_WIDTH` — **not** header-offset

Apply the **same formulas** in `FieldParityWidget.hpp` and `generate_panels.py` (Python reads constants from header).

### D16 — Silkscreen label anchors (no magic mm)

**Choice:** Add to `VcvPanelLayout.hpp`:

- `kRowLabelOffsetMmX` — left of column center (replace hardcoded `11.0`)
- `kRowLabelOffsetMmY` — above knob center (replace baseline-at-center)
- `kPortLabelOffsetGridY` — above/below jack/switch centers (replace `±0.55/0.65/0.75` scattered in Python)

Generator reads these; **no numeric literals** in `generate_panels.py` except unit conversion.

### D17 — Widget bbox CI (not center-only)

**Choice:** Extend `check_vcv_panel_bounds.sh`:

- Last row knob bottom (`center + kKnobRadiusGrid * GRID`) ≤ `RACK_GRID_HEIGHT - screw band`
- I/O jack bottom ≤ `RACK_GRID_HEIGHT`
- Optional: row label anchor left of knob center by at least `kRowLabelOffsetMmX`

Manual gate 1.7 remains mandatory: 100% zoom screenshot before any silkscreen task closes.

### D2 — Static path SVG (not runtime `ui::Label`)

**Choice:** `fontTools` outlines in `generate_panels.py`; anchors from `FieldParityWidget` math.

**Why:** Typical VCV module pattern; CI can grep for `<text>`. Wave morph uses one nanovg custom widget; mod rack uses Rack lights only.

### D3 — Randomize control layout

**Choice:** Two-tier layout on main module:

```
┌─ header: [frog] FroggersTiga ─────────────────────────────┐
├─ global strip (below header, above mod rack):               │
│  [Rand All] [Rand Mods] [Rand Resample] [Rand waveforms]         │
├─ mod rack: CC1 | CC2 | VCO Env | Rand1 | Rand2  (5 cells)  │
├─ voicing columns — each column title row:                   │
│  [Randomize] [Randmod]  above knob column                   │
│  knobs + mod jacks … (Audio rows 0–2 include wave btn)     │
└─ I/O + CC enable row ──────────────────────────────────────┘
```

**Preferred:** Global strip holds **Rand Resample** (marbles step); mod rack row is five output cells only (no duplicate marbles toggle).

### D4 — Engine wiring map

| Control | Web / desktop | VCV call |
|---------|---------------|----------|
| Randomize (page knobs) | `randomizePage` | `host.RandomizePage(page)` |
| Randmod | `randomizePageMod` | `host.RandomizePageMod(page)` |
| Rand All | `randomizeAll` | `host.RandomizeAllPages()` |
| Rand Mods | `randomizeMod` | `host.RandomizeAllMod()` |
| Rand Resample (marbles) | `marbles` / `PressButton(0)` | `ButtonCallback(0)` via engine |
| Rand waveforms | `randomizeMorphs` | `host.RandomizeVcoMorphs()` |
| VCO morph click | `cycleVcoMorph` | `host.CycleVcoMorph(index)` |
| Delay Randomize | `delayRandomizeKnobs` | `delay.randomizeKnobs()` |
| Delay Randmod | `delayRandomizeMod` | `delay.randomizeMod(bridge)` |

Use rising-edge detection on momentary buttons — consolidated handler, no copy-paste per button.

### D5 — Layout authority for new controls

**Choice:** Add constants to `VcvPanelLayout.hpp`:

- `kGlobalStripGridY` (below header, above mod rack)
- `kColumnActionRowGridY` (per-column Randomize/Randmod)
- `kModRackCellCount = 5`, `kModRackCellSpacingX`
- `kWaveMorphGridOffsetX` (tail of Audio rows 0–2)
- FX equivalents

`generate_panels.py` and `FieldParityWidget` both read these — no third coordinate table.

### D6 — Verification gates (non-negotiable)

1. `sim/check_vcv_panel_svg.sh` — no `<text>`
2. `sim/check_vcv_panel_bounds.sh` — widgets fit HP
3. Manual Rack 100% zoom — labels + buttons + wave morph + mod LEDs visible
4. Randomize matrix: spot-check each action vs desktop
5. VCO morph: click cycles waveform; icon updates
6. Mod rack: five outputs patch correctly; LEDs respond at threshold 0.55; Rand1/2 step on **Random** press

### D7 — VCO morph widget (wave icon, not mod-rack scope)

**Choice:** Custom `WaveMorphWidget` (nanovg) placed left of knob on Audio rows 0–2 only.

**Looks like a tiny oscilloscope; behaves like web/desktop morph icon:**

```
Mod rack scope (BANNED on VCV)     Wave morph icon (ALLOWED on VCV)
──────────────────────────────     ────────────────────────────────
Ring buffer of GetCvOut samples    24-point static path, one cycle
Updates every audio block          Updates when morph param changes
Shows CV activity over time        Shows sine↔saw↔square blend shape
```

Web `waveSvg(morph)` and desktop `WaveMorphButton` both call the same morph eval over phase `t ∈ [0,1]` — not audio input, not mod CV.

**Behavior:**

- Reads `host.GetVcoMorph(index)` each frame; draws morph wave path via `VcoWaveEval` / shared eval (24 segments, matches web stroke)
- Click → `host.CycleVcoMorph(index)` on mouse-up
- Size ~28×28 px equivalent in grid units
- No silkscreen path on the widget itself (the stroke is the label)

**Reference:** `desktop/Source/WaveMorphButton.cpp`, `web/src/main.ts` `waveSvg()`.

### D8 — Mod rack five-cell layout (LED-only on VCV)

**Choice:** Replace current 3-output mod row with 5 cells; **all cells use green LED** at threshold 0.55. No CV scope widgets.

| Cell | Mod index | VCV visual | Output |
|------|-----------|------------|--------|
| MIDI CC 1 | 0 | Green LED | `MOD_CC1_OUTPUT` (new) |
| MIDI CC 2 | 1 | Green LED | `MOD_CC2_OUTPUT` (new) |
| VCO Envelope | 4 | Green LED | `MOD_VCO_ENV_OUTPUT` (existing) |
| Random 1 | 5 | Green LED | `MOD_RANDOM1_OUTPUT` (existing) |
| Random 2 | 6 | Green LED | `MOD_RANDOM2_OUTPUT` (existing) |

**Host split (intentional):**

```
modIndicatorModeForVcv()           → LedOnly      ← correct for VCV; keep
modIndicatorModeForDesktopOrVst()  → ScopeAndLed  ← scopes stay on JUCE/web only
```

**CV_OUT1/CV_OUT2:** Remove from VCV main module. Today they duplicate mods 4/5 while mod rack already exposes VCO Envelope and Random 1. **MIDI Out** button and `tickMidiOut()` remove likewise — envelope export is mod rack VCO Envelope CV → user patches to a Rack MIDI-CV module if needed. Main **audio in/out**, **CV/gate inputs**, and **MIDI In** stay.

**HP:** Five mod cells may still require bump to **84 HP**; bottom I/O row shrinks after removing CV outs + MIDI out.

### D9 — No mod-rack oscilloscopes on VCV (product decision)

**Choice:** Field parity on VCV mod rack = topology + labels + LED activity — **not** `CvScopeDisplay`-style CV traces.

**Exception:** VCO morph **wave icons** on Audio rows 0–2 are allowed. They reuse the same static morph preview as web/desktop; they are not mod-rack scopes and do not sample `GetCvOut()`.

### D11 — No redundant external CV/MIDI outs on VCV

**Choice:** Remove bottom-row `CV_OUT1`, `CV_OUT2`, and MIDI Out from the main module. Mod rack is the sole patch point for VCO Envelope (mod 4), Random 1 (mod 5), and Random 2 (mod 6).

**Rationale:** Desktop exposes MIDI Out for hardware VCO-envelope CC export; Daisy firmware maps CV outs to mods 4/5. In VCV Rack, users patch mod-rack jacks directly — duplicate bottom outputs add clutter and overlapped silkscreen.

**Keep on main module I/O row:** audio in, audio out, CV1–4 in, gate in, MIDI In, CC enable toggles.

**Remove:** `CV_OUT1`, `CV_OUT2`, `midiOutput`, `tickMidiOut()`, MIDI Out `MidiButton`, related SVG labels at grid 17.5/19.5 and 21.5.

### D12 — Option C: stereo FX routing via `applyStereoBus` (when expander linked)

**Choice:** Reuse existing stereo delay/reverb architecture from `sim/DelayState.hpp` — same as desktop `AudioEngine`. No new dry tap on `FroggersEngine`.

```
StereoDelay.process() → wet.l, wet.r (width/detune)
                      → folded to mono in engine chain
Reverb → stereo deltas (getReverbStereoDeltaL/R)

Desktop / VCV Option C:
  coreMono = ProcessBlock output (delay + reverb in mono)
  FX L = coreMono + dmix·delayΔL + rvMix·revΔL
  FX R = coreMono + dmix·delayΔR + rvMix·revΔR
  Main out = coreMono
```

**No FX expander:** main `audio` out = full mix only (standalone).

**With FX expander (Option C patch):**

```
FX [L out] ──► mixer L  ┐  primary stereo path (L ≠ R when width > 0)
FX [R out] ──► mixer R  ┘
Main [audio out] ──► optional mono fold, or leave unpatched
```

**Do not** patch main + FX L + FX R to the **same** mixer bus — mono core would accumulate.

**Remove:** `setStereoOutputs(sameVoltage)` duplicate. **Add:** per-sample or per-block `makeStereoFxSpread(delay, host.m_engine.getReverbStereoDeltaL(), …)` then apply L/R voltages on FX module.

**Requires:** engine exposes reverb stereo deltas to VCV primary (already on `FroggersEngine` — same calls as desktop).

**Not in scope:** changing desktop/VST integrated stereo bus.

### D13 — Data-driven mod rack and randomize tables

**Choice:** Define mod rack and randomize wiring from static tables in `plugin.cpp` (or a small header), not copy-paste per cell/button.

**Mod rack cell table** (one source for widget placement, `process()` voltage, and LED update):

```cpp
struct ModRackCell { uint8_t modIndex; int outputId; int lightId; };
static constexpr ModRackCell kModRackCells[] = {
    {0, MOD_CC1_OUTPUT, LIGHT_MOD_CC1},
    {1, MOD_CC2_OUTPUT, LIGHT_MOD_CC2},
    {4, MOD_VCO_ENV_OUTPUT, LIGHT_MOD_VCO_ENV},
    {5, MOD_RANDOM1_OUTPUT, LIGHT_MOD_RANDOM1},
    {6, MOD_RANDOM2_OUTPUT, LIGHT_MOD_RANDOM2},
};
```

Widget ctor, `process()`, and `updateModLights()` iterate `kModRackCells` — no triplicate `GetCvOut` lines, no hardcoded `modOutputs[3]`.

**Randomize action table** (one rising-edge handler):

```cpp
struct RandomizeAction { int paramId; void (*onEdge)(PagedHostIO&, DelayState&); };
```

Global strip + column actions register in one table; `process()` calls one `dispatchRandomizeEdges(params, actions, count)` — no per-button `if` blocks.

**Column sync:** Extract shared knob+mod-jack loop from `syncVoicingColumn` / `syncColumn` into one function taking page index + param/input base offsets; FX delay branch stays a single `if (hostPage == kDelayPage)` inside the loop body.

**Cleanup:** Remove unused `primaryPanelSize()`; remove `CV_OUT1/2` enum entries and duplicate voltage writes; remove mod-rack-row TL1105 when global strip owns Random.

### D14 — Single layout authority (`VcvPanelLayout.hpp`)

**Choice:** All new grid anchors live in `VcvPanelLayout.hpp`:

- `kGlobalStripGridY`, `kColumnActionGridY`, `kModRackCellCount = 5`, `kWaveMorphGridOffsetX`

`FieldParityWidget` and `generate_panels.py` consume these constants — Python parses the header (existing pattern for HP/row counts) instead of re-deriving `headerOffsetY + 1.5 * grid` inline.

**CI:** `check_vcv_panel_bounds.sh` validates global strip, column action row, 5-cell mod rack, and wave-morph tail positions against the same constants.

**Reject:** Third coordinate tables in `generate_panels.py` that mirror C++ math line-for-line.

### D10 — VST as reference host (audit 2026-06-14)

**Architecture:** `FroggersTigaAudioProcessor` → `AudioEngine{true}` → `MainComponent(externalEngine)` → same widgets as standalone desktop.

**Confirmed present on VST (no work needed for field-parity visuals):**

- `SubModulePanel`: per-row `juce::Label`, Randomize/Randmod, wave morph on Audio page 0
- `GlobalStrip`: Rand All, Rand Mods, Rand waveforms, Random (marbles)
- `ModRackPanel`: five `ModModuleBox` cells; mods 0,1,4 → `CvScopeDisplay`; mods 5,6 → LED
- `MidiSettingsComponent(ccControlsOnly=true)`: CC1/CC2 channel + CC + enable toggles
- `PluginEditor`: `setResizable(true)`; min 1024×600 (drift from 1440×720 intent); default 1440×720

**VST gaps (plugin-host UX — tracked in `vst-plugin-host-ux`):**

| Gap | Evidence | Severity |
|-----|----------|----------|
| Record cluster visible in DAW | `MainComponent::initFromEngine` hides Play/Stop/Audio when hosted; **`m_recordCluster` not hidden** | Medium |
| QWERTY MIDI when hosted | Plugin does not call `openDefaultMidi()` on startup; gap when user enables Computer Keyboard — **`shouldCaptureQwertyMidi()` needs hosted guard** | Medium |
| Preset omits morph + CC config | `SimPresetSnapshot` v1 only; **`read()` rejects `version != 1`** | Medium |
| Editor min size drift | `setResizeLimits(1024, 600, …)` vs `HostPanelLayout` 1440×720 | Low |
| Manual DAW gate never run | `vst-plugin-host-ux` tasks 6.1–6.2 open | High (verification) |
| VCO Envelope ≠ PM3 confusion | scope shows mod 4 CV; row 7 knob is Phase mod 3 DSP | Doc only — already in `SIM_MANUAL.md` |

**VCO Envelope scope semantics (all scope hosts):** `FroggersEngine::UpdateM5FromVco` writes slow envelope of |VCO mix| into `m_mods[4]`. The trace is **mod CV activity**, not audio waveform and not the Phase mod 3 knob position. Tooltip on `CvScopeDisplay`: "Mod CV trace (not audio)".

**How other devs do it:**

- VCV: SVG paths for static silkscreen ([VCV Panel manual](https://vcvrack.com/manual/Panel)); LED-only mod rack — no scope widgets
- JUCE VST: runtime Labels in `resized()`; FlexBox/Grid recommended for resize ([JUCE forum](https://forum.juce.com/t/getting-started-best-practice-for-resizable-dynamic-gui-layout/26193)) — FroggersTiga uses fixed pixel layout; acceptable at default 1440×720, spot-check at min size

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| 72 HP too crowded | Bump to 84 HP; update bounds check + SVG |
| HP growth breaks patches | Version bump + migration note in DEVELOPMENT.md |
| Button silkscreen clutter | Abbreviations match desktop |
| Triple mono duplicate on FX L/R | Document Option C patch; apply stereo bus |
| Regress silkscreen fix | Run SVG CI on every build |

## Migration

1. **Fix layout geometry (D15–D17)** — blocking; user screenshot shows silkscreen unusable until this lands
2. Implement randomize controls + rewire Random
3. Add VCO morph widgets on Audio rows
4. Expand mod rack to 5 cells with LEDs; remove CV_OUT/MIDI out
5. Option C stereo FX routing on expander (`applyStereoBus`)
6. Regenerate SVGs; `./build.sh --install`; user Rack pass
7. Archive `vcv-panel-silkscreen-fix`; tag `vcv-field-parity-v2.6`

## Open Questions

1. HP width: 72 vs 84 — resolve during task 2.3 mockup (five mod cells + global strip + column actions likely force 84)
2. Bottom CV_OUT1/2: **removed** — mod rack only (D11)
3. `syncVoicingColumn` / `syncColumn` merge: shared helper vs inline duplication — **shared helper** (≥2 passes, domain boundary: column sync)
