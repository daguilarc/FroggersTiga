# Implementation path

**Canonical change:** `desktop-v2-ux-and-sequencer` (merged `v2-ux-and-operator-docs` + `desktop-v2-ui-polish` on 2026-06-30).

```
Done (v2-ux §1–6) → B0 (grid) → B1 (scroll, no bank) → P (Pair-AR) → A (scene) → H (sequencer) → B2 (height unify) → C (visual) → D (mod) → E (font) → F (MIDI desktop) → G (docs) → I (release gates)
```

---

## Completed — merged from `v2-ux-and-operator-docs`

### Pair-AR labels and host display (§1)

- [x] 1.1 `ParamDisplayNames::forAudioPairAr` full Attack/Release labels
- [x] 1.2 `scripts/generate-host-display.mjs`; `web/src/hostDisplay.generated.ts` synced
- [x] 1.3 Playwright selectors updated for full Attack labels

### Pair-AR global Crunchy engine path (§2)

- [x] 2.1 Pair-AR through `Page::ApplyV2MusicalFuego` on `UsesV2Fuego`
- [x] 2.2 Audio page pointer wired from `PagedHostIO` / `DesktopHostIO`
- [x] 2.3 `AudioPairArEffective_test.cpp` Crunchy/Crispy cases
- [x] 2.4 WASM rebuild; web pair-AR effective reflects Crunchy/Crispy

### Operator documentation (§3)

- [x] 3.1 Quick Dict Scenes/Gestures/Sequencer/Crunchy matrix
- [x] 3.2 `SIM_MANUAL.md` Crunchy/pair-AR bullets
- [x] 3.3 `sync-help-docs.sh` + `check_operator_docs_sync.sh`
- [x] 3.4 Desktop v2 embedded Quick Dict assets rebuilt

### Desktop v2 performance band (§4)

- [x] 4.1 `PerformanceBandV2`: S1–S3 + blend, G1/G2 + weight sliders, sequencer transport
- [x] 4.2 Gesture weight → `MessageIn::GestureWeight`; scene bus wired
- [x] 4.3 Band between scopes and carousel; `DesktopV2ChromeLayout` heights
- [x] 4.4 Duplicate scene/gesture/sequencer removed from `GlobalStripV2`
- [x] 4.5 LFO/VCO hidden per bus audit

### Desktop v2 module layout scaffolding (§5)

- [x] 5.1 `kVisibleEncoderSlots` raised to 10
- [x] 5.2 ~~Bank hide when all rows fit~~ superseded — bank paging removed (Phase B1)
- [x] 5.3 `SubmodulePagePanel` / `AdsrPagePanel` up to 10 ring rows
- [x] 5.4 `PageCarouselComponent` reflow started (full centered group → Phase C 4.1)

### Web transport, morph sync, external meter (§6)

- [x] 6.1 `requireEngineForAction` + `#rand-morphs` gate aligned with `#rand-all`
- [x] 6.2 `lastMorphs` + `renderVcoMorphButtons` on every morph screen update
- [x] 6.3 `#external-meter-label` states Off / Waiting for Play / active
- [x] 6.4 `connectWorkletOutput` before peak; refresh after external enable
- [x] 6.5 Playwright: Rand waveforms + external meter specs

### Verification already green (§7–8 partial)

- [x] 7.1 `ctest --test-dir sim/build` including pair-AR Crunchy tests
- [x] 7.4 Playwright v2 + mobile label + transport/meter specs green
- [x] 8.1 `make -C sim test` (`VcoAdsrState_test`, `V2ModSource_test`, `SequencerState_test`)
- [x] 8.2 Build `FroggersTigaV2.app`

---

## Phase B0 — Grid layout foundation (`desktop-v2-grid-layout`)

