## Context

FroggersTiga v1 desktop already initializes **stereo output** (`AudioEngine`: `initialiseWithDefaultDevices(0, 2)` with output bits 0+1 set). The DSP core renders a **mono** synth bus (`FroggersEngine::ProcessBlock` → `m_monoBlock`); `applyStereoBus` in `sim/DelayState.hpp` applies delay/reverb stereo spread to L/R, or **downmixes to mono** when `numOutputChannels < 2`. VST v1 declares **mono input, stereo output** (`PluginProcessor.cpp`).

The Sheaf miniapp provides the encoder-ring / scene / gesture / shift UX reference. FroggersTiga has no scene/gesture storage today; pair-AR on Audio is four knobs (Atk/Rel × two pairs) via `AudioPairArState`.

## Goals / Non-Goals

**Goals:**
- v2 fork with encoder rings, module carousel (7 modules), scenes/**two gestures**/shift, mod grid, scopes, global Crunchy, ADSR page, **full step sequencer**.
- Stereo-default desktop and VST v2 output; mono output remains supported.
- Document interaction matrix and module vs scene vocabulary before implementation.

**Non-Goals:**
- Full desktop-v2 chrome on web (see `web-v2-parameter-subset` — web gets parameters + global Crunchy only).
- Daisy firmware, VCV/Firmware migration.

## Decisions

### 1. Parallel `desktop-v2/` tree
New target `FroggersTigaDesktopV2`; v1 unchanged.

### 2. Control core (Sheaf patterns, local implementation)
`FroggersV2ControlCore` in `desktop-v2/Source/control/`; bridge to `DesktopHostIO` on audio thread.

### 3. Modulation math — unified in v2.0
v2.0 adopts the Smart Grid range-preserving depth rule on the control core **and** applies the same effective values through `FroggersV2HostBridge` so encoder min/max arcs match audible modulation. No hybrid crossfade-only bridge that diverges from arc display.

Implementation: control core owns signed depths and range normalization; bridge writes final per-row effective values into `PageManager` / `ModMgr` (or replaces per-row read path on v2 hosts) so `Get(param)` and arc min/max use one representation.

### 4. Eight internal mod sources (indices 7–14)
Per `desktop-v2-mod-source-grid` spec.

### 5. Encoder ring voice model (**proposed default**)
FroggersTiga is mono-synth; rings use **scene endpoints as pseudo-voices**:

```
        ╭─────── Scene L ring (outer)
        │  ╭──── Scene R ring (inner)
        │  │    ● blended current dot
        ╰──╯
```

- `voiceCount = 2` for ring geometry; `values[0]` = scene L effective, `values[1]` = scene R effective, indicator dot = post-blend center sent to bridge.
- Modulator badges (upper) and gesture badges (lower) from `modulatorsAffectingMask` / `gesturesAffectingMask`.
- **v2.0:** min/max reachability arcs from `ProcessLite` slewed min/max; arcs MUST match bridged effective values (see decision 3).

### 6. Interaction matrix (**proposed default**)

| Context | Drag ring | Press | Shift+press |
|---------|-----------|-------|-------------|
| Normal view | Edit scene-blended center (or depth if row has mod route and drag-on-mod semantics match v1) | Open mod-depth view | Revert param + depths to default |
| Mod-depth view | Edit assigned depth | Close view (target cell) | Revert target param |
| Shift held | **No turn** (Sheaf rule) | — | Revert |
| Global Crunchy | Edit global master | — | Revert Crunchy |
| Per-page Crispy | Edit page-local fuego | — | Revert that page's Crispy only |

**Crispy/Crunchy exceptions:** Shift+revert on global Crunchy does not scramble other params. Per-page Crispy shift+revert affects only that page's Crispy row. Global Crunchy applies fuego to **all** rows including every Crispy instance; page Crispy then stacks a second fuego pass on that page's musical rows.

**Rand while gesture active:** Rand All/Mods clears gesture selection first, then randomizes (deterministic ordering in bridge).

### 7. Module carousel vs scenes
- **Module carousel** (UI label "Module"): host pages 0–6 (Audio … Delay, ADSR). Arrows wrap 0↔6.
- **Scene strip** (UI label "Scene"): ordinals S1/S2/S3; `SceneSelect(ix)` updates less-selected endpoint; blend slider morphs L↔R.
- Scene storage is **global** — all modules share the same scene endpoints per parameter ID.

### 8. ADSR module page (host page 6)
Replaces v1 Audio pair-AR band (`AudioPairArLayout`).

