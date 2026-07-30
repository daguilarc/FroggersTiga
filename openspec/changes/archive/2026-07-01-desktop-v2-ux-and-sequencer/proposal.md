## Why

**Canonical change:** `desktop-v2-ux-and-sequencer`

Merged on 2026-06-30 from:

| Prior change | Fate |
|---|---|
| `v2-ux-and-operator-docs` | Tasks §1–6 **done**; open release gates → Phase I; **archived** (superseded) |
| `desktop-v2-ui-polish` | Renamed and expanded → this change |

`froggerstiga-desktop-v2` shipped engine and chrome scaffolding. `v2-ux-and-operator-docs` landed performance band, Quick Dict sections, pair-AR/web transport fixes, and layout scaffolding (tasks §1–6 complete). Remaining work spans **desktop polish QA**, **scene/randomize parity**, **sequencer edit-step + Rand-seq**, **MIDI CV UX**, and **v2 release gates**.

**Partial fix already landed:** cold-start silence and shift-revert (`seedSceneCentersFromDefaults`, `resetParameter`); pair-AR Crunchy path; web transport/morph/meter fixes; performance band wired.

## What Changes

### VST v2 parity (shared chrome, host-unique I/O)

All UX, layout, scene, sequencer, and visual polish in this change apply to **both** standalone desktop v2 (`MainComponent`) and **VST v2** (`HostedMainComponentV2` / `PluginEditorV2`). The VST editor already embeds the same five chrome panels; this change upgrades that shared surface.

**VST preserves what desktop cannot replace:**

| Concern | Desktop v2 | VST v2 |
|---------|------------|--------|
| Audio transport | **Engine** button starts `AudioDeviceManager` | DAW owns transport; no Engine row |
| MIDI routing | `MidiCvSettingsComponent` — device pick + CV assignments | DAW MIDI bus → 148 host parameters (`vst-v2-daw-midi-to-any-parameter`) |
| External MIDI CC mod | Assignable **MIDI CC A/B** mod sources in scope grid + `setExternalMidiMod` | DAW maps CC to any parameter; no MidiCv settings UI |
| MIDI clock / transport | Optional in MIDI settings (standalone) | MIDI Start/Stop/Continue + clock from DAW (`AudioEngine::routeMidiMessage`) |
| Automation | N/A | Every knob, scene, gesture, sequencer field exposed (`vst-v2-full-parameter-surface`) |
| Preset state | Local files | `getStateInformation` / `setStateInformation` |

