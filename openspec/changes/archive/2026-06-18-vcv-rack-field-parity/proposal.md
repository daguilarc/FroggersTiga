> **Superseded by `omni-repository-harmonization`.** Field-parity scope is not implemented as written; surviving VCV requirements are represented in omni §3–§4. Archive with `--skip-specs`.

## Why

Prior VCV changes (`vcv-vst-field-parity-panel`, `vcv-rack-panel-ux`, `vcv-panel-silkscreen-fix`) marked implementation tasks complete while the shipped module is still unusable at a glance:

- **Invisible silkscreen** — nanosvg skips SVG `<text>` (path fix landed; **alignment still broken**)
- **Silkscreen misaligned / clipped** — header strip added by **shifting all Y coords down** without recomputing row pitch or bottom anchor; labels use **magic mm offsets** not in `VcvPanelLayout.hpp`; CI checks center points only, not widget extent
- **No randomize controls** — web/desktop expose six randomize actions; VCV has one miswired toggle
- **No VCO waveform morph UI** — desktop `WaveMorphButton` on Audio rows 0–2; web `vco-morph-btn`; VCV has nothing
- **Incomplete mod rack** — desktop/web have five mod cells with patch outputs; VCV has three outputs and no MIDI CC mod jacks

Field parity on VCV means a user can **read** the panel, **drive** the same mutations, **cycle VCO morphs**, and **patch the same mod topology** without hover tooltips. VCV does **not** get desktop-style CV scope traces — **LED-only mod feedback stays on VCV** by explicit product decision.

This change **merges** silkscreen fix (partially applied), randomize-control parity, VCO morph controls, and mod-rack visual parity into one apply track.

## What Changes

### Silkscreen + branding (from `vcv-panel-silkscreen-fix` — partially done)

- Path-based SVG silkscreen via `fontTools`; parse `ParamDisplayNames.hpp` + `VcvPanelLayout.hpp`; no live `<text>`
- Top header: frog logo + “FroggersTiga” on main and FX modules
- **Layout geometry fix (blocking):** header reserves top band; **recompute row step from usable height**; **bottom I/O row anchors from panel bottom** (not `header + panelHeight`); silkscreen label offsets are **named constants in `VcvPanelLayout.hpp`**, not hardcoded `11.0` mm in Python
- `sim/check_vcv_panel_svg.sh` + extended `check_vcv_panel_bounds.sh` (widget **bbox** + bottom-row clearance, not center-only)
- Complete manual Rack visual gate (100% zoom, light/dark) before close — **task 1.x not done until screenshot passes**

### Randomize controls (field parity gap)

- **Rewire** mod-rack `TL1105`: rising edge → marbles step (`ButtonCallback(0)`), matching desktop **Random**
- **Add** global strip: **Rand All**, **Rand Mods**, **Rand Resample**, **Rand waveforms** (labels from `ParamDisplayNames::forGlobalStrip`; see `audio-pair-ad-controls`)
- **Add** per voicing column (Audio, Random S&H, Filter, Drive): **Randomize** + **Randmod**
- **Add** on FX module: Delay **Randomize** + **Randmod**
- Silkscreen every new button from shared layout constants

### VCO waveform morph controls (field parity gap)

- **Add** clickable wave-morph widgets on Audio column rows 0–2 (VCO1–VCO3), matching desktop `WaveMorphButton` placement beside each knob/jack cluster
- Click → `host.CycleVcoMorph(index)`; display current morph via nanovg wave stroke (reuse `VcoWaveEval` logic shared with desktop)
- Global **Rand waveforms** already covered under randomize strip

### Mod rack topology parity (no oscilloscopes on VCV)

- **Expand** mod rack from 3 cells to **5 cells** matching desktop `ModRackPanel` patch points:
  - MIDI CC 1 (mod 0) — output jack + **green LED** (threshold 0.55)
  - MIDI CC 2 (mod 1) — output jack + **green LED**
  - VCO Envelope (mod 4) — output jack + **green LED**
  - Random 1 (mod 5) — output jack + **green LED**
  - Random 2 (mod 6) — output jack + **green LED**
- **No** `ModCvScopeWidget` or mod-rack CV trace widgets on VCV
- **Yes** `WaveMorphWidget` on Audio rows 0–2: static morph wave icon (same as web/desktop — looks like a tiny scope, draws one-cycle shape from `VcoWaveEval`, not live CV)
- **Remove** redundant bottom-row **CV_OUT1/CV_OUT2** and **MIDI Out** (mod rack is sole CV export for VCO Env / Random mods)
- **Option C stereo routing (when FX expander linked):** reuse desktop `makeStereoFxSpread` + `applyStereoBus` — main **audio out = mono core mix**; FX **L/R = coreMono + delay/reverb stereo deltas** (not identical copies; not a separate dry tap)