- [x] B0.1 Add `kGridUnitPx = 10`, `gridPx(int u)` to `DesktopV2ChromeLayout.hpp`
- [x] B0.2 Derive constants from grid: ring 5u, row **5u**, label 9u, mod 7u×5u, transport+scope 7u (standalone) / VST scope strip 5u, default 128u×**92u**
- [x] B0.3 Document control footprints in header comment (encoder 5×5, arrow 2×2, text btn (text+2)×3)
- [x] B0.4 Refactor `SubmodulePagePanel::layoutRows` to grid footprints
- [x] B0.5 Refactor `PageCarouselComponent`, `PerformanceBandV2`, `SequencerPanelComponent`, `GlobalStripV2` to grid (**not** `ScopeGridComponent` — deleted in B0.7–B0.8)
- [x] B0.6 Verify `HostedMainComponentV2` uses same constants (no duplicate layout math)
- [x] B0.7 **Scope relocation (§0a):** extend `CvScopeDisplay` for multi-trace; add `VcoEfScopeDisplay` in transport row (standalone 7u) / VST top strip (5u); move Marbles LEDs to `PerformanceBandV2`; filter mod menu — omit indices 10–12
- [x] B0.8 **Delete scope grid:** remove `ScopeGridComponent`, `kScopeGridH`, layout passes from `MainComponent` / `HostedMainComponentV2`

---

## Phase B1 — Carousel scroll; remove bank paging

- [x] B1.1 Wrap carousel encoder area in `juce::Viewport`; document height = `rowsForPage × kEncoderRowH`
- [x] B1.2 Remove `m_bankPrev`, `m_bankNext`, `m_bankLabel` from `SubmodulePagePanel` / `AdsrPagePanel`
- [x] B1.3 Remove `SelectBank` message, `m_activeBankByPage`, `slotToRow` bank math, `onSelectBank` wiring
- [x] B1.4 Remove `setMaxVisibleRows`; `visibleCount` always `rowsForPage(activePage)`
- [x] B1.5 Tests: Filter 10 rows present with no bank controls; scroll reaches row 9 when viewport short

---

## Phase P — Pair-AR module (remove sustain; web AR parity)

- [x] P.1 `V2ParamDisplayNames`: page 6 → **Pair-AR**; rows Atk1/Rel1/Atk2/Rel2/Atk3/Rel3/Crispy; `CrispyRowForPage(6)=6`
- [x] P.2 `rowsForPage(6)` and `rowsForUiPage(6)` → **7**
- [x] P.3 Refactor `VcoAdsrState` → per-VCO AR (no sustain knob; hold 1.0; release max ≥10s; open gate when `!m_playing` per §4.1)
- [x] P.4 `FroggersEngine.hpp`: voice *n* uses rows `2n` attack, `2n+1` release
- [x] P.5 `HostParameterInventoryV2`: drop Sus* knobs/depths; `FROGGERS_EXPECT_HOST_PARAM_COUNT_V2=142` (148−6 Sus axes)
- [x] P.6 `pageKnobDefault` for Pair-AR: attack ~0.05, release ~0.2 (no 0.8 sustain default)
- [x] P.7 `ControlCoreBridge_test`: remove sustain row assertion; add AR gate tests
- [x] P.8 `HostParameterProcessorV2_test`: assert count 142; no `Sus` stable IDs
- [x] P.9 VST preset load: ignore removed Sus* parameter IDs (`kPluginStateEnvelopeVersion` **5**; legacy `page6_row{1,4,7}_knob/depth` dropped)
- [x] P.10 Quick Dict: Pair-AR page, A/R per VCO, web AR parity. SIM_MANUAL: desktop Pair-AR only (no VST/VCV)
- [x] P.11 `crispyRowForPage(6)` → row **6** in rand/skip paths

---

## Phase A — Scene core parity (control core knob authority)