**VST receives from this change:** fixed grid layout, carousel header, encoder rows, performance band labels (**Start Sequence** not Engine), **full sequencer UX parity** (edit-step toolbar, single/double-click gate editing, step right-click **Reset**/**Randomize**, default internal VCO output, step-gate-only-while-playing), scene/Rand parity, Crunchy scene ring, typography, host→core mod sync after Rand Mod, single row labels, height-derived visible rows. `HostedMainComponentV2` embeds the **same** `SequencerPanelComponent` / `PerformanceBandV2` as standalone — no forked sequencer UI.

**VST does not receive:** transport row rename, MIDI CV settings UX (Phase F), standalone Audio/MIDI settings dialogs.

**Layout policy change:** Bank paging is **removed**. Carousel scrolls when rows don't fit. Default height **92u (920px)** — not 100u.

**Pair-AR refactor:** Page 6 renamed **Pair-AR**; sustain rows removed; six A/R knobs + Crispy (7 rows). Per-VCO AR with **open gate by default** (`!m_playing`); performance gating from step gates + MIDI while **Start Sequence** runs. Release range extended. VST host param count **148 → 142** (six Sus axes removed). Web parity = AR envelope model (no sustain knob); web keeps 4 pair-sum knobs on Audio page.

**OMNI:** One layout authority (`DesktopV2ChromeLayout` grid constants) feeds both hosts; no duplicate layout math in `HostedMainComponentV2`.

- **Scene state parity (control core is knob authority on v2)** — *partial*:
  - ~~**Seed on init**~~ **DONE**: ctor calls `seedSceneCentersFromDefaults()` (`FroggersV2ControlCore.cpp` L50–52, L582–597).
  - ~~**Shift+revert to default**~~ **DONE**: `resetParameter` uses `pageKnobDefault` and `modDepthDefault()` (`FroggersV2ControlCore.cpp` L599–615); tested (`ControlCoreBridge_test.cpp` L192–209).
  - **Per-page Randomize includes scenes**: carousel `onRandomize` only calls `EnqueueRandomizePanel` (`MainComponent.cpp` L86) — never updates `sceneCenter`. Add `MessageIn::RandPage` / `onRandPage(page)` randomizing all three scene slots per row; wire carousel; skip host enqueue on v2.
  - **Rand All**: `onRandAll` already randomizes scenes (`FroggersV2ControlCore.cpp` L480–486); remove redundant `EnqueueRandomizeAllPages` from `GlobalStripV2::pushRandAll` (`GlobalStripV2.cpp` L109–112) on v2 hosts.
  - **Docs**: Fix Quick Dict scene S1–S3 ("store on press" is false — `onSceneSelect` only sets L/R ordinals L416–426); add scenes/gestures/sequencer distinction and cold-start note.
  - **Audio VCO factory defaults** — *not started*: Audio page rows **0–2** (VCO1–VCO3) seed **30 Hz** frequency into all three scene slots via `pageKnobDefault(0, row)`; VCO morph host params default **sine / square / saw** (`0.0 / 1.0 / 0.5` per `EvalWaveMorph`) for indices **0 / 1 / 2**. Today all Audio rows default **0.5** (~632 Hz) and all morphs default **0.0** (sine). Apply on cold start through inventory + first `syncToHost` / `SetVcoMorph`.
- **Remove duplicate row labels**: `SubmodulePagePanel::refresh()` sets both `m_rowLabels` and `EncoderRingComponent::setLabel()` with the same `V2ParamDisplayNames::forHostPageRow` string (`SubmodulePagePanel.cpp` L166–170; `EncoderRingComponent::paint` L111–112). Keep one label surface (left column only).
- **All module rows in carousel document (scroll when clipped)**: Target **92u (920px)**, **5u row height** (no padding above 5u controls), carousel `Viewport`. After scope consolidation (`design.md` §0a), standalone at 920px fits **10** encoder rows without scroll (Filter page); Audio 8 fits with margin. Scroll only when the window is resized shorter than default.
- **Uniform encoder ring bounds**: `EncoderRingComponent` constructs at `kEncoderRingSize+8` but `layoutRows` assigns variable-width bounds; rings paint from `getLocalBounds()` causing size drift. Derive ring diameter from allocated row height once in layout.
- **Performance band labels — no truncation**: … rename transport buttons **Engine** vs **Start Sequence** …
- **Scene L/R indicators**: S1/S2/S3 show **·L** / **·R** on active endpoints; scene blend slider ends labeled **L** (blue) and **R** (orange) — wired from `uiState().leftSceneOrdinal` / `rightSceneOrdinal` (published L214–215, not read by UI today).
- **Resolve duplicate Play**: `MainComponent` top row `m_play` starts audio engine (`MainComponent.h` L51; `MainComponent.cpp` L49–54). `PerformanceBandV2::m_seqPlay` toggles `SequencerState::m_playing` (`PerformanceBandV2.cpp` L60–66). These are different actions; rename and document in Quick Dict.
- **Carousel arrows adjacent to title**: `PageCarouselComponent::resized` still places `m_prev` at far left and `m_next` at far right (`PageCarouselComponent.cpp` L121–125). Implement design decision from `v2-ux-and-operator-docs/design.md` §5: `[←][Module: Audio][→]` centered group.
- **Mod dropdown refresh after Rand Mod**: `FroggersV2HostBridge::syncToHost` pushes knob values host←core only (`FroggersV2HostBridge.cpp` L29–44). `assignedModSource` reads core `ParamState` (`FroggersV2ControlCore.cpp` L133–147). Host `EnqueueRandomizePanelMod` never pulls routes back — dropdown stays "None" after randomize. Add `syncFromHostModRoutes()` for mod assignments after host mutations.
- **Typography**: Replace implicit JUCE default sans with bundled **IBM Plex Sans** via shared `DesktopV2LookAndFeel`; no Helvetica.
- **MIDI CV settings clarity** (`MidiCvSettingsComponent.cpp`):
  - Remove Unicode `→` from status line L317; use ASCII `->` or plain prose.
  - Add section help: **MIDI In** = pick one input device; **CV Assignments** = map incoming messages to targets.
  - Rename **Ext. mod A/B** → **MIDI CC A** / **MIDI CC B** with tooltip linking to module mod assignment.
  - Display Ch **0** as **Any** (engine treats `channel == 0` as all channels per `MidiCvAssignmentTable.cpp` L220–227).
  - Pitch row: show parameter name from `V2ParamDisplayNames::forHostPageRow(page, row)` beside numeric row (Page combo already uses `ParamDisplayNames::forHostPage`).
  - Add **Ch** column to Shift / Scene S1–S3 rows (`MidiCvButtonBinding::channel` exists; UI never exposes it — always defaults 0).
  - Gate toggle: add help text — drives ADSR/sequencer gate via `AudioEngine` `setHostGateCallback` → `SetGate`.
  - **Wire external MIDI CC into modulation**: `setExternalMidiMod` writes `m_externalMidiMods` (`FroggersV2ControlCore.cpp` L516–522) but `sourceValue` reads only `m_sourceValues` (L507–513); `externalMidiMod()` has no callers. Add assignable **MIDI CC A/B** mod sources in `ModSourceCell` and route `sourceValue` to `m_externalMidiMods`.
- **Quick Dict + SIM_MANUAL** — *partial*: Transport entries exist; scene semantics fixed in delta spec. **Open:** full sequencer UX in **Quick Dict** (+ VST subsection there); **SIM_MANUAL** updates **desktop v2 + web only** — **no VST, no VCV** in SIM_MANUAL for now. Also: VCO 30 Hz cold start, Crunchy scene ring, Rand All endpoint/blend, desktop MIDI CV in Quick Dict. Fix stale SIM_MANUAL ADSR/gated-always gate wording.
- **Sequencer step edit + Rand-seq** — *not started*:
  - **Step snapshot too small**: `SequencerStepSnapshot` stores only six global scene floats + two gesture weights + gate (`sim/SequencerState.hpp` L7–16). `applySequencerStepSnapshot` morphs those six values across **every** page/row using live `m_sceneBlend` (`FroggersV2ControlCore.cpp` L226–245). This cannot represent per-knob scene data like Rand All (`onRandAll` L482–488).
  - **No edit step**: only `m_playhead` exists (`SequencerState.hpp` L26); step clicks today toggle gate only (`SequencerPanelComponent.cpp` L16, L26–34) — target: **single** click = edit step, **double** click = gate.
  - **Rand-seq UI**: add **←** / **→** icon buttons to move an **edit step** within `0 .. patternLength-1` (wrap); **dice** icon button beside them runs scene-slot randomization into step buffer(s). **Step / Pattern** toggle: **Step** = selected edit step only; **Pattern** = every **blank** step in the pattern (steps with `hasData == false`).
  - **Step context menu** — *not started*: **right-click** any step cell opens **Reset** / **Randomize** for that step only. **Single** left-click selects **edit step**; **double** left-click toggles step gate (lit/rest).
  - **Rand-seq scope** matches Rand All scene slots + **randomizes L/R endpoints and blend** once per dice press (per-page Randomize still leaves endpoints/blend unchanged).
  - **Record capture**: `captureSequencerStepSnapshot` exists but has no callers (`FroggersV2ControlCore.cpp` L248–258); wire on playhead advance when `m_recordArm` after snapshot model expands.
- **VCO default audio + step gate policy** — *not started*:
  - **Today:** v2.0 disabled `AudioPairArState` and wired `VcoAdsrState` for sequencer/MIDI gates (`DesktopHostIO.hpp` L276–277) — AR silence until gate source was implementation convenience, not product intent. `DesktopHostIO` ORs `stepGate()` without `m_playing` (`L639–641`, `L661`).
  - **Default state:** With **Engine** / DAW audio on, output is driven by **internal VCOs** at current knobs — no MIDI note and no sequencer step required, including when **Start Sequence** is off or the pattern is empty.
  - **Start Sequence running:** Playhead step gates OR-combine with live MIDI/QWERTY/gate CV (`m_gateHigh`) to shape per-VCO AR during pattern playback.
  - **Step gates while stopped:** Pattern data only — stored in grid, ignored by envelopes (even if lit).
  - **Code:** `SequencerState::activeStepGate()` → `m_playing && stepGate()`; gate resolver: `m_playing ? (m_gateHigh || activeStepGate()) : true` at both `DesktopHostIO` call sites. Standalone + VST share one path.
  - **Stop Sequence:** Pattern gate contribution ends; output returns to continuous internal VCO level (open gate).
  - **UI:** Dim gate cells when `!m_playing` to distinguish stored pattern data from live pattern playback (lit = gate on when **Start Sequence** runs).
  - **Docs:** Engine on → internal VCOs drive sound by default; step gates affect envelopes only while **Start Sequence** runs. Web: no step grid; web Audio pair-AR stays always-on smoothing.
- **Global Crunchy scene encoder parity** — *not started*:
  - **Today:** Crunchy is a scalar `m_globalCrunchy` (`FroggersV2ControlCore.hpp` L222); `onParamTurn` bypasses scenes on `kCrunchyPage` (`FroggersV2ControlCore.cpp` L332–335); `GlobalStripV2` uses a plain rotary slider, not `EncoderRingComponent` (`GlobalStripV2.cpp` L16–32).
  - **Target:** Crunchy stores `sceneCenter[0..2]` like every module row; ring drag (no gesture) edits the active scene slot using live S1/S2/S3 blend; ring shows left/right scene arcs; `globalCrunchy()` / `syncToHost` / host `SetGlobalCrunchy` expose the **blended effective** value.
  - **Rand All / Rand-seq:** randomize all three Crunchy scene slots (not a separate scalar write at L507). Per-page carousel Randomize remains module-scoped — Crunchy unchanged.
  - **Shift+revert:** Shift+press on Crunchy ring resets all three slots to factory default (**0.0**).
  - **Web:** unchanged — web global Crunchy stays a single rotary (no `FroggersV2ControlCore`).
- **Fixed VCV-style grid layout** — *not started*:
  - Adopt a **10px base cell (u)**; smallest unit = one body-text character cell (IBM Plex Sans 11pt).
  - **Encoder ring = 5u × 5u**; **arrow buttons = 2u × 2u**; **text buttons = (textWidth + 2u) × 3u**; toggles **3u × 3u**.
  - Panel **128u × 92u** default (1280×920px); vertical chrome sections in integer **u** multiples.
  - Submodule row: `[label 9u | ring 5u | mod 7u wide × 5u tall]` per **5u** row — no vertical padding beyond control height.
  - Scope consolidation (`design.md` §0a): delete `ScopeGridComponent`; one triple-VCO EF scope in transport row (standalone **7u**) or VST top strip (**5u**); Marbles LEDs in performance band; pair/sum EF indices 10–12 removed from UI — see `desktop-v2-scope-visualization`.
  - Replace ad-hoc pixel constants with grid-derived values; both `MainComponent` and `HostedMainComponentV2` use the same footprints.
- **Remove bank paging; safer default height** — *not started*:
  - Delete bank prev/next, `SelectBank`, `setMaxVisibleRows` cap. All `rowsForPage` rows always in carousel; **vertical scroll** when window is short.
  - `kDefaultHeight` → **92u (920px)** — fits 1080p with margin (not 100u/1000px).
  - `Main.cpp` / `PluginEditorV2` use layout constants only.
- **Pair-AR module (was ADSR) — sustain removed** — *not started*:
  - Page 6 carousel label **Pair-AR**; **7 rows**: Atk1, Rel1, Atk2, Rel2, Atk3, Rel3, Crispy (A/R pairs per VCO).
  - Remove Sus1–Sus3 UI and host parameters (six fewer axes → **142** host params).
  - `VcoAdsrState` → per-VCO **AR** (no sustain knob; hold 1.0 while gate high; release max **≥10s**). Default gate **open** when sequencer stopped — internal VCOs drive output; pattern/MIDI gate only while **Start Sequence** runs (`design.md` §4.1).
  - `FroggersEngine` param indices: voice *n* attack row `2n`, release row `2n+1`.
  - Web parity: AR envelope family (no sustain knob); web keeps 4 pair-sum knobs on Audio — desktop keeps per-VCO layout on Pair-AR page.
  - VST: same UI + engine + inventory; drop `ADSR/Sus*` automation IDs; preset migration note in tasks.

## Capabilities

### New Capabilities

- `web-transport-morph-meter`: Web engine-readiness, Rand waveforms + VCO morph sync, external meter labels *(done — v2-ux §6)*
- `desktop-v2-row-layout`: All rows in carousel document; viewport scroll when clipped; **92u** default height
- `desktop-v2-single-row-label`: One label per encoder row from `V2ParamDisplayNames` authority — no duplicate ring header text.
- `desktop-v2-performance-band-labels`: Minimum control widths, explicit BPM/Steps labels, Engine vs Start Sequence naming.
- `desktop-v2-carousel-header`: Prev/next arrows flank module title as a centered group.
- `desktop-v2-host-mod-sync`: Host→core mod-route pull after randomize mod and page change.
- `desktop-v2-typography`: Bundled non-Helvetica sans-serif via application LookAndFeel.
- `desktop-v2-midi-cv-settings-ux`: MIDI settings copy, labels, channel semantics, pitch target names, shift/scene channel column, external CC modulation wiring.
- `desktop-v2-scene-core-parity`: Seed/revert; per-page Randomize; Rand All scene+mod scope; **Crunchy scene encoder parity**; v2 host decouple.
- `desktop-v2-control-core`: `MessageIn::RandPage` + carousel wiring.
- `desktop-v2-global-controls`: Fix Rand All scene wording; add per-page Randomize scene scope.
- `desktop-v2-sequencer-rand`: Full per-step scene snapshot, edit-step navigation (←/→), dice Rand-seq, Step/Pattern scope toggle.
- `desktop-v2-grid-layout`: VCV-inspired fixed cell grid; control footprints; grid-derived `DesktopV2ChromeLayout` constants; VST parity; carousel viewport scroll (no bank).
- `desktop-v2-scope-visualization` (delta): One triple-VCO EF scope in transport row (standalone) or VST top strip; Marbles LEDs in performance band; **delete** `ScopeGridComponent`.
- `desktop-v2-adsr-page` (delta): Pair-AR page — 7 rows, no sustain, per-VCO AR engine (open gate by default), VST inventory 142 params.

### Modified Capabilities

- `desktop-v2-scope-visualization`: Consolidate VCO1/2/3 EF into one multi-trace widget in transport/VST strip; remove pair/sum EF UI (indices 10–12); Marbles LEDs → performance band; delete scope grid band
- `audio-pair-ar-engine`, `audio-pair-ar-web-ui`, `pair-ar-rotated-desktop-labels`: Full Attack/Release labels; Crunchy on pair-AR for v2 hosts *(done)*
- `sim-operator-doc-parity`: Quick Dict / manual mirror parity
- `web-mobile-external-audio-routing`, `web-mobile-knob-labels`, `web-playwright-e2e`: Web QA gates *(mostly done; Phase I)*
- `operator-quick-dict-performance`: Performance sections + desktop v2 control map + MIDI CV + Rand-seq glossary
- `desktop-v2-page-carousel`: Row counts per module; carousel viewport scroll (no bank paging); height budget at 92u
- `desktop-v2-midi-cv-input`: Operator-facing help text, Ch-any semantics, external CC reachable as mod sources.
- `desktop-v2-sequencing`: Fix step recall semantics (per-row scene slots; blend/endpoints stay live); add edit-step toolbar and Rand-seq requirements; **default internal VCO output; step gates active only while Start Sequence runs**.
- `desktop-v2-control-core`: `RandSequencerStep` message; full snapshot capture/apply; shared scene-slot randomize helper with `onRandAll`.
- `vst-v2-midi-modulation` (delta): Editor parity — grid, Crunchy scene ring, **full sequencer UX** (toolbar, single/double/right-click steps, gate policy), performance band naming; DAW MIDI/automation paths.

## OMNI data-flow authority (canonical)

Single write paths — no duplicate authorities:

```
Knob/scene values:  UI ring → MessageIn → FroggersV2ControlCore::m_params.sceneCenter[*]
                    → syncToHost (output) → DesktopHostIO::SetPageKnob
Randomize (v2):     carousel/global → RandPage|onRandAll|RandSequencerStep (core only)
                    → syncToHost once — NOT EnqueueRandomizePanel on DesktopV2/VstV2
Mod routes:         host Rand Mod → syncFromHostModRoutes → core ParamState.modSource
                    → ModSourceCell refresh (after DrainPendingMutations)
Layout geometry:    DesktopV2ChromeLayout gridPx(u) → all chrome components
                    MainComponent + HostedMainComponentV2 share same layout functions
Sequencer steps:    dice/menu → randomize into m_steps[] (buffer only)
                    playhead → applySequencerStepSnapshot → m_params once per step
External MIDI CC:   MidiCv → m_externalMidiMods → sourceValue (MIDI CC A/B mod enum)
```

Shared helpers (one implementation, multiple call sites — OMNI repetition rule):

| Helper | Callers |
|--------|---------|
| `randomizeSceneSlotsInto(snapshot\|page)` | `onRandPage`, `onRandAll`, Rand-seq dice, `randomizeFullStepSnapshot` |
| `randomizeSceneEndpointsAndBlend()` | `onRandAll`, Rand-seq dice (Step + Pattern scopes only) |
| `captureFactoryStepSnapshot()` | Reset context menu, factory step baseline |
| `syncModRoutes(page, row)` | `syncToHost` knob path + `syncFromHostModRoutes` pull path |

## OMNI rule audit (2026-07-01)

Audit scope: `desktop-v2-ux-and-sequencer` planning artifacts + verified sources (`desktop-v2/Source/`, `sim/SequencerState.hpp`, `src/core/DesktopHostIO.hpp`). Tasks §1–6 from merged `v2-ux-and-operator-docs` are **done**; all Phase B0–I items remain open in code.

### Compliant (artifacts)

| Rule | Finding |
|------|---------|
| Data flow — canonical authority | `proposal.md` §OMNI data-flow authority maps single write paths for knobs, randomize, mod routes, layout, sequencer steps, external CC. |
| Repetition — shared helpers | `design.md` §10–11 and `tasks.md` Phase A/H define `randomizeSceneSlotsInto`, `randomizeSceneEndpointsAndBlend`, `captureFactoryStepSnapshot`, `syncModRoutes` with ≥2 callers each (helper extraction trigger met). |
| Nesting guardrail | `design.md` §713–715 mandates early-return sequencer mouse handler ≤3 levels. |
| Plan language — delta specs | Grep over `specs/**/*.md`: **zero** forbidden hedge terms (`maybe`, `might`, `could`, `if needed`, `consider`). Normative **may** only (RFC-style). |
| Verification gates | `tasks.md` §OMNI verification gates (OMNI.1–OMNI.10) + Phase I release matrix. |
| VST boundary | Proposal and `vst-v2-midi-modulation` delta preserve DAW MIDI/automation; standalone-only MIDI CV settings. |

### Gaps closed in this audit (artifact updates)

| Rule | Finding | Resolution |
|------|---------|------------|
| Artifact drift — scope grid | `desktop-v2-grid-layout` and `desktop-v2-page-carousel` still described 6-column scope grid band; `design.md` §0a deletes it. Code still ships `ScopeGridComponent`. | Align grid-layout + page-carousel specs with scope-visualization delta; fix `tasks.md` B0.5/B0.7 (delete scope grid, not refactor it). |
| Line-number drift | Proposal cited `rebuildVisibleSlots` cap at L622–625; cap is **L646**. | Correct audit table line refs. |
| Repetition — ADSR panel | `AdsrPagePanel.cpp` duplicates `setLabel` + bank UI same as `SubmodulePagePanel`. | Extend single-row-label scope + Phase C tasks; bank removal in B1 covers both panels. |
| Data flow — sequencer gate | `SequencerPanelComponent` toggles gate from **both** `onClick` and `mouseDown`. | Add task H.7c: one gate-toggle path; align with single/double-click spec. |
| Repetition — host wiring | `MainComponent` and `HostedMainComponentV2` duplicate identical `wireCallbacks` blocks. | Add task A 0.4c — extract shared `wireCallbacks` helper. |
| Symbol naming | Resolved 2026-07-01 — all artifacts use `syncFromHostModRoutes()`. | — |

### Implementation guardrails (from audit)

| Rule | Directive |
|------|-----------|
| Accumulate then apply | `applySequencerStepSnapshot` copies all `sceneCenter` into `m_params`, then **one** `syncToHost` — no per-row host push in the apply loop. |
| Accumulate then apply | `visibleCount = rowsForPage(page)` computed once; carousel viewport scrolls — do not cap slots in `rebuildVisibleSlots`. |
| Data flow | v2 randomize: `RandPage` / `onRandAll` / Rand-seq → control core only — grep gate OMNI.1 blocks `EnqueueRandomizePanel` on v2 hosts. |
| Defensive code | `activeStepGate()` guard is required because `DesktopHostIO` today ORs `stepGate()` without `m_playing` (`L639–641`, `L661`). |
| One-time helpers | `visibleRowCount(page, contentHeight)` — extract only if trigger ≥2 when implementing B1 viewport; default inline in layout header. |

## Implementation status (2026-07-01 OMNI audit — artifacts refreshed)

| Capability | Status | Verified evidence |
|---|---|---|
| `desktop-v2-scene-core-parity` | **PARTIAL** | Seed + shift-revert done; **Audio VCO 30 Hz defaults open**; RandPage + **Crunchy scene ring** open |
| `desktop-v2-control-core` | **NOT STARTED** | No `RandPage` message type; `HostedMainComponentV2.cpp` L37 still `EnqueueRandomizePanel` |
| `desktop-v2-global-controls` | **NOT STARTED** | Main spec still says Rand All skips scene centers (wrong); delta spec ready |
| `desktop-v2-row-layout` | **NOT STARTED** | No `setMaxVisibleRows`; `kDefaultHeight` still 880 |
| `desktop-v2-page-carousel` | **PARTIAL** | Row counts defined; height budget + visibleCount wiring open |
| `desktop-v2-single-row-label` | **NOT STARTED** | Duplicate `setLabel` at `SubmodulePagePanel.cpp` L166–170 |
| `desktop-v2-performance-band-labels` | **NOT STARTED** | Still Play/Rec; no BPM/Steps labels (`PerformanceBandV2.hpp` L31–36) |
| `desktop-v2-carousel-header` | **NOT STARTED** | Edge layout at `PageCarouselComponent.cpp` L121–125 |
| `desktop-v2-host-mod-sync` | **NOT STARTED** | `syncToHost` only (`FroggersV2HostBridge.cpp` L29–44) |
| `desktop-v2-typography` | **NOT STARTED** | No `DesktopV2LookAndFeel`; default JUCE font |
| `desktop-v2-midi-cv-input` | **PARTIAL** | Device dropdown exists; CC A/B not in mod enum (`kNumModSources=8`) |
| `desktop-v2-midi-cv-settings-ux` | **PARTIAL** | Section headers only; Unicode arrow L317; Ext. mod A/B labels |
| `operator-quick-dict-performance` | **PARTIAL** | Scene fixed; **sequencer step UX + VST subsection open** (7.1g) |
| `sim-operator-doc-parity` | **PARTIAL** | **SIM_MANUAL** desktop sequencer/Pair-AR stale; **no VST/VCV in manual** (7.1i) |
| `vst-v2-midi-modulation` (delta) | **PARTIAL** | Sequencer **UI** parity spec added; chrome not yet applied |
| `desktop-v2-sequencer-rand` | **NOT STARTED** | 6-float snapshot; no edit step; no toolbar; **no step context menu** (`SequencerPanelComponent.hpp`) |
| `desktop-v2-sequencing` (delta) | **NOT STARTED** | Main spec L29–31 wrongly claims blend/endpoints recall; **VcoAdsr silence until gate** + step gates when stopped (`DesktopHostIO.hpp` L639–641) |
| `desktop-v2-grid-layout` | **NOT STARTED** | Ad-hoc pixels; no `kGridUnitPx`; bank paging still present |
| `desktop-v2-adsr-page` (delta) | **NOT STARTED** | 10-row ADSR with Sus*; `VcoAdsrState` sustain stage |

**OMNI violations to fix during implementation:**

| Violation | Type | Fix | Task |
|---|---|---|---|
| Duplicate label on row + ring | Repetition | Single `m_rowLabels` path; remove `EncoderRingComponent::setLabel` paint | C 2.1–2.2 |
| Dual randomize authority core+host | Data flow | `RandPage` in `MainComponent` **and** `HostedMainComponentV2`; drop host enqueue on v2 | A 0.4 |
| `m_externalMidiMods` not read in `sourceValue` | Broken pipeline | MIDI CC A/B mod sources in `sourceValue` | F 8.6 |
| `visibleCount` capped by `rebuildVisibleSlots` | Data flow | `visibleCount = rowsForPage(page)` always; viewport scroll (`FroggersV2ControlCore.cpp` L646) | B1.4 |
| Duplicate label on ADSR panel | Repetition | Same `setLabel` path as Submodule (`AdsrPagePanel.cpp` L135–138) | C 2.2 |
| Dual gate-toggle paths on step click | Data flow | Remove redundant `mouseDown` gate toggle; double-click only | H.7c |
| `ScopeGridComponent` still in tree | Scope drift | Delete per `design.md` §0a; embed `VcoEfScopeDisplay` | B0.7–B0.8 |
| Duplicate `wireCallbacks` blocks | Repetition | Extract shared `wireCallbacks` helper for both hosts | A 0.4c |
| Repeated rand scene loops | Repetition | Shared `randomizeSceneSlotsInto` + `randomizeSceneEndpointsAndBlend` | A 0.3, H.3 |
| Ad-hoc layout constants per component | Repetition | Grid-derived `DesktopV2ChromeLayout` via `gridPx(u)` | B0 |
| `Main.cpp` 820 vs layout 880 vs target 920 | Inconsistency | Single `kDefaultHeight = gridPx(92)` everywhere | B2 1.1–1.1b |
| Bank paging hides rows | UX + repetition | Delete bank UI + `SelectBank`; carousel viewport | B1 |
| Stale bank row in grid spec diagrams | Artifact drift | Carousel flex = header 3u + N×5u only; no scope grid band | B0/B1 |
| Stale scope grid in grid-layout spec | Artifact drift | Transport-embedded scope per §0a; delete `ScopeGridComponent` | B0.7–B0.8 |
| ADSR sustain vs web AR model | Scope drift | Pair-AR 7-row refactor | P |
| Step gate without `m_playing` guard | Broken semantics | `activeStepGate()` + open gate when `!m_playing` | H.13 |
| VcoAdsr default silence on Engine start | Implementation debt | Gate resolver §4.1 | H.13, P.3 |
| `syncToHost` only after Rand Mod | Broken pipeline | `syncFromHostModRoutes` after host mod mutations | D 5.1–5.2 |
| Step snapshot apply mutates per-row without accumulate | Efficiency | `applySequencerStepSnapshot` copies all `sceneCenter` then one `syncToHost` | H.4 |
| MIDI CV binding row layout copy-paste | Repetition | Loop Shift/Scene rows in `MidiCvSettingsComponent` | F 8.4 |
| VST spec 128u×100u height | Artifact drift | 128u×92u per grid spec | B2 |

## Impact

- `desktop-v2/Source/ui/DesktopV2ChromeLayout.hpp` (grid unit + derived constants)
- `desktop-v2/Source/ui/SubmodulePagePanel.cpp`, `AdsrPagePanel.cpp`, `EncoderRingComponent.cpp`, `PageCarouselComponent.cpp`, `PerformanceBandV2.cpp`, `SequencerPanelComponent.cpp`, `GlobalStripV2.cpp` (delete `ScopeGridComponent.*` in Phase B0.8)
- `desktop-v2/Source/MainComponent.cpp`, `HostedMainComponentV2.cpp`, `Main.cpp`, `PluginEditorV2.cpp` (window height)
- `desktop-v2/Source/control/FroggersV2HostBridge.cpp` (+ new `syncFromHostModRoutes()` path)
- New font asset + `DesktopV2LookAndFeel` applied in `Main.cpp` and `PluginEditorV2`
- `desktop-v2/Source/MidiCvSettingsComponent.cpp`, `control/MidiCvAssignmentTable.*`, `control/FroggersV2ControlCore.cpp`, `control/FroggersV2HostBridge.cpp`, `ui/GlobalStripV2.cpp` (+ Crunchy `EncoderRingComponent`), `ui/ModSourceCell.cpp`
- `sim/SequencerState.hpp`, `src/core/DesktopHostIO.hpp` (**activeStepGate** guard), `desktop-v2/Source/ui/SequencerPanelComponent.*`, `FroggersV2ControlCore.cpp` (snapshot + rand-seq)
- `src/core/VcoAdsrState.hpp` → gated AR; `FroggersEngine.hpp` Pair-AR param routing
- `sim/V2ParamDisplayNames.hpp`, `HostParameterInventoryV2.hpp` (**Audio VCO 30 Hz + morph defaults**), `desktop/CMakeLists.txt` (`FROGGERS_EXPECT_HOST_PARAM_COUNT_V2=142`)
- `QUICK_DICT.md` + mirrors, `SIM_MANUAL.md`, `web/src/main.ts`, `sim/ParamDisplayNames.hpp`
- **Single apply target:** run `/opsx:apply` on `desktop-v2-ux-and-sequencer` only
