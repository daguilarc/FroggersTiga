## Context

`froggerstiga-desktop-v2` archived 2026-06-30. `v2-ux-and-operator-docs` added `PerformanceBandV2`, raised `kVisibleEncoderSlots` to 10, and raised `kDefaultHeight` to 880. Screenshot QA shows the app still violates carousel and performance-band specs.

**2026-06-30 implementation audit:** Scene cold-start and shift-revert landed in control core (`FroggersV2ControlCore.cpp` L50–52, L599–615; tests at `ControlCoreBridge_test.cpp` L47–85, L192–209). All other capabilities remain open. Standalone app opens at 820×1280 (`Main.cpp` L43) while `MainComponent` resizes to 880 (`MainComponent.cpp` L78) — both below the **920px** target (`gridPx(92)`).

Verified chrome budget at **current** size (880px, bank row still present):

```
880 total
 −16  chrome pad (kChromePad × 2)
 −38  transport row + gap
 −50  global strip + gap
 −138 sequencer + gap (m_sequencerVisible=true, MainComponent.h L55)
 −94  scope grid + gap
 −78  performance band + gap
 ───
 466  carousel remainder
 −30  carousel header (PageCarouselComponent L120)
 −30  bank/button row (SubmodulePagePanel L193–203)  ← removed in Phase B1
 ───
 406  encoder row area
 ÷72  kEncoderRowH today (→ gridPx(5)=50 after B0)
 =5.6  rows visible (Audio needs 8 in document)
```

**Target budget at 92u (920px), post-grid, scope grid removed, 5u rows (standalone):**

```
920 total (92u)
 −20  chrome pad (2u)
 −70  transport + VCO scope (7u)     ← was transport 3u + scope 9u + gap
 −10  gap
 −70  performance band (7u)         ← includes Marbles LEDs
 −10  gap
 −130 sequencer (13u)
 −10  gap
 −40  global strip (4u)
 −10  gap
 ───
 550  carousel flex
 −30  header (3u)
 ───
 520  encoder viewport
 ÷50  row (5u)
 ≈10.4 rows visible — Filter 10 fits without scroll
```

VST (5u scope strip, no transport): carousel viewport **~530px** → **~10.6 rows** at 920px.