- [x] 0.1 `seedSceneCentersFromDefaults()` in ctor (`FroggersV2ControlCore.cpp` L50–52)
- [x] 0.1b **Audio VCO factory defaults:** `audioVcoFrequencyDefaultNorm()` (~0.058697) in `HostParameterInventoryV2.hpp`; `pageKnobDefault(0,0..2)` → 30 Hz; `vcoMorphDefault(index)` → sine/square/saw (`0.0/1.0/0.5`); wire `buildRuntimeDescriptor` VcoMorph branch; apply morph on cold start via `syncToHost` or `DesktopHostIO::Init`; extend `ControlCoreBridge_test` + `HostParameterProcessorV2_test`
- [x] 0.2 `resetParameter` → `pageKnobDefault` + `modDepthDefault()` (L599–615)
- [x] 0.3 `MessageIn::RandPage` + shared scene-slot helpers + **`randomizeSceneEndpointsAndBlend()`** (Rand All + Rand-seq only); align `onRandAll` crispy skip (L478)
- [x] 0.4 Carousel `onRandomize` → `RandPage` → `syncToHost`; no `EnqueueRandomizePanel` on v2 (`MainComponent.cpp` L86)
- [x] 0.4b Same `onRandomize` / `onRandMod` wiring in `HostedMainComponentV2.cpp` L37–39 (VST editor parity)
- [x] 0.4c Extract shared `wireCallbacks` helper used by `MainComponent` and `HostedMainComponentV2` (OMNI repetition — one host-message wiring path)
- [x] 0.5 `GlobalStripV2::pushRandAll`: drop `EnqueueRandomizeAllPages` on v2 (L109–112)
- [x] 0.8 **Crunchy scene encoder parity:** replace `m_globalCrunchy` scalar with `ParamState m_crunchy` (`sceneCenter[3]`); `globalCrunchy()` = `blendedSceneCenter(m_crunchy)`; seed all slots **0.0**; remove scalar branch in `onParamTurn` L332–335; `onRandAll` randomizes Crunchy slots (remove L507 scalar write)
- [x] 0.8b `FroggersV2UIState`: add Crunchy ring fields; `populateUiState()` fills arcs for global strip ring
- [x] 0.8c `GlobalStripV2`: replace `juce::Slider m_crunchy` with `EncoderRingComponent`; wire drag → `ParamTurn(kNumHostPages,0)`; shift+press → `resetCrunchy()`
- [x] 0.8d `setGlobalCrunchy` / host automation: write active scene slot at current blend ordinal; `syncToHost` unchanged path
- [x] 0.8e Tests: Crunchy ring edits scene slot; blend morph; Rand All randomizes three slots; shift-revert to 0
- [x] 0.6a Test `test_scene_centers_seeded_from_defaults`
- [x] 0.6b Test shift-revert to default
- [x] 0.6c Test `test_rand_page_updates_scenes`; per-page leaves endpoints/blend unchanged
- [x] 0.6d Test Rand All + Rand-seq randomize endpoints (L≠R) and blend; **replace** `ControlCoreBridge_test.cpp` L290–295 expectation (today fails if metadata changes)
- [x] 0.7 Quick Dict scene S1–S3 semantics; scenes vs gestures vs sequencer

---

## Phase H — Sequencer edit step, toolbar, Rand-seq, full snapshot

- [x] H.1 Expand `SequencerStepSnapshot`: per-row `sceneCenter[7][10][3]`, `crunchySceneCenter[3]`, `gestureWeight[2]`, `gate`, `hasData` (remove six-float L/R model)
- [x] H.2 Add `m_editStep` to `SequencerState`
- [x] H.3 Shared helpers: `randomizeSceneSlotsInto`, `randomizeFullStepSnapshot`, `captureFactoryStepSnapshot`
- [x] H.4 Rewrite capture/apply snapshot (no blend/ordinal overwrite)
- [x] H.5 `MessageIn::ResetSequencerStep` + extend `RandSequencerStep` (Step / Pattern / Full-step scope)
- [x] H.6 Sequencer toolbar: ← → dice + Step/Pattern toggle
- [x] H.7 Step grid: **single** click → `m_editStep`; **double** click → toggle gate; remove **both** legacy gate paths (`onClick` L16 and redundant `mouseDown` L94–100) — one double-click handler only
- [x] H.7c OMNI data-flow: verify step grid has exactly one gate-toggle code path (no duplicate `toggleStepGate` from click + mouseDown)
- [x] H.7b Step **right-click** context menu (`juce::PopupMenu`): **Reset** → `ResetSequencerStep`; **Randomize** → `RandSequencerStep` Full-step scope; set edit step on right-click; no gate toggle
- [x] H.8 Dual highlight playhead vs edit step
- [x] H.9 Dice → `RandSequencerStep`
- [x] H.10 Record capture on playhead advance when armed
- [x] H.11 Tests: snapshot round-trip, Step/Pattern dice, factory reset, full-step randomize, prev/next wrap
- [x] H.12 Quick Dict sequencer UX (7.1g); SIM_MANUAL desktop sequencer UX (7.1i)
- [x] H.15 Verify `HostedMainComponentV2` uses same `SequencerPanelComponent` wiring as `MainComponent` (no VST fork)
- [x] H.13 **Default VCO output + step gate guard:** `SequencerState::activeStepGate()`; gate resolver `m_playing ? (m_gateHigh || activeStepGate()) : true` in `DesktopHostIO.hpp` L639–641 and L661; test internal VCOs audible with Engine on + seq off + empty pattern; test lit step ignored when stopped; test pattern+MIDI OR when playing
- [x] H.14 Dim sequencer gate cells when `!m_playing`; restore full brightness when **Start Sequence** runs; refresh on `m_playing` change; document in Quick Dict (task 7.1f)