| Row | Parameter |
|-----|-----------|
| 0–2 | Attack VCO1, VCO2, VCO3 |
| 3–5 | Sustain VCO1, VCO2, VCO3 |
| 6–8 | Release VCO1, VCO2, VCO3 |
| 9 | Crispy (page-local fuego for ADSR rows 0–8; also subject to global Crunchy as a Crispy instance) |

`kAdsrNumRows = 10`. Engine: `VcoAdsrState` — **full gated ADSR per VCO** (attack → sustain hold while gate high → release on gate off). Gate from MIDI note, sequencer step gate, or `DesktopHostIO`.

### 9. Global Crunchy + per-page Crispy (stacked fuego)

v2 keeps v1 per-page **Crispy** on every module and adds **global Crunchy** in the strip. **Crunchy is global fuego for everything.** Crunchy does **not** replace page Crispy.

| Control | Location | Role |
|---------|----------|------|
| **Crunchy** | Global strip encoder | Global fuego on **all** persisted rows on **all** pages — musical rows **and** every Crispy instance |
| **Crispy** | Row 7 on Audio (0); row 9 on expanded modules 1–5 and ADSR (6) | Page-local fuego on that page's musical rows only (v1 FUEG semantics), applied **after** global Crunchy |

**Crispy row authority:** `V2ParamDisplayNames` (or a v2 `HostPanelLayout` extension) SHALL expose `CrispyRowForPage(hostPage)` — a single O(1) lookup table. Rand All, fuego pipeline, encoder banks, and bridge paths read from this table only; no scattered row-index conditionals per module.

| hostPage | Crispy row |
|----------|------------|
| Audio (0) | 7 |
| Random–Delay (1–5) | 9 |
| ADSR (6) | 9 |

**Fuego pipeline (per musical row on a page):**

```
modulatedCenter = SmartGridEffective(row)
afterCrunchy    = Fuegoize(modulatedCenter, globalCrunchy, row)
pageCrispyAmt   = Fuegoize(pageCrispyKnob, globalCrunchy, crispyRow)   // Crispy instances get Crunchy too
musicalRowOut   = Fuegoize(afterCrunchy, pageCrispyAmt, row)
```

- Global Crunchy at 0 → no global fuego pass; page Crispy alone behaves like v1.
- Global Crunchy non-zero → **every** knob including all Crispy knobs receives global fuego.
- Crunchy and each page Crispy are independently moddable (`Global/Crunchy`, `Module/<Page>/Crispy`, `ADSR/Crispy`).

### 10. Module page row layout

| Page | Rows | Notes |
|------|------|-------|
| **Audio (0)** | unchanged | No v2 expansion rows |
| **Random, Reverb, Filter, Drive, Delay (1–5)** | **10** | v1 rows 0–6 + two new (7–8) + Crispy (9) |
| **ADSR (6)** | **10** | rows 0–8 A/S/R + Crispy (9) |

**v2 expansion (+2) labels** (pages 1–5 only):

| Page | Row 7 | Row 8 |
|------|-------|-------|
| Random | Spread | Bias |
| Reverb | Mod depth | Hold |
| Filter | Comb/Peak | Scoop |
| Drive | Blend | Phase |
| Delay | Color | Halo |

### 10b. Filter parallel topology (v2 only)

v1 serial chain: `PureDelay → Comb → Peak`. v2 replaces this with **parallel** paths on `DesktopV2` / `VstV2`:

```
                    ┌─ PureDelay → Comb ────────┐
  in ───────────────┤                           ├─ Comb/Peak (row 7) ── Scoop (row 8) ── out
                    └─ ResonantBump (Peak) ─────┘
```

- Existing rows 0–6 retain v1 roles (Comb offset/delay front, Peak freq/gain/Q, Comb delay/feedback/LP).
- **Comb/Peak** (row 7): crossfade between parallel path outputs.
- **Scoop** (row 8): notch depth at/near Peak freq on the mixed output (not labeled "Notch" in UI).

### 11. Hardware encoder banks
When physical encoder count < visible rows (e.g. 4 encoders, 7–10 rows), `BankSlot` paging applies (Sheaf pattern): bank buttons or auto-page within module. Daisy Field 8-knob mapping: one bank covers 7 module rows + bank-switch for remainder.

### 12. Stereo audio I/O
- **Desktop v2:** `initialiseWithDefaultDevices(0, 2)`; stereo output default; user can select mono output device in Audio Settings → `applyStereoBus` downmixes.
- **VST v2:** `BusesProperties` mono in + stereo out (match v1); `isBusesLayoutSupported` accepts mono output (downmix) and stereo output.
- **Internal processing:** unchanged mono core + stereo spread.