`FroggersV2ControlCore::rebuildVisibleSlots` sets `m_visibleCount = min(remaining, kEncoderCount)` with `kEncoderCount=10` (`FroggersV2ControlCore.cpp` L622–625) — independent of panel height. `SubmodulePagePanel::layoutRows` always iterates `kVisibleEncoderSlots` slots (`L205–212). Rows 6–9 render below the clip rect.

## Goals / Non-Goals

**Goals:**
- **Playable cold start**: scene centers match `pageKnobDefault` before first `syncToHost`; Audio VCO1–VCO3 at **30 Hz** with **sine / square / saw** morphs (see §9.1).
- **Default audio is internal VCO-driven**: with Engine / DAW audio processing on, sound comes from internal VCOs at current knob values — no MIDI note and no sequencer step gate required, including when **Start Sequence** is off or the pattern is empty (see §4.1).
- **Randomize writes scenes**: per-page and Rand All update all `sceneCenter[0..2]` in control core; host randomize not knob authority on v2.
- **Shift+revert matches manual**: restore inventory defaults, not zero.
- One label per parameter row (OMNI repetition: single display path).
- Default window shows all module rows in carousel document (vertical scroll when clipped; **no bank paging**).
- Performance band controls readable without ellipsis.
- Distinct names for audio-engine start vs sequencer pattern play.
- Carousel arrows flank title.
- Mod dropdown reflects host state after Rand Mod.
- MIDI CV settings explain device vs assignment; external CC A/B assignable as mod sources.
- Non-Helvetica bundled sans font.

**Non-Goals:**
- v1 desktop layout changes.
- New engine features (LFO/VCO select buttons stay hidden per `v2-ux` design §6).
- Web UI changes in this change.
- Rearrangeable rack columns (grid is **fixed** like a single VCV module faceplate, not a patchable rack).
- **SIM_MANUAL** coverage of VST plugin hosting or VCV/grid layout jargon (Quick Dict may cover VST; see `sim-manual-excludes-vst-and-vcv`).

## Decisions

### 0. Fixed grid layout (VCV-inspired, not modular rack)

**Choice:** Introduce base cell **u = 10px** (`kGridUnitPx`). All chrome dimensions are integer multiples of **u**. Layout reads like a Eurorack faceplate at fixed width **128u** (1280px) — inspired by VCV Rack module panels (Marbles, reverb, delay, drive modules) for **density and vertical grouping**, not for patch cables or rearrangeable columns.

**Control footprints (authoritative):**

| Control | Size (u×u) | px @ default |
|---------|------------|--------------|
| Body text cell | 1×1 | 10×10 |
| Arrow / icon button | 2×2 | 20×20 |
| Toggle (S1–S3, G1–G2) | 3×3 | 30×30 |
| Text button height | 3 tall; width = text + 2u pad | — |
| Encoder ring | 5×5 | 50×50 |
| Mod dropdown cell | 7×5 | 70×50 |
| Row label column | 9×5 per row | 90×50 |
| Encoder row | 5 tall | 50 |
| Scope column | *(removed)* | Scope grid band deleted — see §0a |
| Sequencer step cell | 2×2 | 20×20 |

**Submodule row (one parameter — all FX pages share this):**

```
┌─ 9u label ─┬──── flex (ring 5u) ────┬─ 7u mod (5u tall) ─┐
│  "Decay"   │        ( ◯ )           │ [None ▼]             │
└────────────┴────────────────────────┴──────────────────────┘
                         5u row height (= ring height)
```

**Vertical stack (standalone; VST omits 3u transport):**

```
 7u  Transport + VCO EF scope (standalone) — buttons left, one triple-trace scope right
 5u  VCO EF scope strip (VST only)
 7u  Performance band (+ Marbles S&H LEDs at right)
 ──  Carousel flex: 3u header + N×5u rows (viewport scroll if resized shorter)
13u  Sequencer (3u toolbar + step grid)
 4u  Global strip (Crunchy 5u ring)
```

**VCV density reference (fixed panel, not wiring):**

| Froggers page | VCV analogue | Grid takeaway |
|---------------|--------------|---------------|
| Random (Marbles) | Marbles | Many similar small knobs in vertical rows; bag/slew pairs |
| Reverb | Plate/room verbs | Wet/time cluster top; tone/width mid; mod row bottom |
| Delay | Delay | Time/feedback/send vertical; color row at bottom |
| Drive | VCV distortion | Drive/shape prominent; bit-crush rows grouped |

**Alternative rejected:** JUCE `FlexBox`/`Grid` without a base unit — harder to explain and drifts from hardware-module readability.

**OMNI:** One constant source (`DesktopV2ChromeLayout`); `gridPx(u)` helper; both `MainComponent` and `HostedMainComponentV2` call the same layout functions.

### 0a. Chrome redistribution: one scope, no scope grid

**Choice:** Remove `ScopeGridComponent` entirely.

| Widget | Before | After |
|--------|--------|-------|
| VCO1/2/3 EF | 3 scope columns | **One** multi-trace scope (indices 7–9) |
| Pair/sum EF (10–12) | 3 scope columns | **Removed** — not in UI or mod dropdown |
| Marbles S&H (13–14) | 2 LEDs in scope grid | **Performance band** right edge |
| Scope grid band | 9u dedicated row | **Deleted** |

**Standalone:** `MainComponent` transport row grows **3u → 7u**. Engine / Stop / Audio / MIDI on the left; `VcoEfScopeDisplay` (multi-trace `CvScopeDisplay`) on the right flex (~88u wide).

**VST:** **5u** full-width scope strip above performance band (no transport buttons).

**Mod menus:** Dropdown lists indices **7–9** and **13–14** only for v2 tap sources. Indices **10–12** are not assignable (engine may still compute taps internally; no UI surface).

**Height win at 920px:** Removing scope band + gap while growing transport **+40u** nets **~60px** to carousel → **~one full encoder row** — Filter (10 rows) fits without scroll at default standalone size.

**Implementation:**
- Extend `CvScopeDisplay` with multi-trace `pushSample(trace, value)` + per-trace colors
- Add `VcoEfScopeDisplay` (or inline in `MainComponent` / `HostedMainComponentV2`)
- Move Marbles LED paint from `ScopeGridComponent::paint` into `PerformanceBandV2`
- Delete `ScopeGridComponent`, `kScopeGridH`, `kScopeGridH` layout passes
- Filter `ModSourceCell` menu: omit indices 10–12

### 0b. VST v2: shared chrome, host-unique I/O

**Choice:** Every grid, scene, sequencer, typography, and label change applies to `HostedMainComponentV2`. VST-specific behavior is **I/O boundary only**:

```
DAW MIDI/CC/automation ──► HostParameterRegistryV2 ──► control core / engine
DAW transport (MIDI start/stop) ──► sequencer m_playing
Desktop MIDI settings UI ──► NOT in VST (DAW mapping instead)
Engine button ──► NOT in VST (DAW audio transport)
```

**VST sequencer:** **Start Sequence** / **Stop Sequence** in performance band; BPM/Steps/Record arm automatable. **Same** step grid UX as standalone: single-click edit step, double-click gate, right-click **Reset** / **Randomize**, edit-step toolbar (←/→, dice, Step/Pattern). DAW audio transport replaces **Engine**; default internal VCO output and step-gate policy use the same `DesktopHostIO` path. No **Engine** label (no transport row).

**VST MIDI CC mod:** No `MidiCvSettingsComponent`; DAW maps CC to parameters. Desktop-only **MIDI CC A/B** mod sources in scope grid remain standalone + optional future VST sidechain — out of scope for VST settings UX in Phase F.

**Data flow preserved:**

```
PluginEditorV2 → HostedMainComponentV2 → same carousel/sequencer/global as MainComponent
processBlock → ingestMidiMessage → routeMidiMessage (clock, gate, CC via host params)
```

### 1. Height budget: 92u default, scroll not bank

**Choice:** Set `kDefaultHeight` to **92u** = **920px**. Fits 1080p laptops with taskbar/dock margin. Ten-row FX pages may need carousel scroll at default — acceptable trade vs 1000px clipping window chrome.

**Rejected:** 100u (1000px) default — too tall for many 900p/768p laptops. Bank paging — operator rejected; rows hidden behind bank arrows is worse than scroll.

**Choice — no bank paging:** Remove `SelectBank`, bank UI, `setMaxVisibleRows` cap. Wrap carousel encoder area in `juce::Viewport`. `visibleCount` always equals `rowsForPage(page)`. Document height = `rowsForPage × kEncoderRowH + header`.

**Also required:** `Main.cpp` and `PluginEditorV2` `centreWithSize` SHALL use `DesktopV2ChromeLayout::kDefaultWidth` × `kDefaultHeight`.

**Choice — row height = control height (5u):** `kEncoderRowH = gridPx(5)`. The 5u ring and 5u-tall mod dropdown define row height. Prior 7u rows wasted 2u (20px) padding per row; that padding is removed.

**Rejected (superseded):** 7u rows with centered 5u controls — caused unnecessary scroll at 920px. Prior rejection of &lt;6u rows assumed padding was required; it is not.

### 2. Single row label (left column)

**Choice:** Remove `EncoderRingComponent::m_label` paint path and `setLabel()` calls. Left `m_rowLabels` remains sole authority string.

**OMNI:** Eliminates duplicate `forHostPageRow` application on two components.

### 4. Performance band naming and layout

| Control | Current | New |
|---------|---------|-----|
| `MainComponent::m_play` | Play | **Engine** |
| `PerformanceBandV2::m_seqPlay` | Play | **Start Sequence** (idle) / **Stop Sequence** (playing); width ≥ 108px |
| `m_seqRecord` | Rec (40px → ellipsis) | **Record**; min width 56px |
| S1–S3 | 28px buttons | min width 32px |
| BPM slider | no label | `Label "BPM"` before slider |
| Pattern length | no label | `Label "Steps"` before slider |

**Alternative rejected:** **Seq** — operator feedback: too terse; collides mentally with Engine/Play.

Gesture toggles: **G1** / **G2** at 40px; full names in Quick Dict.

### 4.1 Default audio: internal VCOs; step gates only while Start Sequence runs

**Why v2 is silent today (implementation history, not product intent):**

Original v2.0 plan prioritized **sequencer step gates + MIDI note gates** and implemented that as `VcoAdsrState`, while **disabling** `AudioPairArState` on v2 hosts (`DesktopHostIO.hpp` L276–277 vs L281–282). AR is not inherently gated — v2 accidentally inherited “gate defaults false → silence until a gate source” because that was the convenient hook for pattern/MIDI performance.

```
Non-v2:  AudioPairArState ON  → pair-sum smoothing always active on Audio
v2:      AudioPairArState OFF → VcoAdsrState ON → MixOscVoices multiplies by envelope level
         gate = m_gateHigh || stepGate()         → no m_playing check (L639–641, L661)
```

**Product default:** When audio is processing (**Engine** on / DAW playing audio), output SHALL be driven by **internal VCOs** at the current knob/scene values. Operators SHALL hear knob tweaks without sending MIDI and without running the step sequencer — including when **Start Sequence** is off or the pattern is empty.

**Performance layer (Start Sequence running):** Step gates at the playhead and live MIDI/QWERTY/gate CV (`m_gateHigh`) OR-combine to shape per-VCO AR envelopes during pattern playback. Step gates stored in the grid while stopped are **pattern data only** (like muted automation lanes) — they do not affect envelopes.

**Choice — single gate resolver in `DesktopHostIO`:**

Add `SequencerState::activeStepGate()`:

```cpp
bool activeStepGate() const { return m_playing && stepGate(); }
```

Resolve envelope gate at both call sites (helper on `SequencerState` or `DesktopHostIO` keeps one path):

```cpp
const bool seqGate = m_sequencer.activeStepGate();
const bool performanceGate = m_gateHigh || seqGate;
const bool gate = m_playing ? performanceGate : true;
m_vcoAdsr.setGate(gate);
```

When `!m_playing`, gate is **true** — internal VCO path at full level (Pair-AR A/R knobs apply on future gate edges only; knob/scene/morph edits are audible immediately). When `m_playing`, pattern + live MIDI drive the gate. Standalone and VST share `DesktopHostIO` — no fork.

**Behavior matrix:**

| State | Step gates in grid | MIDI / QWERTY / gate CV | VCO envelope |
|-------|-------------------|-------------------------|--------------|
| Engine ON, Start Sequence OFF | Stored only, ignored | Optional live articulation; **not required** for sound | Open — internal VCOs drive output |
| Engine ON, Start Sequence ON | Playhead lit step ON | OR-combined with pattern | Pattern + live |
| Stop Sequence | Stored (playhead may sit on lit step) | Live notes still work if held | Returns to open gate — internal VCOs drive output |

**Operator feel:**

- **Engine on, tweak knobs** — hear internal VCOs immediately; no MIDI or sequencer required.
- **Engine on, gates lit in grid, sequencer stopped** — grid is programmed but does not gate; sound continues from internal VCOs.
- **Start Sequence** — pattern gates breathe; playhead moves; step snapshots fire (unchanged).
- **Stop Sequence** — pattern gate contribution ends; output returns to continuous internal VCO level (unless live MIDI is actively holding a performance gate).

**Edge cases (consistent):**

- DAW MIDI Start/Stop — same as Start/Stop Sequence; step gates affect envelopes only while `m_playing`.
- External MIDI clock — `advanceOnExternalClock` already requires `m_playing`.
- Record arm while stopped — no capture until clock runs (unchanged).
- Pair-AR refactor — per-VCO AR engine unchanged in default-open behavior; performance gating only while `m_playing`.

**UI (required):**

- Lit step = gate on when **Start Sequence** runs.
- Playhead ring = current playback position.
- When `!m_playing`, gate cells SHALL render dimmed (reduced opacity or muted fill) so stored pattern gates are visually distinct from live pattern playback.
- `SequencerPanelComponent` SHALL refresh gate cell appearance when `m_playing` changes.

**Web:** No step grid on v2 chrome; web Audio pair-AR is always-on smoothing via `AudioPairArState`. Desktop v2 default matches that spirit for knob tweaking (continuous internal VCO output); pattern gates join only when **Start Sequence** runs.

**Docs:** Quick Dict — with Engine on, internal VCOs drive sound by default; step gates affect envelopes only while **Start Sequence** is running.

### 5. Carousel header centered group

**Choice:** Replace edge-aligned layout with:
```
auto header = area.removeFromTop(kCarouselHeaderH);
auto group = header.withSizeKeepingCentre(28 + 4 + titleW + 4 + 28, header.getHeight());
// centre group in header; prev | title | next inside group
```

### 6. Host→core mod sync

**Choice:** Add `FroggersV2HostBridge::syncFromHostModRoutes()` reading `DesktopHostIO` mod index per page row (same authority as web/desktop v1). Call after `DrainPendingMutations()` in `MainComponent::timerCallback` when host version changes, and after Rand Mod enqueue.

**Alternative rejected:** Randomize through control core only — host owns sim mod state today; bridge must stay bidirectional for knobs already.

### 7. Typography: IBM Plex Sans

**Choice:** Bundle `IBMPlexSans-Regular.ttf` + `IBMPlexSans-SemiBold.ttf` in `desktop-v2/Assets/`, load in `DesktopV2LookAndFeel`, set as default in `Main.cpp` via `juce::LookAndFeel::setDefaultLookAndFeel`.

**License:** SIL Open Font License (verify file in repo before merge).

### 8. MIDI CV settings UX

**Two-step model (surface in UI copy):**

```
MIDI In device dropdown  →  which port/keyboard feeds the app
CV Assignments rows      →  what those messages control
```

**Choice:** Add `m_inHelp` and `m_assignHelp` labels (2 lines each, ASCII only).

**Stray character:** Replace `Computer keyboard → virtual` (`MidiCvSettingsComponent.cpp` L317) with `Computer keyboard -> virtual` or rephrase without arrow.

**Ext mod rename:** **MIDI CC A** / **MIDI CC B** — distinct from scope-grid taps (`V2ModSourceLabel` 7–14: envelope followers and Random S&H).

**Ch 0 = Any:** `MidiCvCcBinding::channel == 0` matches any channel (`MidiCvAssignmentTable.cpp` L220–221). Slider text box shows **Any** when value is 0 (custom `textFromValue` on Ch sliders).

**Pitch target:** Read-only label updates from `V2ParamDisplayNames::forHostPageRow(pitchPage, pitchRow)` on page/row change.

**Shift/scene channel:** Add Ch slider per binding row; sync `MidiCvButtonBinding::channel` (already honored in `handleNoteOn` / `handleCc` L161, L233).

**External CC modulation wiring (bug fix):**

Current data flow is broken:

```
MIDI CC  →  m_externalMidiMods[slot]   (setExternalMidiMod)
Mod math →  m_sourceValues[source]     (sourceValue only)
```

**Choice:** Extend `kNumModSources` by 2 OR reserve internal IDs 8–9 for **MIDI CC A/B**. Add menu items in `ModSourceCell::rebuildMenu`. Branch `sourceValue` to return `m_externalMidiMods[0|1]`. Single mod path — no duplicate CC ingest.

**Gate help:** One line under Gate toggle: "Note on/off drives ADSR and sequencer gates."

### 9. Control core owns knob values on v2 (scenes)

**Problem (verified):**

```
Init:  sceneCenter[*] = 0     (FroggersV2ControlCore.hpp L155)
       pageKnobDefault exists  (HostParameterInventoryV2.hpp L85–107) — unused
       syncToHost @ 15Hz        → host knobs = effective from scenes (all ~0) → silence

Randomize (per page): EnqueueRandomizePanel → host only (MainComponent.cpp L86)
                      syncToHost overwrites → no audible change

Rand All: onRandAll randomizes scenes (L480–486) ✅
          + EnqueueRandomizeAllPages redundant on v2 (GlobalStripV2.cpp L109–112)
```

**Choice — seed on construct:**

`seedSceneCentersFromDefaults()` in `FroggersV2ControlCore` ctor: for each page row, set `sceneCenter[0..2] = pageKnobDefault(page, row)`.

**Choice — shift+revert:**

`resetParameter` sets scenes to `pageKnobDefault`, `modDepth` to `modDepthDefault()` (0.5). Matches `SIM_MANUAL.md` L245. Gesture depths stay 0.

**Choice — per-page randomize:**

Add `MessageIn::Type::RandPage`, `onRandPage(uint8_t page)` — **scene slots only** (all three `sceneCenter[*]` per musical row), scoped to one page, skip `crispyRowForPage(page)`. Same scene loop as `onRandAll` L482–489 without mod depths, gestures, or Crunchy. Wire carousel `onRandomize` → core → `syncToHost()`. Do not call `EnqueueRandomizePanel` when `IsV2SimHostKind`.

**Crispy skip (spec):** `crispyRowForPage(page)` — Audio row 7, expanded/ADSR row 9. `onRandAll` today only skips ADSR; align both paths in task 0.3.

**Choice — Rand All:**

Keep `onRandAll` scene randomization. Remove `m_host->EnqueueRandomizeAllPages()` from `GlobalStripV2::pushRandAll` on v2.

**Choice — randomize L/R endpoints and blend (Rand All + Rand-seq only):**

`randomizeSceneEndpointsAndBlend()`: pick distinct `m_sceneLeftOrdinal`, `m_sceneRightOrdinal` ∈ {0,1,2}; `m_sceneBlend` ∈ [0,1]; `m_sceneSelectFlip = 0`. Called from `onRandAll` and Rand-seq dice — **not** from `onRandPage`. Per-page Randomize leaves morph assignment unchanged so a module-scoped action does not surprise global L/R.

Cold start defaults remain **S1·L / S2·R / blend 0.5** (deterministic factory baseline).

**Data flow after fix:**

```
ctor → seedSceneCentersFromDefaults()
     → pushSelectPage(0) → syncToHost → host hears defaults

Randomize(page) → onRandPage → all sceneCenter[*] on page → syncToHost

Shift+press → resetParameter → pageKnobDefault per row
```

**OMNI:** Single knob authority (core scenes); host `SetPageKnob` is output-only on v2. No duplicate randomize paths.

**Docs:** Quick Dict scene S1–S3 = L/R endpoint pick (`onSceneSelect` L416–426), ring edit writes scene slot; not capture-on-press.

### 9.1 Audio VCO cold-start defaults (30 Hz + waveform morph)

**Problem (verified):**

```
pageKnobDefault(0, row)     → 0.5 for all rows (HostParameterInventoryV2.hpp L107)
ExpMap(20 Hz, 20 kHz, 0.5)  → ~632 Hz audible at default blend (FroggersEngine.hpp L372–374)
morphDefault()              → 0.0 for all three VCO morph axes (L80–82, L314)
VcoWaveMorph m_knobValue    → struct default 0.0 (sine) until host sets morph (VcoWaveMorph.hpp L10)
```

**Choice — frequency (scene slots + host knobs):**

Add `audioVcoFrequencyDefaultNorm()` — inverse of engine `ExpParam::Compute(20, 20000, norm)` at **30 Hz**:

```
norm = log(30 / 20) / log(20000 / 20) ≈ 0.058697
```

`pageKnobDefault(0, row)` for `row ∈ {0, 1, 2}` returns that constant. All three scene slots seed to the same value via `seedSceneCentersFromDefaults()`. Other Audio rows keep **0.5** unless a future inventory rule overrides them.

**Choice — waveform morph (host-only axis):**

Replace flat `morphDefault()` for VcoMorph indices with `vcoMorphDefault(index)`:

| VCO | index | morph norm | heard wave (`EvalWaveMorph`) |
|-----|-------|------------|------------------------------|
| VCO1 | 0 | **0.0** | sine |
| VCO2 | 1 | **1.0** | square |
| VCO3 | 2 | **0.5** | saw |

Apply on cold start: after `FroggersV2HostBridge::syncToHost()` (or in `DesktopHostIO::Init` for v2 hosts), call `SetVcoMorph(i, vcoMorphDefault(i))` so factory state matches inventory before Engine. Shift+revert on morph is out of scope (morph is not scene-slotted on v2); **Rand waveforms** still randomizes all three.

**Scope:** Engine/inventory authority — desktop v2, VST v2, and WASM share the same defaults. No web UI layout change.

**Test:** Extend `test_scene_centers_seeded_from_defaults` — Audio rows 0–2 effective == `audioVcoFrequencyDefaultNorm()`; add morph default test after first sync (`GetVcoMorph(0)==0`, `GetVcoMorph(1)==1`, `GetVcoMorph(2)==0.5`).

### 11. Global Crunchy scene encoder parity

**Problem (verified):**

```
m_globalCrunchy scalar          (FroggersV2ControlCore.hpp L222)
onParamTurn kCrunchyPage        → direct scalar bump (L332–335), no sceneCenter
GlobalStripV2 m_crunchy         → juce::Slider rotary (L16–32), not EncoderRingComponent
onRandAll                       → m_globalCrunchy = rand (L507), not scene slots
Module rows                     → ParamState.sceneCenter[3] + ring arcs (onParamTurn L364–367)
```

**Choice — store Crunchy as scene slots:**

Add `ParamState m_crunchy` (scene centers only; no mod routes on Crunchy in v1 of this change). Remove `m_globalCrunchy` scalar.

```
globalCrunchy()  → blendedSceneCenter(m_crunchy)
setGlobalCrunchy(v) → write active scene slot OR distribute to match effective (host automation: set blended slot at current blend ordinal, same policy as module host knob writes)
```

Seed: `m_crunchy.sceneCenter[0..2] = 0.0f` in `seedSceneCentersFromDefaults()` (factory Crunchy off).

**Choice — global strip UI:**

Replace `juce::Slider m_crunchy` with `EncoderRingComponent m_crunchyRing` in `GlobalStripV2`. Label **Crunchy** beside ring (same strip position). Ring uses dedicated `FroggersV2UIState` fields (`crunchySceneLeft`, `crunchySceneRight`, `crunchyEffective`, …) populated in `populateUiState()`.

Mouse drag on ring posts `MessageIn::ParamTurn(kNumHostPages, 0, delta)`; handler uses **same scene-slot branch** as module rows (L364–367), not the scalar shortcut.

Shift+press on Crunchy ring calls `resetCrunchy()` → all three slots **0.0**.

**Choice — randomize scope:**

| Action | Crunchy scene slots |
|--------|---------------------|
| Per-page Randomize | unchanged |
| Rand All | all three slots randomized |
| Rand-seq | included in `randomizeSceneSlotsInto` + step snapshot `crunchySceneCenter[3]` |

**Choice — sequencer snapshot:**

Extend step struct with `crunchySceneCenter[kNumScenes]`; capture/apply alongside page/row scene data.

**Choice — web out of scope:**

Web/WASM global Crunchy remains a single rotary per `web-v2-parameter-subset` (no v2 control core).

**Data flow:**

```
Ring drag → sceneCenter[activeScene] on m_crunchy
         → globalCrunchy() = blend(sceneCenter, ordinals, m_sceneBlend)
         → syncToHost → SetGlobalCrunchy(effective)
```

### 10. Sequencer edit step, toolbar, and Rand-seq

**Problem (verified):**

```
SequencerStepSnapshot: 6 floats (L/R × 3 scenes) + 2 gestures + gate
                       (SequencerState.hpp L7–16)

applySequencerStepSnapshot: same 6 floats → ALL page/row sceneCenter[*]
                            via m_sceneBlend morph (FroggersV2ControlCore.cpp L226–245)

onRandAll scene loop: per page, per row, sceneCenter[0..2] (L482–488)

UI: title + step gate buttons only (SequencerPanelComponent.hpp L27–28)
    no m_editStep — only m_playhead (SequencerState.hpp L26)
```

**Choice — full scene snapshot per step:**

Replace the six-float L/R model with a fixed array matching control-core scene storage:

```
sceneCenter[kNumHostPages][kNumRows][kNumScenes]
crunchySceneCenter[kNumScenes]
gestureWeight[kNumGestures]
gate
hasData
```

Unused rows above `rowsForPage(page)` are ignored on apply. Size ≈ 7×10×3×4 + 8 + 1 + 1 bytes per step — acceptable for 64 steps.

**Choice — apply on step fire:**

`applySequencerStepSnapshot` copies stored `sceneCenter` into `m_params[page][row].sceneCenter[*]` for each page/row up to `rowsForPage`. Gesture weights copy from snapshot. **Live** `m_sceneBlend`, `m_sceneLeftOrdinal`, `m_sceneRightOrdinal` are **not** overwritten (matches Rand All / carousel Randomize policy and fixes incorrect main spec L29–31).

**Choice — edit step vs playhead:**

Add `m_editStep` to `SequencerState`. **Playhead** advances during **Start Sequence** playback. **Edit step** is the Rand-seq / manual navigation target.

| Control | Action |
|---------|--------|
| **←** (prev) | `m_editStep = (m_editStep + patternLength - 1) % patternLength` |
| **→** (next) | `m_editStep = (m_editStep + 1) % patternLength` |
| Step grid **single** click | Set `m_editStep` to clicked step; **no** gate change |
| Step grid **double** click | Toggle `m_steps[step].gate` (lit ↔ rest) |
| Step grid right-click | Context menu: **Reset** / **Randomize** for that step index (no gate toggle) |
| Playhead highlight | Blue ring on active playback step (existing `buttonOnColourId`) |
| Edit highlight | Distinct border or tint on `m_editStep` when `m_editStep != m_playhead` |

Toolbar row above step grid: `[←][→][dice]` left-aligned; **Step** / **Pattern** toggle right-aligned (or adjacent to dice per layout fit).

Icons: JUCE `DrawableButton` with vector paths (no external asset dependency) — chevron left/right, dice face (five-dot pattern).

**Choice — Rand-seq (dice):**

Shared helper `randomizeSceneSlotsInto(SequencerStepSnapshot&)` — scene slots for all pages/rows (skip Crispy) **plus** `crunchySceneCenter[0..2]`; gestures zeroed in output; no mod/Crunchy scalar write; no mod depths.

| Mode | Dice behavior |
|------|----------------|
| **Step** | Randomize once → write `m_steps[m_editStep]`; set `hasData = true` |
| **Pattern** | For `i` in `0 .. patternLength-1`: if `!m_steps[i].hasData`, randomize → write step `i` |

Dice does **not** change live `m_params` until playback hits the step or operator previews (preview out of scope — no live preview on dice press).

**Choice — step context menu (right-click):**

Each step cell in `SequencerPanelComponent` SHALL open a `juce::PopupMenu` on **right-click** with two items:

| Menu item | Action |
|-----------|--------|
| **Reset** | Write factory cold-start snapshot into `m_steps[step]` only |
| **Randomize** | Randomize all storable fields in `m_steps[step]` only |

**Reset** uses shared helper `captureFactoryStepSnapshot()` — same values as app cold start for step storage:

- `sceneCenter[page][row][scene]` = `HostParameterInventoryV2::pageKnobDefault(page, row)` for every page/row (includes Audio VCO 30 Hz + morph defaults on host sync path)
- `crunchySceneCenter[0..2]` = **0.0**
- `gestureWeight[0..1]` = **0.0**
- `gate` = **false**
- `hasData` = **true**

**Randomize** uses shared helper `randomizeFullStepSnapshot(SequencerStepSnapshot&)`:

- Randomize all `sceneCenter` musical rows (skip `crispyRowForPage(page)` on module pages); randomize `crunchySceneCenter[0..2]`
- Randomize `gestureWeight[0..1]` ∈ [0, 1]
- Randomize `gate` (uniform bool)
- Set `hasData = true`

Neither menu action mutates live `m_params`, other steps, mod depths, or global scene L/R/blend. Distinct from **Rand-seq dice**, which also calls `randomizeSceneEndpointsAndBlend()` once per press.

Right-click and single-click set `m_editStep` to the clicked step; neither toggles gate. **Double-click** toggles gate only.

**Data flow (extended):**

```
Right-click Reset     → captureFactoryStepSnapshot → m_steps[step]
Right-click Randomize → randomizeFullStepSnapshot → m_steps[step]
Dice (Step)           → randomizeSceneSlotsInto + randomizeSceneEndpointsAndBlend → m_steps[editStep]
```

**Choice — record capture:**

On playhead advance while `m_recordArm`, call expanded `captureSequencerStepSnapshot` from current `m_params` + gesture weights; set `hasData = true`. Single capture path shared with manual step authoring.

**Data flow:**

```
Dice (Step)  → randomizeSceneSlotsInto → m_steps[editStep]
Dice (Pattern) → for blank steps → same
Playback step  → applySequencerStepSnapshot → m_params scene + gestures
Record advance → captureSequencerStepSnapshot ← m_params
```

### 12. Pair-AR module: remove sustain, web AR parity

**Problem (verified):**

```
Page 6: 10 rows — Atk×3, Sus×3, Rel×3, Crispy  (V2ParamDisplayNames.hpp L40–41)
Engine: GetParam(0–2) atk, (3–5) sus, (6–8) rel  (FroggersEngine.hpp L679–681)
Web:    AudioPairArState — attack/release only, no sustain knobs
VST:    148 params incl. ADSR/Sus1–3 host automation
```

**Choice — seven-row Pair-AR page:**

| Row | Label | Engine |
|-----|-------|--------|
| 0 | Atk1 | VCO1 attack |
| 1 | Rel1 | VCO1 release |
| 2 | Atk2 | VCO2 attack |
| 3 | Rel2 | VCO2 release |
| 4 | Atk3 | VCO3 attack |
| 5 | Rel3 | VCO3 release |
| 6 | Crispy | page fuego |

Carousel module name: **Pair-AR** (replace "ADSR").

**Choice — per-VCO AR engine (performance gating when seq runs):**

Refactor `VcoAdsrState` to hold at **1.0** while gate high (no sustain knob). Release mapping: `kMinTimeSeconds`–**10s** at max knob (web pair-AR range spirit). Remove `Stage::Sustain` knob parameter; sustain *level* is always full after attack.

Per-v2 gate policy (`design.md` §4.1): when `!m_playing`, envelope gate is **open** so internal VCOs drive output; when `m_playing`, gate follows `m_gateHigh || activeStepGate()`. AR supports rhythm during pattern playback — it is not the reason for silence on Engine start.

**Choice — host inventory:**

Remove 3 Sus knobs + 3 Sus mod depths (6 axes total) → `FROGGERS_EXPECT_HOST_PARAM_COUNT_V2` **142** (148−6). VST preset load: ignore/map removed Sus stable IDs (`page6_row{1,4,7}_knob/depth`).

**Web parity clarification:**

| | Web | Desktop v2 / VST |
|--|-----|------------------|
| Envelope | AR (no sustain knob) | AR (no sustain knob) |
| UI layout | 4 pair-sum knobs on Audio page | 6 per-VCO knobs on Pair-AR page |
| Default output | Continuous internal VCO / pair-sum path | Continuous internal VCO path (gate open when `!m_playing`) |
| Performance gating | N/A (no step grid) | Step gates + MIDI while **Start Sequence** runs |

**Data flow:**

```
Gate on  → Attack → hold 1.0 per VCO
Gate off → Release from RelN knob (extended range) s
Page 6 knobs → m_adsrParams rows 0–5 → FroggersEngine apply(voice, atk, rel)
```

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| Taller FX pages need scroll at 920px | Carousel `Viewport`; Pair-AR only 7 rows |
| VST preset Sus* params orphaned | Preset version bump; ignore removed IDs on load |
| syncFromHostModRoutes duplication with syncToHost | Single `syncModRoutes(page, row)` helper; host is authority for mod after randomize |
| Font binary size | Subset or Regular+SemiBold only (~400KB) |
| External CC wiring touches mod enum | Extend `kNumModSources` to 10; update `ModSourceCell` colours array and tests |

## Migration Plan

Implementation order reflects dependency chain and partial progress already in tree:

```
Phase B0 — Grid constants + gridPx helper (DesktopV2ChromeLayout)   ← unblocks layout polish
Phase A — Scene core finish (RandPage, host decouple, docs)     ← seed/revert DONE
Phase H — Sequencer edit step, toolbar, Rand-seq, full snapshot
Phase B — Layout (kDefaultHeight 92u, viewport scroll, remove bank, Main.cpp + PluginEditorV2)
Phase C — Visual polish (labels, rings, perf band, carousel header) — grid footprints
Phase D — Host mod sync (syncFromHostModRoutes)
Phase E — Typography (LookAndFeel + font assets; desktop + VST editor)
Phase F — MIDI CV UX + external CC mod wiring (desktop only)
Phase G — Docs + verification (desktop + VST manual gates)
Phase P — Pair-AR refactor (7 rows, VcoAr engine, inventory 142, docs)
Phase I — Release gates (VST3/AU build, DAW MIDI map QA)
```

0. Scene core parity remainder (RandPage, Rand All cleanup, docs, tests) — unblocks randomize QA.
1. Layout math + viewport scroll + `Main.cpp` height (unblocks row visibility; no `setMaxVisibleRows`).
2. Label dedup + ring sizing.
3. Performance band + carousel header.
4. Host mod sync + refresh path.
5. Font + LookAndFeel.
6. MIDI CV settings UX + external CC mod wiring.
7. Quick Dict mirrors (+ SIM_MANUAL desktop-only per `sim-manual-excludes-vst-and-vcv`).
8. Manual QA checklist from tasks.md.
9. Sequencer snapshot + toolbar + Rand-seq (Phase H; depends on Phase A scene-slot rand helper).

## OMNI compliance map (verified violations → fix location)

| Violation | Type | Location | Fix | Task |
|---|---|---|---|---|
| Repetition: same label on row + ring | Repetition | `SubmodulePagePanel.cpp` L166–170, `AdsrPagePanel.cpp` L135–138 | Single `m_rowLabels` path; remove ring label paint | C 2.1–2.2 |
| Dual gate-toggle on step click | Data flow | `SequencerPanelComponent.cpp` L16 (`onClick`) + L94–100 (`mouseDown`) | Remove `mouseDown` gate path; double-click toggles gate only | H.7c |
| `ScopeGridComponent` not deleted | Scope drift | `MainComponent` + `HostedMainComponentV2` still wire scope grid | Delete component; embed triple-VCO scope per §0a | B0.7–B0.8 |
| Duplicate host callback wiring | Repetition | `MainComponent.cpp` L83–91, `HostedMainComponentV2.cpp` L34–42 | Extract shared `wireCallbacks` helper | A 0.4c |
| Dual randomize authority | Data flow | `MainComponent.cpp` L86, `HostedMainComponentV2.cpp` L37, `GlobalStripV2.cpp` L109–112 | Core-only `RandPage` / `onRandAll`; no host enqueue on v2 | A 0.4–0.5 |
| Broken CC mod pipeline | Data flow | `setExternalMidiMod` vs `sourceValue` | MIDI CC A/B in mod enum + `sourceValue` branch | F 8.6 |
| Repetition: shift/scene MIDI rows | Repetition | `MidiCvSettingsComponent.cpp` L497–522 | Loop binding rows | F 8.4 |
| `visibleCount` capped vs `rowsForPage` | Data flow | `rebuildVisibleSlots` L643–646 | `visibleCount = rowsForPage(page)`; viewport scroll | B1.4 |
| Bank paging | UX + repetition | `SubmodulePagePanel` bank row | Delete bank UI + `SelectBank` | B1 |
| Three default heights | Inconsistency | `Main.cpp` 820, layout 880, target 920 | `kDefaultHeight = gridPx(92)` everywhere | B2 |
| ADSR sustain rows | Scope drift | `VcoAdsrState`, inventory Sus* | Pair-AR 7-row refactor | P |
| Repeated rand scene loops | Repetition | `onRandAll`, RandPage, Rand-seq | `randomizeSceneSlotsInto` + `randomizeSceneEndpointsAndBlend` | A 0.3, H.3 |
| Snapshot apply without accumulate | Efficiency | `applySequencerStepSnapshot` | Copy all `sceneCenter` into `m_params`, then one `syncToHost` | H.4 |
| `syncToHost` only after Rand Mod | Data flow | `FroggersV2HostBridge` | `syncFromHostModRoutes` after host mod mutations | D 5.1–5.2 |
| Snapshot migration | One-time | Old 6-float steps | Version bump or reset `hasData` on load | H.1 |
| Stale 1000px / bank in specs | Artifact drift | grid-layout, vst specs | 92u height; carousel header + viewport only | B0/B1/B2 |
| Stale scope grid in specs | Artifact drift | grid-layout, page-carousel vs scope-visualization §0a | Transport-embedded scope; delete `ScopeGridComponent` | B0.7–B0.8 |

### Shared helper extraction (OMNI repetition — trigger ≥2 met)

| Helper | Boundary | Complexity | Contract | Callers |
|--------|----------|------------|----------|---------|
| `randomizeSceneSlotsInto` | Scene-slot rand domain step | ≥2 loops over pages/rows | In: snapshot or page scope; out: mutated scene slots | `onRandPage`, `onRandAll`, Rand-seq, `randomizeFullStepSnapshot` |
| `randomizeSceneEndpointsAndBlend` | Global morph assignment | 3 branches (L, R, blend) | Distinct ordinals; blend ∈ [0,1] | `onRandAll`, Rand-seq dice |
| `captureFactoryStepSnapshot` | Factory baseline | Inventory read pass | Matches cold-start step storage | Reset menu, tests |
| `syncModRoutes(page,row)` | Host↔core mod bridge | Bidirectional read/write | Host authority after Rand Mod | `syncToHost`, `syncFromHostModRoutes` |

### Implementation nesting (UI handlers)

Sequencer step grid mouse handler SHALL use early returns (right-click menu → return; double-click gate → return; single-click edit step) to stay ≤3 nesting levels. No nested `if` inside `for` inside `if` inside `try` in one function.

## Open Questions

None for UI polish. **Sequencer:** Pattern-mode dice fills **blank steps only** (confirmed). Non-blank steps are never overwritten by Pattern-mode dice.