---

## Phase B2 — Default height unify

- [x] 1.1 `kDefaultHeight` → **gridPx(92)** = 920px (`DesktopV2ChromeLayout.hpp`)
- [x] 1.1b `Main.cpp` + `PluginEditorV2.cpp`: use layout constants (not hardcoded 820/880/1000)
- [x] 1.4 Verify at 920px with **5u rows** and scope consolidation (B0.7–B0.8): standalone Filter **10 rows** fit without scroll; Audio 8 fits with margin; VST fits all 10-row FX pages without scroll

---

## Phase C — Labels, rings, performance band polish, carousel header

- [x] 2.1 Remove `EncoderRingComponent` label paint / `setLabel()`
- [x] 2.2 Remove duplicate labels in `SubmodulePagePanel` / `AdsrPagePanel`
- [x] 2.3 Ring radius from row bounds
- [x] 3.1 Top transport → **Engine**
- [x] 3.2 Sequencer → **Start Sequence** / **Stop Sequence**; **Record**; width ≥ 108px
- [x] 3.3 **BPM** / **Steps** labels; **G1**/**G2** toggles
- [x] 3.4 Performance band widths — no ellipsis at 1280px
- [x] 3.4b Scene L/R indicators: button suffixes **S{n}·L** / **S{n}·R** from `leftSceneOrdinal`/`rightSceneOrdinal`; blend slider end labels **L** (blue) / **R** (orange)
- [x] 4.1 Centered `[←][Module: X][→]` carousel header

---

## Phase D — Host mod sync

- [x] 5.1 `FroggersV2HostBridge::syncFromHostModRoutes()`
- [x] 5.2 Call after host Rand Mod; refresh mod cells
- [x] 5.3 Test Rand Mod updates dropdown

---

## Phase E — Typography

- [x] 6.1 IBM Plex Sans assets
- [x] 6.2 `DesktopV2LookAndFeel` default in `Main.cpp` and `PluginEditorV2`

---

## Phase F — MIDI CV settings UX + mod wiring (desktop standalone only)

- [x] 8.1 ASCII status line; MIDI In + CV Assignments help
- [x] 8.2 MIDI CC A/B labels; Ch 0 → **Any**
- [x] 8.3 Pitch row parameter name display
- [x] 8.4 Ch column on Shift / Scene rows; loop row layout
- [x] 8.5 Gate row help text
- [x] 8.6 MIDI CC A/B in mod enum + `sourceValue`
- [x] 8.7 MIDI CV + control-core tests

---

## Phase G — Docs and automated verification

- [x] 7.1 G1/G2 labels in UI (Quick Dict partial)
- [x] 7.1b Scene/blend/gestures/sequencer glossary
- [x] 7.1c Randomize / Rand All scope in Quick Dict + SIM_MANUAL (desktop) — Rand All + Rand-seq also randomize L/R endpoints and blend
- [x] 7.1d Quick Dict: Crunchy scene-ring parity on desktop v2. SIM_MANUAL: desktop Crunchy/Pair-AR only (no VST/VCV)
- [x] 7.1e Quick Dict + SIM_MANUAL (desktop): Audio cold start — VCO1–VCO3 at **30 Hz**, morphs **sine / square / saw**
- [x] 7.1f Quick Dict + SIM_MANUAL (desktop): **Engine** on → internal VCOs by default; step gates only while **Start Sequence** runs; dimmed gate cells when sequencer stopped
- [x] 7.1g **Quick Dict + mirrors:** full sequencer UX; edit step; ←/→ + dice + Step/Pattern; single/double/right-click steps; dice vs context-menu Randomize; **VST v2** subsection (no Engine; same step grid; DAW MIDI transport)
- [x] 7.1i **SIM_MANUAL + mirrors (desktop/web only):** same sequencer UX as 7.1g minus VST rows; Pair-AR desktop section; **no VST, no VCV** wording
- [x] 7.1h Rebuild embedded Quick Dict assets (`sync-help-docs.sh`); grep SIM_MANUAL for accidental VST/VCV; fix stale Quick Dict “click toggles gate” / “Engine required before sound”
- [x] 7.2 Desktop v2 MIDI CV Quick Dict section
- [x] 7.3 `SIM_MANUAL.md` v2 MIDI CV table (not v1 two CC pairs)
- [x] 7.4 Run `scripts/sync-help-docs.sh`
- [x] 9.1 `ctest --test-dir sim/build`
- [x] 9.2 `ctest --test-dir desktop-v2/build`
- [ ] 9.3 Desktop manual QA checklist (labels, rows, arrows, mod rand, font, MIDI, sequencer toolbar, dimmed gate cells when stopped)

---

## Phase I — Release gates (merged from `v2-ux-and-operator-docs` §7–8)

- [ ] I.1 Desktop v2 manual QA: performance band, 10 visible rows on Filter, gesture weight → ring, carousel arrows (was v2-ux 7.2)
- [ ] I.1b VST v2 manual QA: same grid/labels/rows/sequencer toolbar + step single/double/right-click as desktop minus transport row; default VCO output with DAW playing
- [ ] I.2 Web manual QA: Attack/Release labels; Crunchy on pair-AR; Rand waveforms + transport (was 7.3)
- [ ] I.3 Build VST3/AU v2 artefacts (was 8.3)
- [ ] I.4 Manual desktop: carousel, ADSR gates, Crunchy scene ring, scene rings, gestures, sequencer record/playback + Phase H (was 8.4)
- [ ] I.5 DAW: MIDI map `Global/Crunchy`, `Pair-AR/Atk1`, `Pair-AR/Rel1`, `Sequencer/BPM` (no Sus* params)
- [ ] I.5b DAW: verify no Engine/Audio/MIDI settings in VST editor; Start Sequence responds to MIDI transport
- [ ] I.6 Manual web: expanded pages 1–5 + global Crunchy (was 8.6)
- [ ] I.7 `cd web && npm run test:e2e` full suite (was 8.7)
- [ ] I.8 v1 default build regression `BUILD_DESKTOP_V2=OFF` `BUILD_VST_V2=OFF` (was 8.8)

---

## OMNI verification gates (run before Phase I)

- [x] OMNI.1 Grep: no `EnqueueRandomizePanel` / `EnqueueRandomizeAllPages` in v2 host wiring (`MainComponent`, `HostedMainComponentV2`, `GlobalStripV2`)
- [x] OMNI.2 Grep: no `setLabel` on `EncoderRingComponent` after Phase C
- [x] OMNI.3 Grep: no `kBankRowH`, `SelectBank`, `m_bankPrev` after Phase B1
- [x] OMNI.4 Single `kDefaultHeight` source: `Main.cpp`, `PluginEditorV2`, `MainComponent`, `HostedMainComponentV2` all use `DesktopV2ChromeLayout::kDefaultHeight` (= `gridPx(92)`)
- [x] OMNI.5 `randomizeSceneSlotsInto` is the only scene-slot randomization loop (RandPage, Rand All, Rand-seq, full-step menu)
- [x] OMNI.6 `applySequencerStepSnapshot` performs one bulk copy into `m_params` then one `syncToHost` per step fire (no per-row host push in loop)
- [x] OMNI.7 Grep: no `ScopeGridComponent` / `kScopeGridH` after Phase B0.8
- [x] OMNI.8 Grep: `activeStepGate` defined; `DesktopHostIO` gate resolver uses `m_playing ? (m_gateHigh || activeStepGate()) : true` at both call sites
- [x] OMNI.9 Grep: `SequencerPanelComponent` — single `toggleStepGate` call site (double-click only)
- [x] OMNI.10 Grep: `wireCallbacks` defined once; `MainComponent` and `HostedMainComponentV2` call the shared helper (no duplicate callback lambdas)

---

## Cleanup

- [ ] 10.1 Remove stale `web/test-results` artifacts if regenerated