### 13. VST v2 parameter inventory
Generated manifest: module knobs, ADSR, depths, global Crunchy, scenes/gestures, sequencer, delay, morphs. **Dual IDs:** each entry has a flat `stableId` (automation/MIDI) and a grouped `displayName` (`Module/Audio/VCO1`, `Global/Crunchy`, `ADSR/AtkVCO1`, `Sequencer/BPM`).

### 14. Sequencing (v2.0)
Full step sequencer: BPM transport, pattern length 4–64, per-step scene L/R + gesture snapshot capture, playhead recall, optional external MIDI clock sync. Sequencer provides per-step gate for `VcoAdsrState`. Global strip exposes Play/Stop, record arm, BPM, pattern length; desktop adds sequencer panel. VST exposes sequencer params with dual IDs per §13.

### 16. Host surface split (desktop v2 vs web)

| Feature | DesktopV2 / VstV2 | Web (`SimHostKind::Web`) |
|---------|-------------------|--------------------------|
| Module +2 rows (pages 1–5) | Yes | **Yes** |
| Filter parallel comb/peak + Scoop | Yes | **Yes** (shared engine) |
| Global Crunchy | Yes | **Yes** |
| Per-page Crispy + Crunchy stack | Yes | **Yes** |
| ADSR module page | Yes | **No** (pair-AR stays on Audio) |
| Encoder rings, scenes, gestures, shift | Yes | **No** |
| Sequencer | Yes | **No** |
| v2 eight-source mod grid | Yes | **No** (v1 four-cell mod bay) |

Web keeps page pills, rotary knobs, and mod dropdowns. Engine/DSP changes are shared; control chrome is not.

| Risk | Mitigation |
|------|------------|
| Math unification scope | Bridge tests assert arc min/max == effective value bounds |
| ADSR DSP scope | `VcoAdsrState` gated ADSR per VCO; unit tests before UI |
| Sequencer + scene storage | Step buffer isolated from unstored scene metadata |
| 10-row ADSR page layout | Taller carousel panel or scroll |
| Scene/global Crunchy Rand All scope | Spec: Rand All skips scene metadata |

## Migration Plan

### Branch and merge policy

All v2 work lands on a **long-lived feature branch** (e.g. `froggerstiga-desktop-v2`), **not** `main`. The branch is the integration fork for **desktop v2, web parameter subset, and shared engine changes** until manual QA passes locally.

| Phase | Where | Gate |
|-------|--------|------|
| Development | Feature branch | Opt-in CMake: `BUILD_DESKTOP_V2`, `BUILD_VST_V2` |
| CI on branch | Same branch | v2 targets **OFF**; v1 sim tests + default builds must stay green |
| Local QA | Developer machine | Desktop v2 app, web build, VST v2 artefacts, **Playwright e2e** (`web/npm run test:e2e`) |
| Merge to `main` | After sign-off | User approves; v1 default build path unchanged until then |

**Do not merge to `main`** until desktop v2, web expansion + Crunchy, and shared engine paths are tested locally. Production web deploy from `main` continues serving v1 until merge.

### Build and test steps

1. `BUILD_DESKTOP_V2=ON` (OFF in CI).
2. `BUILD_VST_V2=ON` (OFF in CI).
3. Extend `ParamDisplayNames` or add `V2ParamDisplayNames.hpp`.
4. v1 pair-AR tests remain; add `VcoAdsrState_test`, `V2ModuleExpansion_test`, `V2FilterParallel_test`.
5. Manual QA: desktop v2 carousel + ADSR + Crunchy; web pages 1–5 + Crunchy; stereo headphones + mono output device + DAW VST v2.
6. Playwright e2e: expanded module labels, ten-row pages 1–5, Crunchy in global strip; existing v1 e2e specs remain green.

## Decisions (locked)

| Topic | Decision |
|-------|----------|
| Encoder min/max arcs | v2.0 with unified Smart Grid math |
| Crispy/Crunchy | Global Crunchy fuego on all rows incl. Crispy instances; per-page Crispy stacks on musical rows |
| Module expansion | +2 rows pages 1–5; Filter parallel comb/peak + Scoop; Audio unchanged |
| ADSR DSP | Full gated ADSR per VCO (1B) |
| Sequencing | Full sequencer in v2.0 |
| Gesture lanes | 2 at launch (G1/G2) |
| VST parameter IDs | Both flat `stableId` + grouped `displayName` |