### Process / postmortem (why predecessors failed)

- **No task complete** without target-host verification (Rack screenshot at 100% or automated SVG gate)
- **Reject** “SVG file exists” or “compiles” as done for panel UX tasks
- **VCV mod rack:** LED-only feedback is intentional — do not add oscilloscopes to close this change
- Carry forward unfinished manual gates from archived field-parity (routing matrix, PM3 spot-check)

### Structural audit (2026-06-14 — code review of shipped VCV)

Silkscreen pipeline is largely done; **field-parity behavior is not**. The audit traced `plugin.cpp`, `FieldParityWidget.hpp`, and `generate_panels.py` against desktop/web reference hosts.

**Confirmed blockers (wrong or missing today):**

| Issue | Evidence | Field-parity impact |
|-------|----------|---------------------|
| **Random miswired** | `RANDOM_PARAM` → `RandomizeAllPages()` (`plugin.cpp:361–366`) | Marbles never fire from faceplate Random |
| **Mod rack 3/5 cells** | Outputs only mods 4,5,6; CC mods 0,1 ingested but not exported | Cannot patch CC1/CC2 like desktop |
| **No randomize surface** | One TL1105 in mod-rack row; no global strip or column actions | Cannot drive same mutations as web/desktop |
| **No VCO morph UI** | `WaveMorphWidget.hpp` absent; `addExpanderColumn` has no Audio-row branch | Cannot cycle morphs on VCV |
| **Redundant I/O** | `CV_OUT1/2` duplicate mods 4/5; `tickMidiOut()` + MIDI Out still present | Clutter; spec D11 violated |
| **Stereo FX = mono copy** | `setStereoOutputs(audioVoltage)` on FX expander | Option C stereo routing not applied |
| **SVG ≠ widgets** | `generate_panels.py` mod labels include misplaced "Random"; 4 labels vs 3 jacks | Silkscreen lies about layout |
| **Header strip broke geometry** | `headerOffsetY()` added to row Y **and** bottom I/O Y (`header + panelHeight - margin`); row step unchanged | Bottom jacks clip ~4 px past panel; Crispy row overlaps screw band; primary 24 HP band mostly empty with labels on wrong widgets |
| **Magic silkscreen offsets** | `label_x = cx_mm - 11.0`; `±0.55/0.65/0.75` grid fudge in Python only | Row labels sit on knob centers; long strings bleed into mod jacks; column titles drift |
| **False silkscreen completion** | Tasks 1.4–1.5 marked done; CI green; user screenshot shows unusable panel | Same verification-gap class as invisible `<text>` |

**Structural debt to fix during apply (prevents repeat failure):**

- **Dual layout authority** — coordinate math duplicated in `FieldParityWidget.hpp` and `generate_panels.py:130–154`; **plus undeclared magic mm offsets** (`11.0`, `0.55`, `0.65`, `0.75`) only in Python
- **Copy-paste mod rack** — hardcoded 3-wide output/light arrays; `updateModLights()` triplicates same line; spacing divides by 5 but places 3 widgets
- **Duplicate sync loops** — `syncVoicingColumn` and `syncColumn` share knob+mod-jack pattern without shared extraction
- **Dead code** — `primaryPanelSize()` defined, never called
- **Missing data tables** — mod rack cells, randomize actions, and I/O ports shall be table-driven single loops (design D13–D14)

Apply track adds section 11 tasks: one cell table for mod rack, one action table for randomize rising edges, layout constants only in `VcvPanelLayout.hpp`, CI bounds extended before marking panel tasks done.

### VST audit (reference host — local-only, not on public main)

VST reuses `MainComponent` verbatim via `PluginEditor` → same JUCE labels, randomize buttons, wave morph widgets, and five-cell mod rack with `CvScopeDisplay` as desktop. **VCV must converge toward this surface**, not the other way around.

**VST already passes field-parity visuals** (runtime `juce::Label` text — no nanosvg trap). Gaps are **plugin-hosting UX** and **unverified manual gates**, not missing knobs:

| Check | VST | VCV (today) |
|-------|-----|-------------|
| Row labels visible without hover | ✓ `SubModulePanel` labels | ✗ was SVG `<text>` |
| Randomize + global strip | ✓ `SubModulePanel` + `GlobalStrip` | ✗ one miswired toggle |
| VCO morph buttons (Audio 0–2) | ✓ `WaveMorphButton` | ✗ missing |
| Mod rack 5 cells (topology) | ✓ five outputs | ✗ 3 cells, missing CC outs |
| Mod rack scopes | ✓ `CvScopeDisplay` | **N/A — LED-only by design** |
| CC enable gating | ✓ core + CC-only MIDI dialog | partial / miswired ingest |
| DAW manual verification | ✗ task 2.5 never signed off | ✗ |

**VST-specific issues** — spun off to **`vst-plugin-host-ux`** (do not block VCV apply):

| Gap | VST audit finding |
|-----|-------------------|
| Record/Export in DAW | `m_recordCluster` not hidden in `initFromEngine` |
| QWERTY MIDI | Guard needed in `shouldCaptureQwertyMidi()` when hosted (even if user enables Computer Keyboard) |
| Preset recall | `SimPresetSnapshot` v1 only; `read()` rejects `version != 1` |
| Editor min size | 1024×600 limits vs 1440×720 design |
| Manual gates | 0/22 tasks; DAW smoke + A/B never signed off |
| VCO Envelope doc | Already in `SIM_MANUAL.md`; VST § extension only |

- Use VST as A/B reference when closing VCV tasks 8.3–8.6; VST hosting fixes live in the spin-off change only

**Industry comparison:**

- **VCV modules:** VCV manual requires SVG text → paths ([Panel guide](https://vcvrack.com/manual/Panel)); community confirms live `<text>` renders blank — FroggersTiga VCV hit this exactly
- **JUCE VSTs:** labels in `resized()` / `Label::attachToComponent`; resizable editor via `setResizable` + `setResizeLimits` — FroggersTiga VST follows this pattern correctly
- **Dynamic widgets:** VCV uses nanovg for wave-morph buttons only; mod rack uses Rack `SmallLight` LEDs — no scope widgets

Track VST plugin-host gaps in **`openspec/changes/vst-plugin-host-ux/`** (audit table synced 2026-06-14); use VST as manual A/B reference when closing VCV tasks 8.3–8.6.

### Supersedes

- Active: `vcv-panel-silkscreen-fix` (absorb remaining tasks; archive after merge)
- Archived context: `vcv-vst-field-parity-panel`, `vcv-rack-panel-ux`

## Capabilities

### New Capabilities

- `vcv-rack-field-parity`: Usable VCV main + FX — silkscreen, randomize, VCO morph, five-cell LED mod rack, wiring, verification
- `vcv-panel-silkscreen`: Path-based faceplate labels and header branding (subset; required by field parity)
- `vcv-randomize-controls`: Randomize / Randmod / global strip parity with web and desktop
- `vcv-vco-morph-controls`: Wave morph buttons on Audio VCO rows matching desktop/web
- `vcv-mod-rack-scopes`: Five-cell mod rack topology with LED-only feedback (filename retained; **no scopes on VCV**)
- `vst-plugin-host-ux`: VST plugin-hosted UX audit — reference parity confirmed; plugin-host gaps tracked separately from VCV apply
- `vcv-io-simplification`: Remove redundant CV_OUT1/2 and MIDI Out; mod rack is sole export for VCO Env and Random mods
- `vcv-stereo-fx-routing`: Option C — main mono core + FX L/R via `applyStereoBus` (consistent with stereo delay/reverb DSP)

### Modified Capabilities

- (none in `openspec/specs/` — VCV is local-only)

## Impact

- `vcv/res/*.svg`, `vcv/scripts/generate_panels.py`, `vcv/scripts/trace_frog_logo.py`
- `sim/VcvPanelLayout.hpp`, `sim/check_vcv_panel_bounds.sh`, `sim/check_vcv_panel_svg.sh`
- `vcv/src/plugin.cpp`, `vcv/src/widgets/FieldParityWidget.hpp`
- Reuse `sim/DelayState.hpp`: `makeStereoFxSpread`, `applyStereoBus` (already used by desktop)
- **New widgets:** `WaveMorphWidget.hpp` only (nanovg wave morph on Audio rows)
- **Structural:** `kModRackCells[]` + `RandomizeAction[]` tables in `plugin.cpp`; shared column-sync helper; remove dead `primaryPanelSize()`
- `vcv/plugin.json` (version bump; HP may grow 72 → 84+ for density)
- `vcv/DEVELOPMENT.md`, `.github/workflows/pages.yml`
